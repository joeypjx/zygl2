#pragma once

#include "value_objects.h"
#include <string>
#include <vector>
#include <cstring>
#include <chrono>

namespace zygl::domain {

// 常量定义
constexpr int MAX_ALERT_MESSAGES = 16;  // 每个告警最多16条消息

#pragma pack(push, 1)

/**
 * @brief Alert聚合根 - 告警
 * 
 * Alert记录系统中的异常事件，包括板卡异常和组件异常。
 * 告警包含详细的上下文信息（什么、在哪里、何时）。
 * 
 * 告警类型：
 * - 板卡异常：包含板卡位置信息
 * - 组件异常：包含业务链路、组件、任务信息
 */
class Alert {
public:
    /**
     * @brief 构造函数
     * @param alertUUID 告警唯一标识（系统生成）
     * @param type 告警类型
     */
    Alert(const char* alertUUID, AlertType type)
        : m_alertType(type),
          m_timestamp(0),
          m_isAcknowledged(false),
          m_messageCount(0) {
        SetAlertUUID(alertUUID);
        std::memset(m_relatedEntity, 0, sizeof(m_relatedEntity));
        std::memset(m_messages.data(), 0, sizeof(AlertMessage) * MAX_ALERT_MESSAGES);
        std::memset(&m_location, 0, sizeof(LocationInfo));
        std::memset(m_stackName, 0, sizeof(m_stackName));
        std::memset(m_stackUUID, 0, sizeof(m_stackUUID));
        std::memset(m_serviceName, 0, sizeof(m_serviceName));
        std::memset(m_serviceUUID, 0, sizeof(m_serviceUUID));
        std::memset(m_taskID, 0, sizeof(m_taskID));
    }

    Alert()
        : m_alertType(AlertType::Board),
          m_timestamp(0),
          m_isAcknowledged(false),
          m_messageCount(0) {
        std::memset(m_alertUUID, 0, sizeof(m_alertUUID));
        std::memset(m_relatedEntity, 0, sizeof(m_relatedEntity));
        std::memset(m_messages.data(), 0, sizeof(AlertMessage) * MAX_ALERT_MESSAGES);
        std::memset(&m_location, 0, sizeof(LocationInfo));
        std::memset(m_stackName, 0, sizeof(m_stackName));
        std::memset(m_stackUUID, 0, sizeof(m_stackUUID));
        std::memset(m_serviceName, 0, sizeof(m_serviceName));
        std::memset(m_serviceUUID, 0, sizeof(m_serviceUUID));
        std::memset(m_taskID, 0, sizeof(m_taskID));
    }

    // ==================== 访问器（Getters） ====================

    const char* GetAlertUUID() const { return m_alertUUID; }
    AlertType GetAlertType() const { return m_alertType; }
    uint64_t GetTimestamp() const { return m_timestamp; }
    bool IsAcknowledged() const { return m_isAcknowledged; }
    const char* GetRelatedEntity() const { return m_relatedEntity; }
    int32_t GetMessageCount() const { return m_messageCount; }
    
    const std::array<AlertMessage, MAX_ALERT_MESSAGES>& GetMessages() const {
        return m_messages;
    }

    const LocationInfo& GetLocation() const { return m_location; }
    const char* GetStackName() const { return m_stackName; }
    const char* GetStackUUID() const { return m_stackUUID; }
    const char* GetServiceName() const { return m_serviceName; }
    const char* GetServiceUUID() const { return m_serviceUUID; }
    const char* GetTaskID() const { return m_taskID; }

    // ==================== 业务逻辑方法 ====================

    /**
     * @brief 创建板卡异常告警
     * @param alertUUID 告警UUID
     * @param location 板卡位置信息
     * @param messages 告警消息列表
     * @return Alert实例
     */
    static Alert CreateBoardAlert(const char* alertUUID,
                                  const LocationInfo& location,
                                  const std::vector<std::string>& messages) {
        Alert alert(alertUUID, AlertType::Board);
        alert.SetTimestamp(GetCurrentTimestamp());
        alert.SetLocation(location);
        alert.SetRelatedEntity(location.boardAddress);
        
        // 添加消息
        for (const auto& msg : messages) {
            if (!alert.AddMessage(msg.c_str())) {
                break;  // 消息已满
            }
        }
        
        return alert;
    }

    /**
     * @brief 创建组件异常告警
     * @param alertUUID 告警UUID
     * @param stackName 业务链路名称
     * @param stackUUID 业务链路UUID
     * @param serviceName 组件名称
     * @param serviceUUID 组件UUID
     * @param taskID 任务ID
     * @param location 任务运行位置
     * @param messages 告警消息列表
     * @return Alert实例
     */
    static Alert CreateComponentAlert(const char* alertUUID,
                                      const char* stackName,
                                      const char* stackUUID,
                                      const char* serviceName,
                                      const char* serviceUUID,
                                      const char* taskID,
                                      const LocationInfo& location,
                                      const std::vector<std::string>& messages) {
        Alert alert(alertUUID, AlertType::Component);
        alert.SetTimestamp(GetCurrentTimestamp());
        alert.SetStackInfo(stackName, stackUUID);
        alert.SetServiceInfo(serviceName, serviceUUID);
        alert.SetTaskID(taskID);
        alert.SetLocation(location);
        alert.SetRelatedEntity(taskID);
        
        // 添加消息
        for (const auto& msg : messages) {
            if (!alert.AddMessage(msg.c_str())) {
                break;  // 消息已满
            }
        }
        
        return alert;
    }

    /**
     * @brief 添加告警消息
     * @param message 消息内容
     * @return true 如果添加成功，false 如果已满
     */
    bool AddMessage(const char* message) {
        if (m_messageCount >= MAX_ALERT_MESSAGES) {
            return false;
        }
        
        m_messages[m_messageCount].SetMessage(message);
        m_messages[m_messageCount].timestamp = GetCurrentTimestamp();
        m_messageCount++;
        return true;
    }

    /**
     * @brief 确认告警
     * 
     * 标记告警已被运维人员查看和处理。
     */
    void Acknowledge() {
        m_isAcknowledged = true;
    }

    /**
     * @brief 取消确认
     */
    void Unacknowledge() {
        m_isAcknowledged = false;
    }

    /**
     * @brief 检查告警是否为板卡异常
     */
    bool IsBoardAlert() const {
        return m_alertType == AlertType::Board;
    }

    /**
     * @brief 检查告警是否为组件异常
     */
    bool IsComponentAlert() const {
        return m_alertType == AlertType::Component;
    }

    /**
     * @brief 获取告警年龄（秒）
     * @return 从告警产生到现在的秒数
     */
    uint64_t GetAgeInSeconds() const {
        uint64_t now = GetCurrentTimestamp();
        return (now > m_timestamp) ? (now - m_timestamp) : 0;
    }

    // ==================== 修改器（Setters） ====================

    void SetAlertUUID(const char* uuid) {
        std::strncpy(m_alertUUID, uuid, sizeof(m_alertUUID) - 1);
        m_alertUUID[sizeof(m_alertUUID) - 1] = '\0';
    }

    void SetTimestamp(uint64_t timestamp) {
        m_timestamp = timestamp;
    }

    void SetRelatedEntity(const char* entity) {
        std::strncpy(m_relatedEntity, entity, sizeof(m_relatedEntity) - 1);
        m_relatedEntity[sizeof(m_relatedEntity) - 1] = '\0';
    }

    void SetLocation(const LocationInfo& location) {
        m_location = location;
    }

    void SetStackInfo(const char* name, const char* uuid) {
        std::strncpy(m_stackName, name, sizeof(m_stackName) - 1);
        m_stackName[sizeof(m_stackName) - 1] = '\0';
        
        std::strncpy(m_stackUUID, uuid, sizeof(m_stackUUID) - 1);
        m_stackUUID[sizeof(m_stackUUID) - 1] = '\0';
    }

    void SetServiceInfo(const char* name, const char* uuid) {
        std::strncpy(m_serviceName, name, sizeof(m_serviceName) - 1);
        m_serviceName[sizeof(m_serviceName) - 1] = '\0';
        
        std::strncpy(m_serviceUUID, uuid, sizeof(m_serviceUUID) - 1);
        m_serviceUUID[sizeof(m_serviceUUID) - 1] = '\0';
    }

    void SetTaskID(const char* taskID) {
        std::strncpy(m_taskID, taskID, sizeof(m_taskID) - 1);
        m_taskID[sizeof(m_taskID) - 1] = '\0';
    }

private:
    /**
     * @brief 获取当前时间戳（Unix时间，秒）
     */
    static uint64_t GetCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto duration = now.time_since_epoch();
        return std::chrono::duration_cast<std::chrono::seconds>(duration).count();
    }

    // ==================== 基本信息 ====================
    char m_alertUUID[64];                               // 告警UUID
    AlertType m_alertType;                              // 告警类型
    uint64_t m_timestamp;                               // 告警时间戳
    bool m_isAcknowledged;                              // 是否已确认
    char m_relatedEntity[64];                           // 相关实体（BoardAddress 或 TaskID）

    // ==================== 告警消息 ====================
    std::array<AlertMessage, MAX_ALERT_MESSAGES> m_messages;  // 消息列表
    int32_t m_messageCount;                             // 有效消息数量

    // ==================== 位置信息（通用） ====================
    LocationInfo m_location;                            // 位置信息

    // ==================== 组件告警专用字段 ====================
    char m_stackName[128];                              // 业务链路名称
    char m_stackUUID[64];                               // 业务链路UUID
    char m_serviceName[128];                            // 组件名称
    char m_serviceUUID[64];                             // 组件UUID
    char m_taskID[64];                                  // 任务ID
};

#pragma pack(pop)

} // namespace zygl::domain

