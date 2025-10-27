#pragma once

#include "../../domain/value_objects.h"
#include "../../domain/board.h"
#include "../../domain/alert.h"
#include <cstdint>
#include <cstring>

namespace zygl::interfaces {

// UDP多播配置
constexpr const char* MULTICAST_GROUP = "239.255.0.1";  // 多播组地址
constexpr uint16_t STATE_BROADCAST_PORT = 9001;         // 状态广播端口
constexpr uint16_t COMMAND_LISTEN_PORT = 9002;          // 命令监听端口

// 数据包类型枚举
enum class PacketType : uint16_t {
    // 状态广播包（服务端 -> 前端）
    ChassisState = 0x0001,          // 机箱/板卡状态包
    AlertMessage = 0x0002,          // 告警消息包
    StackLabel = 0x0003,            // 业务链标签包
    
    // 命令包（前端 -> 服务端）
    DeployStack = 0x1001,           // 部署业务链命令
    UndeployStack = 0x1002,         // 卸载业务链命令
    AcknowledgeAlert = 0x1003,      // 确认告警命令
    
    // 响应包（服务端 -> 前端）
    CommandResponse = 0x2001        // 命令响应包
};

// 命令结果枚举
enum class CommandResult : uint16_t {
    Success = 0,            // 成功
    Failed = 1,             // 失败
    InvalidParameter = 2,   // 参数无效
    NotFound = 3,           // 未找到
    Timeout = 4             // 超时
};

#pragma pack(1)

/**
 * @brief UDP数据包头（所有数据包共用）
 */
struct UdpPacketHeader {
    uint16_t packetType;        // 数据包类型（PacketType）
    uint16_t version;           // 协议版本（当前为1）
    uint32_t sequenceNumber;    // 序列号（用于检测丢包）
    uint64_t timestamp;         // 时间戳（毫秒）
    uint32_t dataLength;        // 数据长度（字节）
    char reserved[4];           // 保留字段
    
    UdpPacketHeader() 
        : packetType(0), version(1), sequenceNumber(0), 
          timestamp(0), dataLength(0) {
        std::memset(reserved, 0, sizeof(reserved));
    }
};

/**
 * @brief 资源监控报文头部（22字节）
 */
struct ResourceMonitorHeader {
    char data[22];              // 22字节头部数据
    
    ResourceMonitorHeader() {
        std::memset(data, 0, sizeof(data));
    }
};

/**
 * @brief 机箱/板卡状态数据包（旧协议，保留兼容）
 * 
 * 包含一个机箱的完整信息（14个板卡）
 */
struct ChassisStatePacket {
    UdpPacketHeader header;             // 数据包头
    int32_t chassisNumber;              // 机箱号（1-9）
    char chassisName[64];               // 机箱名称
    int32_t boardCount;                 // 板卡数量（固定14）
    domain::Board boards[14];           // 14个板卡的完整信息
    
    ChassisStatePacket() 
        : chassisNumber(0), boardCount(14) {
        header.packetType = static_cast<uint16_t>(PacketType::ChassisState);
        header.dataLength = sizeof(ChassisStatePacket) - sizeof(UdpPacketHeader);
        std::memset(chassisName, 0, sizeof(chassisName));
    }
};

/**
 * @brief 资源监控报文响应数据包
 * 
 * 按照资源监控报文响应协议定义，总计1000字节：
 * - 0-21字节：22字节头部
 * - 22-23字节：2字节命令码（F000H = 0xF000）
 * - 24-27字节：4字节响应ID
 * - 28-135字节：108字节板卡状态（9个机箱*12块板卡，1=正常，0=异常）
 * - 136-999字节：864字节任务状态（9个机箱*12块板卡*8个任务，1=正常，2=异常）
 */
struct ResourceMonitorResponsePacket {
    ResourceMonitorHeader header;        // 22字节头部
    uint16_t commandCode;               // 命令码 F000H (0xF000)
    uint32_t responseID;                // 响应ID（从0开始）
    // 板卡状态数组：9个机箱，每个机箱12个板卡
    uint8_t boardStates[9][12];         // 1=正常，0=异常
    // 任务状态数组：9个机箱，每个机箱12个板卡，每个板卡8个任务
    uint8_t taskStates[9][12][8];       // 1=正常，2=异常
    
    ResourceMonitorResponsePacket() 
        : commandCode(0xF000), responseID(0) {
        std::memset(boardStates, 0, sizeof(boardStates));
        std::memset(taskStates, 0, sizeof(taskStates));
    }
};

// 静态断言：确保数据包大小为1000字节
static_assert(sizeof(ResourceMonitorResponsePacket) == 1000, 
              "ResourceMonitorResponsePacket大小必须为1000字节");

/**
 * @brief 告警消息数据包
 * 
 * 包含一批告警消息（最多32条）
 */
struct AlertMessagePacket {
    UdpPacketHeader header;                 // 数据包头
    int32_t alertCount;                     // 告警数量（最多32）
    domain::Alert alerts[32];               // 告警数组
    char reserved[60];                      // 保留字段（用于对齐）
    
    AlertMessagePacket() 
        : alertCount(0) {
        header.packetType = static_cast<uint16_t>(PacketType::AlertMessage);
        header.dataLength = sizeof(AlertMessagePacket) - sizeof(UdpPacketHeader);
        std::memset(reserved, 0, sizeof(reserved));
    }
};

/**
 * @brief 业务链标签数据包
 * 
 * 包含多个业务链的标签信息（最多64个业务链）
 */
struct StackLabelPacket {
    UdpPacketHeader header;                             // 数据包头
    int32_t stackCount;                                 // 业务链数量
    struct StackEntry {
        char stackUUID[64];                             // 业务链UUID
        char stackName[128];                            // 业务链名称
        int32_t deployStatus;                           // 部署状态
        int32_t runningStatus;                          // 运行状态
        int32_t labelCount;                             // 标签数量
        domain::StackLabelInfo labels[8];               // 标签数组（最多8个）
        char reserved[12];                              // 保留字段
    };
    StackEntry stacks[64];                              // 业务链数组（最多64个）
    
    StackLabelPacket() 
        : stackCount(0) {
        header.packetType = static_cast<uint16_t>(PacketType::StackLabel);
        header.dataLength = sizeof(StackLabelPacket) - sizeof(UdpPacketHeader);
        std::memset(stacks, 0, sizeof(stacks));
    }
};

// ==================== 命令包定义 ====================

/**
 * @brief 部署业务链命令
 */
struct DeployStackCommand {
    UdpPacketHeader header;         // 数据包头
    char labelUUID[64];             // 标签UUID（根据标签批量部署）
    char operatorID[64];            // 操作员ID
    uint64_t commandID;             // 命令ID（用于响应匹配）
    char reserved[16];              // 保留字段
    
    DeployStackCommand() 
        : commandID(0) {
        header.packetType = static_cast<uint16_t>(PacketType::DeployStack);
        header.dataLength = sizeof(DeployStackCommand) - sizeof(UdpPacketHeader);
        std::memset(labelUUID, 0, sizeof(labelUUID));
        std::memset(operatorID, 0, sizeof(operatorID));
        std::memset(reserved, 0, sizeof(reserved));
    }
};

/**
 * @brief 卸载业务链命令
 */
struct UndeployStackCommand {
    UdpPacketHeader header;         // 数据包头
    char labelUUID[64];             // 标签UUID（根据标签批量卸载）
    char operatorID[64];            // 操作员ID
    uint64_t commandID;             // 命令ID（用于响应匹配）
    char reserved[16];              // 保留字段
    
    UndeployStackCommand() 
        : commandID(0) {
        header.packetType = static_cast<uint16_t>(PacketType::UndeployStack);
        header.dataLength = sizeof(UndeployStackCommand) - sizeof(UdpPacketHeader);
        std::memset(labelUUID, 0, sizeof(labelUUID));
        std::memset(operatorID, 0, sizeof(operatorID));
        std::memset(reserved, 0, sizeof(reserved));
    }
};

/**
 * @brief 确认告警命令
 */
struct AcknowledgeAlertCommand {
    UdpPacketHeader header;         // 数据包头
    char alertID[64];               // 告警ID
    char operatorID[64];            // 操作员ID
    uint64_t commandID;             // 命令ID（用于响应匹配）
    char reserved[16];              // 保留字段
    
    AcknowledgeAlertCommand() 
        : commandID(0) {
        header.packetType = static_cast<uint16_t>(PacketType::AcknowledgeAlert);
        header.dataLength = sizeof(AcknowledgeAlertCommand) - sizeof(UdpPacketHeader);
        std::memset(alertID, 0, sizeof(alertID));
        std::memset(operatorID, 0, sizeof(operatorID));
        std::memset(reserved, 0, sizeof(reserved));
    }
};

/**
 * @brief 命令响应包
 * 
 * 用于反馈命令执行结果
 */
struct CommandResponsePacket {
    UdpPacketHeader header;         // 数据包头
    uint64_t commandID;             // 命令ID（匹配请求）
    uint16_t originalCommandType;   // 原始命令类型
    uint16_t result;                // 命令结果（CommandResult）
    char message[256];              // 结果消息
    char reserved[8];               // 保留字段
    
    CommandResponsePacket() 
        : commandID(0), originalCommandType(0), result(0) {
        header.packetType = static_cast<uint16_t>(PacketType::CommandResponse);
        header.dataLength = sizeof(CommandResponsePacket) - sizeof(UdpPacketHeader);
        std::memset(message, 0, sizeof(message));
        std::memset(reserved, 0, sizeof(reserved));
    }
};

#pragma pack()

} // namespace zygl::interfaces

