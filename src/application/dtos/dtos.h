#pragma once

#include "../../domain/value_objects.h"
#include <string>
#include <vector>
#include <cstdint>

namespace zygl::application {

/**
 * @brief DTOs（数据传输对象）
 * 
 * 用于在各层之间传递数据，避免领域对象泄露到外层。
 */

// ==================== 板卡相关DTOs ====================

/**
 * @brief 板卡DTO
 */
struct BoardDTO {
    std::string boardAddress;           // 板卡IP地址
    int32_t boardNumber;                // 板卡槽位号
    int32_t boardType;                  // 板卡类型（0-计算，1-交换，2-电源）
    int32_t boardStatus;                // 板卡状态（-1-未知，0-正常，1-异常，2-离线）
    int32_t taskCount;                  // 任务数量
    
    // 任务简要信息
    std::vector<std::string> taskIDs;
    std::vector<std::string> taskStatuses;
};

/**
 * @brief 机箱DTO
 */
struct ChassisDTO {
    int32_t chassisNumber;              // 机箱号
    std::string chassisName;            // 机箱名称
    std::vector<BoardDTO> boards;       // 板卡列表
    
    // 统计信息
    int32_t totalBoards;                // 总板卡数
    int32_t normalBoards;               // 正常板卡数
    int32_t abnormalBoards;             // 异常板卡数
    int32_t offlineBoards;              // 离线板卡数
    int32_t totalTasks;                 // 总任务数
};

/**
 * @brief 系统概览DTO
 */
struct SystemOverviewDTO {
    std::vector<ChassisDTO> chassis;    // 所有机箱
    
    // 系统级统计
    int32_t totalChassis;               // 总机箱数（应该是9）
    int32_t totalBoards;                // 总板卡数（应该是126）
    int32_t totalNormalBoards;          // 正常板卡总数
    int32_t totalAbnormalBoards;        // 异常板卡总数
    int32_t totalOfflineBoards;         // 离线板卡总数
    int32_t totalTasks;                 // 总任务数
};

// ==================== 业务链路相关DTOs ====================

/**
 * @brief 任务资源DTO
 */
struct TaskResourceDTO {
    std::string taskID;                 // 任务ID
    std::string taskStatus;             // 任务状态
    
    // 资源使用情况
    float cpuCores;                     // CPU总量
    float cpuUsed;                      // CPU使用量
    float cpuUsage;                     // CPU使用率
    float memorySize;                   // 内存总量
    float memoryUsed;                   // 内存使用量
    float memoryUsage;                  // 内存使用率
    float netReceive;                   // 网络接收流量
    float netSent;                      // 网络发送流量
    float gpuMemUsed;                   // GPU显存使用
    
    // 运行位置
    std::string chassisName;            // 机箱名称
    int32_t chassisNumber;              // 机箱号
    std::string boardName;              // 板卡名称
    int32_t boardNumber;                // 板卡槽位号
    std::string boardAddress;           // 板卡IP地址
};

/**
 * @brief 组件DTO
 */
struct ServiceDTO {
    std::string serviceUUID;            // 组件UUID
    std::string serviceName;            // 组件名称
    int32_t serviceStatus;              // 组件状态
    int32_t serviceType;                // 组件类型
    int32_t taskCount;                  // 任务数量
    
    std::vector<std::string> taskIDs;   // 任务ID列表
};

/**
 * @brief 业务链路DTO
 */
struct StackDTO {
    std::string stackUUID;              // 业务链路UUID
    std::string stackName;              // 业务链路名称
    int32_t deployStatus;               // 部署状态
    int32_t runningStatus;              // 运行状态
    
    // 标签
    std::vector<std::string> labelNames;
    std::vector<std::string> labelUUIDs;
    
    // 组件
    std::vector<ServiceDTO> services;
    
    // 统计
    int32_t serviceCount;               // 组件数量
    int32_t totalTaskCount;             // 总任务数
};

/**
 * @brief 业务链路列表DTO
 */
struct StackListDTO {
    std::vector<StackDTO> stacks;
    
    // 统计
    int32_t totalStacks;                // 总业务链路数
    int32_t deployedStacks;             // 已部署数
    int32_t normalRunningStacks;        // 正常运行数
    int32_t abnormalStacks;             // 异常数
};

// ==================== 告警相关DTOs ====================

/**
 * @brief 告警DTO
 */
struct AlertDTO {
    std::string alertUUID;              // 告警UUID
    int32_t alertType;                  // 告警类型（0-板卡，1-组件）
    uint64_t timestamp;                 // 时间戳
    bool isAcknowledged;                // 是否已确认
    
    // 相关实体
    std::string relatedEntity;          // 相关实体ID
    
    // 告警消息
    std::vector<std::string> messages;
    
    // 位置信息
    std::string chassisName;
    int32_t chassisNumber;
    std::string boardName;
    int32_t boardNumber;
    std::string boardAddress;
    
    // 组件告警专用字段
    std::string stackName;
    std::string stackUUID;
    std::string serviceName;
    std::string serviceUUID;
    std::string taskID;
};

/**
 * @brief 告警列表DTO
 */
struct AlertListDTO {
    std::vector<AlertDTO> alerts;
    
    // 统计
    int32_t totalAlerts;                // 总告警数
    int32_t unacknowledgedCount;        // 未确认数
    int32_t boardAlertCount;            // 板卡告警数
    int32_t componentAlertCount;        // 组件告警数
};

// ==================== 命令相关DTOs ====================

/**
 * @brief Deploy/Undeploy命令DTO
 */
struct DeployCommandDTO {
    std::vector<std::string> stackLabels;  // 业务链路标签UUID列表
};

/**
 * @brief Deploy/Undeploy结果DTO
 */
struct DeployResultDTO {
    struct StackResult {
        std::string stackName;
        std::string stackUUID;
        std::string message;
    };
    
    std::vector<StackResult> successStacks;  // 成功的业务链路
    std::vector<StackResult> failureStacks;  // 失败的业务链路
    
    int32_t totalCount;                      // 总数
    int32_t successCount;                    // 成功数
    int32_t failureCount;                    // 失败数
};

/**
 * @brief 任务资源查询命令DTO
 */
struct TaskResourceQueryDTO {
    std::string taskID;                 // 任务ID
};

/**
 * @brief 告警确认命令DTO
 */
struct AlertAcknowledgeDTO {
    std::vector<std::string> alertUUIDs;  // 告警UUID列表
};

// ==================== 响应包装 ====================

/**
 * @brief 通用响应DTO
 */
template<typename T>
struct ResponseDTO {
    bool success;                       // 是否成功
    std::string message;                // 消息
    T data;                            // 数据
    int32_t errorCode;                  // 错误码（0表示成功）
    
    // 便捷构造函数
    static ResponseDTO<T> Success(const T& data, const std::string& msg = "success") {
        return ResponseDTO<T>{true, msg, data, 0};
    }
    
    static ResponseDTO<T> Failure(const std::string& msg, int32_t code = -1) {
        return ResponseDTO<T>{false, msg, T{}, code};
    }
};

} // namespace zygl::application

