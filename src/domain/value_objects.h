#pragma once

#include <cstdint>
#include <cstring>

namespace zygl::domain {

// 板卡类型枚举
enum class BoardType : int32_t {
    Computing = 0,  // 计算板卡（可运行任务）
    Switch = 1,     // 交换板卡（槽位6、7）
    Power = 2       // 电源板卡（槽位13、14）
};

// 板卡运行状态枚举
enum class BoardOperationalStatus : int32_t {
    Unknown = -1,   // 未知状态（启动时初始状态）
    Normal = 0,     // 正常（来自API的0值）
    Abnormal = 1,   // 异常（来自API的1值）
    Offline = 2     // 离线（API中未上报此板卡）
};

// 业务链路部署状态枚举
enum class StackDeployStatus : int32_t {
    Undeployed = 0, // 未部署
    Deployed = 1    // 已部署
};

// 业务链路运行状态枚举
enum class StackRunningStatus : int32_t {
    Normal = 1,     // 正常运行
    Abnormal = 2    // 异常运行
};

// 算法组件状态枚举
enum class ServiceStatus : int32_t {
    Disabled = 0,   // 已停用
    Enabled = 1,    // 已启用
    Running = 2,    // 运行正常
    Abnormal = 3    // 运行异常
};

// 算法组件类型枚举
enum class ServiceType : int32_t {
    Normal = 0,         // 普通组件
    SharedReference = 1, // 普通链路引用的公共组件
    SharedOwned = 2     // 公共链路自有组件
};

// 告警类型枚举
enum class AlertType : int32_t {
    Board = 0,      // 板卡异常
    Component = 1   // 组件异常
};

// ==================== 固定大小的值对象（用于UDP传输） ====================

#pragma pack(push, 1)  // 强制1字节对齐，确保跨平台兼容性

// 任务状态信息（用于板卡信息接口）
struct TaskStatusInfo {
    char taskID[64];            // 任务ID
    char taskStatus[32];        // 任务状态
    char serviceName[128];      // 算法组件名称
    char serviceUUID[64];       // 算法组件UUID
    char stackName[128];        // 业务链路名称
    char stackUUID[64];         // 业务链路UUID

    TaskStatusInfo() {
        std::memset(this, 0, sizeof(TaskStatusInfo));
    }

    void SetTaskID(const char* id) {
        std::strncpy(taskID, id, sizeof(taskID) - 1);
        taskID[sizeof(taskID) - 1] = '\0';
    }

    void SetTaskStatus(const char* status) {
        std::strncpy(taskStatus, status, sizeof(taskStatus) - 1);
        taskStatus[sizeof(taskStatus) - 1] = '\0';
    }

    void SetServiceName(const char* name) {
        std::strncpy(serviceName, name, sizeof(serviceName) - 1);
        serviceName[sizeof(serviceName) - 1] = '\0';
    }

    void SetServiceUUID(const char* uuid) {
        std::strncpy(serviceUUID, uuid, sizeof(serviceUUID) - 1);
        serviceUUID[sizeof(serviceUUID) - 1] = '\0';
    }

    void SetStackName(const char* name) {
        std::strncpy(stackName, name, sizeof(stackName) - 1);
        stackName[sizeof(stackName) - 1] = '\0';
    }

    void SetStackUUID(const char* uuid) {
        std::strncpy(stackUUID, uuid, sizeof(stackUUID) - 1);
        stackUUID[sizeof(stackUUID) - 1] = '\0';
    }
};

// 位置信息（机箱、板卡位置）
struct LocationInfo {
    char chassisName[64];       // 机箱名称
    int32_t chassisNumber;      // 机箱号
    char boardName[64];         // 板卡名称
    int32_t boardNumber;        // 板卡槽位号
    char boardAddress[16];      // 板卡IP地址（IPv4）

    LocationInfo() {
        std::memset(this, 0, sizeof(LocationInfo));
    }

    void SetChassisName(const char* name) {
        std::strncpy(chassisName, name, sizeof(chassisName) - 1);
        chassisName[sizeof(chassisName) - 1] = '\0';
    }

    void SetBoardName(const char* name) {
        std::strncpy(boardName, name, sizeof(boardName) - 1);
        boardName[sizeof(boardName) - 1] = '\0';
    }

    void SetBoardAddress(const char* addr) {
        std::strncpy(boardAddress, addr, sizeof(boardAddress) - 1);
        boardAddress[sizeof(boardAddress) - 1] = '\0';
    }
};

// 业务链路标签信息
struct StackLabelInfo {
    char labelName[128];        // 标签名称
    char labelUUID[64];         // 标签UUID

    StackLabelInfo() {
        std::memset(this, 0, sizeof(StackLabelInfo));
    }

    void SetLabelName(const char* name) {
        std::strncpy(labelName, name, sizeof(labelName) - 1);
        labelName[sizeof(labelName) - 1] = '\0';
    }

    void SetLabelUUID(const char* uuid) {
        std::strncpy(labelUUID, uuid, sizeof(labelUUID) - 1);
        labelUUID[sizeof(labelUUID) - 1] = '\0';
    }
};

// 资源使用情况（用于任务详情）
struct ResourceUsage {
    float cpuCores;             // CPU总量（核心数）
    float cpuUsed;              // CPU使用量
    float cpuUsage;             // CPU使用率（百分比）
    float memorySize;           // 内存总量（字节）
    float memoryUsed;           // 内存使用量（字节）
    float memoryUsage;          // 内存使用率（百分比）
    float netReceive;           // 网络接收流量
    float netSent;              // 网络发送流量
    float gpuMemUsed;           // GPU显存使用情况

    ResourceUsage() {
        std::memset(this, 0, sizeof(ResourceUsage));
    }
};

// 告警消息
struct AlertMessage {
    char message[256];          // 告警消息内容
    uint64_t timestamp;         // 时间戳（Unix时间）

    AlertMessage() {
        std::memset(this, 0, sizeof(AlertMessage));
    }

    void SetMessage(const char* msg) {
        std::strncpy(message, msg, sizeof(message) - 1);
        message[sizeof(message) - 1] = '\0';
    }
};

#pragma pack(pop)  // 恢复默认对齐

} // namespace zygl::domain

