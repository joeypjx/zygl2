#pragma once

#include "udp_protocol.h"
#include "../../application/services/monitoring_service.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <atomic>
#include <memory>
#include <chrono>
#include <cstring>

namespace zygl::interfaces {

/**
 * @brief StateBroadcaster - UDP状态广播器
 * 
 * 职责：
 * 1. 定期向前端多播系统状态（机箱/板卡、告警、业务链标签）
 * 2. 使用UDP多播协议，支持多个前端同时接收
 * 3. 三种广播周期可配置
 * 
 * 线程安全：
 * - 运行在独立线程中
 * - 通过std::atomic<bool>控制启停
 */
class StateBroadcaster {
public:
    /**
     * @brief 构造函数
     * 
     * @param monitoringService 监控服务（用于获取系统状态）
     * @param chassisBroadcastInterval 机箱状态广播间隔（毫秒）
     * @param alertBroadcastInterval 告警广播间隔（毫秒）
     * @param labelBroadcastInterval 标签广播间隔（毫秒）
     */
    StateBroadcaster(
        std::shared_ptr<application::MonitoringService> monitoringService,
        uint32_t chassisBroadcastInterval = 1000,    // 默认1秒
        uint32_t alertBroadcastInterval = 2000,       // 默认2秒
        uint32_t labelBroadcastInterval = 5000)       // 默认5秒
        : m_monitoringService(monitoringService),
          m_chassisBroadcastInterval(chassisBroadcastInterval),
          m_alertBroadcastInterval(alertBroadcastInterval),
          m_labelBroadcastInterval(labelBroadcastInterval),
          m_running(false),
          m_socketFd(-1),
          m_sequenceNumber(0) {
    }

    /**
     * @brief 析构函数
     */
    ~StateBroadcaster() {
        Stop();
    }

    /**
     * @brief 启动广播器
     * 
     * @return true 如果启动成功，false 如果失败
     */
    bool Start() {
        if (m_running.load()) {
            return false;  // 已经在运行
        }

        // 创建UDP socket
        m_socketFd = socket(AF_INET, SOCK_DGRAM, 0);
        if (m_socketFd < 0) {
            return false;
        }

        // 设置多播TTL
        int ttl = 64;
        if (setsockopt(m_socketFd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0) {
            close(m_socketFd);
            m_socketFd = -1;
            return false;
        }

        // 配置多播目标地址
        std::memset(&m_multicastAddr, 0, sizeof(m_multicastAddr));
        m_multicastAddr.sin_family = AF_INET;
        m_multicastAddr.sin_addr.s_addr = inet_addr(MULTICAST_GROUP);
        m_multicastAddr.sin_port = htons(STATE_BROADCAST_PORT);

        // 启动广播线程
        m_running.store(true);
        m_broadcastThread = std::thread(&StateBroadcaster::BroadcastLoop, this);

        return true;
    }

    /**
     * @brief 停止广播器
     */
    void Stop() {
        if (!m_running.load()) {
            return;  // 未运行
        }

        m_running.store(false);
        
        if (m_broadcastThread.joinable()) {
            m_broadcastThread.join();
        }

        if (m_socketFd >= 0) {
            close(m_socketFd);
            m_socketFd = -1;
        }
    }

    /**
     * @brief 检查是否正在运行
     */
    bool IsRunning() const {
        return m_running.load();
    }

private:
    /**
     * @brief 广播循环（运行在独立线程）
     */
    void BroadcastLoop() {
        auto lastChassisTime = std::chrono::steady_clock::now();
        auto lastAlertTime = std::chrono::steady_clock::now();
        auto lastLabelTime = std::chrono::steady_clock::now();

        while (m_running.load()) {
            auto now = std::chrono::steady_clock::now();

            // 广播机箱状态
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastChassisTime).count() 
                >= m_chassisBroadcastInterval) {
                if (!m_running.load()) break;  // 再次检查运行状态
                BroadcastChassisStates();
                lastChassisTime = now;
            }

            // 广播告警消息
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastAlertTime).count() 
                >= m_alertBroadcastInterval) {
                if (!m_running.load()) break;  // 再次检查运行状态
                BroadcastAlerts();
                lastAlertTime = now;
            }

            // 广播业务链标签
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastLabelTime).count() 
                >= m_labelBroadcastInterval) {
                if (!m_running.load()) break;  // 再次检查运行状态
                BroadcastStackLabels();
                lastLabelTime = now;
            }

            // 短暂休眠，避免CPU占用过高，并允许快速响应停止请求
            for (int i = 0; i < 10 && m_running.load(); ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
    }

    /**
     * @brief 广播所有机箱的状态
     * 
     * 使用ResourceMonitorResponsePacket协议，包含所有9个机箱的状态
     */
    void BroadcastChassisStates() {
        // 获取系统概览
        auto response = m_monitoringService->GetSystemOverview();
        if (!response.success) {
            return;
        }

        // 创建资源监控响应数据包
        ResourceMonitorResponsePacket packet;
        
        // 设置响应ID（从0开始递增）
        static uint32_t responseID = 0;
        packet.responseID = responseID++;
        if (responseID == 0xFFFFFFFF) {
            responseID = 0;  // 防止溢出
        }
        
        // 初始化数组为0（默认状态为异常/离线）
        std::memset(packet.boardStates, 0, sizeof(packet.boardStates));
        std::memset(packet.taskStates, 0, sizeof(packet.taskStates));
        
        // 遍历所有机箱，填充板卡和任务状态
        for (const auto& chassisDTO : response.data.chassis) {
            int32_t chassisIndex = chassisDTO.chassisNumber - 1;  // 机箱号1-9转换为索引0-8
            
            // 检查机箱号是否有效
            if (chassisIndex < 0 || chassisIndex >= 9) {
                continue;
            }
            
            // 填充板卡状态（12块板卡）
            for (size_t boardIdx = 0; boardIdx < chassisDTO.boards.size() && boardIdx < 12; ++boardIdx) {
                const auto& boardDTO = chassisDTO.boards[boardIdx];
                
                // 板卡状态：1=正常，0=异常
                // boardStatus: -1=未知，0=正常，1=异常，2=离线
                if (boardDTO.boardStatus == 0) {
                    packet.boardStates[chassisIndex][boardIdx] = 1;  // 正常
                } else {
                    packet.boardStates[chassisIndex][boardIdx] = 0;  // 异常或离线
                }
                
                // 填充任务状态（每个板卡最多8个任务）
                for (size_t taskIdx = 0; taskIdx < boardDTO.taskStatuses.size() && taskIdx < 8; ++taskIdx) {
                    const std::string& status = boardDTO.taskStatuses[taskIdx];
                    
                    // 任务状态：1=正常，2=异常
                    // 根据任务状态字符串判断
                    if (status.empty() || status == "unknown") {
                        packet.taskStates[chassisIndex][boardIdx][taskIdx] = 0;  // 未知
                    } else if (status == "normal" || status == "running") {
                        packet.taskStates[chassisIndex][boardIdx][taskIdx] = 1;  // 正常
                    } else {
                        packet.taskStates[chassisIndex][boardIdx][taskIdx] = 2;  // 异常
                    }
                }
            }
        }
        
        // 发送数据包（总计1000字节）
        SendPacket(&packet, sizeof(packet));
    }

    /**
     * @brief 广播告警消息
     */
    void BroadcastAlerts() {
        // 获取未确认的告警
        auto response = m_monitoringService->GetUnacknowledgedAlerts();
        if (!response.success || response.data.alerts.empty()) {
            return;
        }

        // 将告警分批发送（每包最多32条）
        AlertMessagePacket packet;
        packet.header.sequenceNumber = m_sequenceNumber++;
        packet.header.timestamp = GetCurrentTimestampMs();
        
        for (const auto& alertDTO : response.data.alerts) {
            if (packet.alertCount >= 32) {
                // 当前包已满，发送并创建新包
                SendPacket(&packet, sizeof(packet));
                
                packet = AlertMessagePacket();
                packet.header.sequenceNumber = m_sequenceNumber++;
                packet.header.timestamp = GetCurrentTimestampMs();
            }
            
            // 添加告警到当前包 - 构造Alert对象
            auto& alert = packet.alerts[packet.alertCount++];
            alert = domain::Alert(
                alertDTO.alertUUID.c_str(),
                static_cast<domain::AlertType>(alertDTO.alertType)
            );
            alert.SetTimestamp(alertDTO.timestamp);
            alert.SetRelatedEntity(alertDTO.relatedEntity.c_str());
            
            // 添加告警消息
            for (const auto& msg : alertDTO.messages) {
                alert.AddMessage(msg.c_str());
            }
            
            if (alertDTO.isAcknowledged) {
                alert.Acknowledge();
            }
        }
        
        // 发送最后一个包
        if (packet.alertCount > 0) {
            SendPacket(&packet, sizeof(packet));
        }
    }

    /**
     * @brief 广播业务链标签
     */
    void BroadcastStackLabels() {
        // 获取所有业务链信息
        auto response = m_monitoringService->GetAllStacks();
        if (!response.success || response.data.stacks.empty()) {
            return;
        }

        // 创建标签数据包
        StackLabelPacket packet;
        packet.header.sequenceNumber = m_sequenceNumber++;
        packet.header.timestamp = GetCurrentTimestampMs();
        
        for (const auto& stackDTO : response.data.stacks) {
            if (packet.stackCount >= 64) {
                // 包已满，发送并创建新包
                SendPacket(&packet, sizeof(packet));
                
                packet = StackLabelPacket();
                packet.header.sequenceNumber = m_sequenceNumber++;
                packet.header.timestamp = GetCurrentTimestampMs();
            }
            
            // 添加业务链到当前包
            auto& entry = packet.stacks[packet.stackCount++];
            std::strncpy(entry.stackUUID, stackDTO.stackUUID.c_str(), 63);
            std::strncpy(entry.stackName, stackDTO.stackName.c_str(), 127);
            entry.deployStatus = stackDTO.deployStatus;
            entry.runningStatus = stackDTO.runningStatus;
            entry.labelCount = static_cast<int32_t>(stackDTO.labelUUIDs.size());
            
            // 填充标签
            for (size_t i = 0; i < stackDTO.labelUUIDs.size() && i < 8; ++i) {
                std::strncpy(entry.labels[i].labelUUID, stackDTO.labelUUIDs[i].c_str(), 63);
                std::strncpy(entry.labels[i].labelName, stackDTO.labelNames[i].c_str(), 127);
            }
        }
        
        // 发送最后一个包
        if (packet.stackCount > 0) {
            SendPacket(&packet, sizeof(packet));
        }
    }

    /**
     * @brief 发送数据包
     */
    void SendPacket(const void* data, size_t size) {
        if (m_socketFd < 0) {
            return;
        }

        sendto(m_socketFd, data, size, 0, 
               (struct sockaddr*)&m_multicastAddr, 
               sizeof(m_multicastAddr));
    }

    /**
     * @brief 获取当前时间戳（毫秒）
     */
    uint64_t GetCurrentTimestampMs() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
    }

private:
    // 依赖服务
    std::shared_ptr<application::MonitoringService> m_monitoringService;
    
    // 配置参数
    uint32_t m_chassisBroadcastInterval;    // 机箱状态广播间隔（毫秒）
    uint32_t m_alertBroadcastInterval;      // 告警广播间隔（毫秒）
    uint32_t m_labelBroadcastInterval;      // 标签广播间隔（毫秒）
    
    // 运行状态
    std::atomic<bool> m_running;            // 是否正在运行
    std::thread m_broadcastThread;          // 广播线程
    
    // 网络相关
    int m_socketFd;                         // UDP socket文件描述符
    struct sockaddr_in m_multicastAddr;     // 多播目标地址
    uint32_t m_sequenceNumber;              // 序列号（用于检测丢包）
};

} // namespace zygl::interfaces

