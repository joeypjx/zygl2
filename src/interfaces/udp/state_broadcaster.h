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
                BroadcastChassisStates();
                lastChassisTime = now;
            }

            // 广播告警消息
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastAlertTime).count() 
                >= m_alertBroadcastInterval) {
                BroadcastAlerts();
                lastAlertTime = now;
            }

            // 广播业务链标签
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastLabelTime).count() 
                >= m_labelBroadcastInterval) {
                BroadcastStackLabels();
                lastLabelTime = now;
            }

            // 短暂休眠，避免CPU占用过高
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    /**
     * @brief 广播所有机箱的状态
     */
    void BroadcastChassisStates() {
        // 获取系统概览
        auto response = m_monitoringService->GetSystemOverview();
        if (!response.success) {
            return;
        }

        // 为每个机箱发送一个数据包
        for (const auto& chassisDTO : response.data.chassis) {
            ChassisStatePacket packet;
            
            // 填充包头
            packet.header.sequenceNumber = m_sequenceNumber++;
            packet.header.timestamp = GetCurrentTimestampMs();
            
            // 填充机箱信息
            packet.chassisNumber = chassisDTO.chassisNumber;
            std::strncpy(packet.chassisName, chassisDTO.chassisName.c_str(), 
                        sizeof(packet.chassisName) - 1);
            packet.boardCount = static_cast<int32_t>(chassisDTO.boards.size());
            
            // 填充板卡信息 - 直接构造Board对象
            for (size_t i = 0; i < chassisDTO.boards.size() && i < 14; ++i) {
                const auto& boardDTO = chassisDTO.boards[i];
                
                // 构造Board对象
                packet.boards[i] = domain::Board(
                    boardDTO.boardAddress.c_str(),
                    boardDTO.slotNumber,
                    static_cast<domain::BoardType>(boardDTO.boardType)
                );
                
                // 注意：Board的任务信息通过UpdateFromApiData更新，
                // 但在广播时我们只需要基本状态，任务详情通过"方案B"按需查询
            }
            
            // 发送数据包
            SendPacket(&packet, sizeof(packet));
        }
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

