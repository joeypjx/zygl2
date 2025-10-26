#pragma once

#include "udp_protocol.h"
#include "../../application/services/stack_control_service.h"
#include "../../application/services/alert_service.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <atomic>
#include <memory>
#include <cstring>
#include <functional>

namespace zygl::interfaces {

/**
 * @brief CommandListener - UDP命令监听器
 * 
 * 职责：
 * 1. 监听前端发送的UDP命令（加入多播组）
 * 2. 解析命令并调用相应的应用服务
 * 3. 发送命令响应包到前端
 * 
 * 支持的命令：
 * - 部署业务链（DeployStack）
 * - 卸载业务链（UndeployStack）
 * - 确认告警（AcknowledgeAlert）
 * 
 * 线程安全：
 * - 运行在独立线程中
 * - 通过std::atomic<bool>控制启停
 */
class CommandListener {
public:
    /**
     * @brief 构造函数
     * 
     * @param stackControlService 业务链控制服务
     * @param alertService 告警服务
     */
    CommandListener(
        std::shared_ptr<application::StackControlService> stackControlService,
        std::shared_ptr<application::AlertService> alertService)
        : m_stackControlService(stackControlService),
          m_alertService(alertService),
          m_running(false),
          m_socketFd(-1),
          m_responseFd(-1) {
    }

    /**
     * @brief 析构函数
     */
    ~CommandListener() {
        Stop();
    }

    /**
     * @brief 启动监听器
     * 
     * @return true 如果启动成功，false 如果失败
     */
    bool Start() {
        if (m_running.load()) {
            return false;  // 已经在运行
        }

        // 创建接收socket
        m_socketFd = socket(AF_INET, SOCK_DGRAM, 0);
        if (m_socketFd < 0) {
            return false;
        }

        // 设置socket选项：允许地址重用
        int reuse = 1;
        if (setsockopt(m_socketFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
            close(m_socketFd);
            m_socketFd = -1;
            return false;
        }

        // 绑定到多播端口
        struct sockaddr_in localAddr;
        std::memset(&localAddr, 0, sizeof(localAddr));
        localAddr.sin_family = AF_INET;
        localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        localAddr.sin_port = htons(COMMAND_LISTEN_PORT);

        if (bind(m_socketFd, (struct sockaddr*)&localAddr, sizeof(localAddr)) < 0) {
            close(m_socketFd);
            m_socketFd = -1;
            return false;
        }

        // 加入多播组
        struct ip_mreq mreq;
        mreq.imr_multiaddr.s_addr = inet_addr(MULTICAST_GROUP);
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);
        
        if (setsockopt(m_socketFd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
            close(m_socketFd);
            m_socketFd = -1;
            return false;
        }

        // 创建响应socket（用于发送命令响应）
        m_responseFd = socket(AF_INET, SOCK_DGRAM, 0);
        if (m_responseFd < 0) {
            close(m_socketFd);
            m_socketFd = -1;
            return false;
        }

        // 配置多播响应地址
        std::memset(&m_responseAddr, 0, sizeof(m_responseAddr));
        m_responseAddr.sin_family = AF_INET;
        m_responseAddr.sin_addr.s_addr = inet_addr(MULTICAST_GROUP);
        m_responseAddr.sin_port = htons(STATE_BROADCAST_PORT);

        // 启动监听线程
        m_running.store(true);
        m_listenerThread = std::thread(&CommandListener::ListenLoop, this);

        return true;
    }

    /**
     * @brief 停止监听器
     */
    void Stop() {
        if (!m_running.load()) {
            return;  // 未运行
        }

        m_running.store(false);
        
        if (m_listenerThread.joinable()) {
            m_listenerThread.join();
        }

        if (m_socketFd >= 0) {
            close(m_socketFd);
            m_socketFd = -1;
        }

        if (m_responseFd >= 0) {
            close(m_responseFd);
            m_responseFd = -1;
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
     * @brief 监听循环（运行在独立线程）
     */
    void ListenLoop() {
        constexpr size_t BUFFER_SIZE = 65536;
        char buffer[BUFFER_SIZE];

        while (m_running.load()) {
            struct sockaddr_in senderAddr;
            socklen_t senderLen = sizeof(senderAddr);

            // 接收数据
            ssize_t recvLen = recvfrom(m_socketFd, buffer, BUFFER_SIZE, 0,
                                      (struct sockaddr*)&senderAddr, &senderLen);

            if (recvLen < 0) {
                if (m_running.load()) {
                    // 只有在运行状态下的错误才需要处理
                    continue;
                } else {
                    break;
                }
            }

            // 解析并处理命令
            if (recvLen >= sizeof(UdpPacketHeader)) {
                ProcessCommand(buffer, recvLen);
            }
        }
    }

    /**
     * @brief 处理接收到的命令
     */
    void ProcessCommand(const char* data, size_t dataLen) {
        const UdpPacketHeader* header = reinterpret_cast<const UdpPacketHeader*>(data);
        PacketType packetType = static_cast<PacketType>(header->packetType);

        switch (packetType) {
            case PacketType::DeployStack:
                if (dataLen >= sizeof(DeployStackCommand)) {
                    HandleDeployStack(reinterpret_cast<const DeployStackCommand*>(data));
                }
                break;

            case PacketType::UndeployStack:
                if (dataLen >= sizeof(UndeployStackCommand)) {
                    HandleUndeployStack(reinterpret_cast<const UndeployStackCommand*>(data));
                }
                break;

            case PacketType::AcknowledgeAlert:
                if (dataLen >= sizeof(AcknowledgeAlertCommand)) {
                    HandleAcknowledgeAlert(reinterpret_cast<const AcknowledgeAlertCommand*>(data));
                }
                break;

            default:
                // 未知命令类型，忽略
                break;
        }
    }

    /**
     * @brief 处理部署业务链命令
     */
    void HandleDeployStack(const DeployStackCommand* cmd) {
        std::string labelUUID(cmd->labelUUID);
        
        // 构造DeployCommandDTO
        application::DeployCommandDTO command;
        command.stackLabels.push_back(labelUUID);

        // 调用业务逻辑
        auto response = m_stackControlService->DeployByLabels(command);

        // 发送响应
        SendCommandResponse(
            cmd->commandID,
            static_cast<uint16_t>(PacketType::DeployStack),
            response.success ? CommandResult::Success : CommandResult::Failed,
            response.message
        );
    }

    /**
     * @brief 处理卸载业务链命令
     */
    void HandleUndeployStack(const UndeployStackCommand* cmd) {
        std::string labelUUID(cmd->labelUUID);
        
        // 构造DeployCommandDTO（Deploy和Undeploy共用同一个DTO）
        application::DeployCommandDTO command;
        command.stackLabels.push_back(labelUUID);

        // 调用业务逻辑
        auto response = m_stackControlService->UndeployByLabels(command);

        // 发送响应
        SendCommandResponse(
            cmd->commandID,
            static_cast<uint16_t>(PacketType::UndeployStack),
            response.success ? CommandResult::Success : CommandResult::Failed,
            response.message
        );
    }

    /**
     * @brief 处理确认告警命令
     */
    void HandleAcknowledgeAlert(const AcknowledgeAlertCommand* cmd) {
        std::string alertID(cmd->alertID);
        // operatorID 可用于日志记录，但AlertService::AcknowledgeAlert不需要此参数

        // 调用业务逻辑
        auto response = m_alertService->AcknowledgeAlert(alertID);

        // 发送响应
        SendCommandResponse(
            cmd->commandID,
            static_cast<uint16_t>(PacketType::AcknowledgeAlert),
            response.success ? CommandResult::Success : CommandResult::Failed,
            response.message
        );
    }

    /**
     * @brief 发送命令响应
     */
    void SendCommandResponse(
        uint64_t commandID,
        uint16_t originalCommandType,
        CommandResult result,
        const std::string& message) {
        
        if (m_responseFd < 0) {
            return;
        }

        CommandResponsePacket response;
        response.header.timestamp = GetCurrentTimestampMs();
        response.commandID = commandID;
        response.originalCommandType = originalCommandType;
        response.result = static_cast<uint16_t>(result);
        std::strncpy(response.message, message.c_str(), sizeof(response.message) - 1);

        sendto(m_responseFd, &response, sizeof(response), 0,
               (struct sockaddr*)&m_responseAddr,
               sizeof(m_responseAddr));
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
    std::shared_ptr<application::StackControlService> m_stackControlService;
    std::shared_ptr<application::AlertService> m_alertService;
    
    // 运行状态
    std::atomic<bool> m_running;            // 是否正在运行
    std::thread m_listenerThread;           // 监听线程
    
    // 网络相关
    int m_socketFd;                         // 接收socket文件描述符
    int m_responseFd;                       // 响应socket文件描述符
    struct sockaddr_in m_responseAddr;      // 响应目标地址
};

} // namespace zygl::interfaces

