#pragma once

#include "task.h"
#include "value_objects.h"
#include <string>
#include <map>
#include <optional>
#include <vector>

namespace zygl::domain {

/**
 * @brief Service实体 - 算法组件
 * 
 * Service是业务链路的组成部分，包含一个或多个Task实例。
 * Service有自己的状态（已停用、已启用、运行正常、运行异常）和类型。
 * 
 * 注意：此类用于内部状态管理，可以使用std::map和std::string。
 */
class Service {
public:
    /**
     * @brief 构造函数
     * @param serviceUUID 组件唯一标识
     * @param serviceName 组件名称
     */
    Service(const std::string& serviceUUID, const std::string& serviceName)
        : m_serviceUUID(serviceUUID),
          m_serviceName(serviceName),
          m_status(ServiceStatus::Disabled),
          m_type(ServiceType::Normal) {
    }

    Service()
        : m_serviceUUID(""),
          m_serviceName(""),
          m_status(ServiceStatus::Disabled),
          m_type(ServiceType::Normal) {
    }

    // ==================== 访问器（Getters） ====================

    const std::string& GetServiceUUID() const { return m_serviceUUID; }
    const std::string& GetServiceName() const { return m_serviceName; }
    ServiceStatus GetStatus() const { return m_status; }
    ServiceType GetType() const { return m_type; }
    
    const std::map<std::string, Task>& GetAllTasks() const {
        return m_tasks;
    }

    // ==================== 修改器（Setters） ====================

    void SetStatus(ServiceStatus status) {
        m_status = status;
    }

    void SetType(ServiceType type) {
        m_type = type;
    }

    void SetServiceName(const std::string& name) {
        m_serviceName = name;
    }

    // ==================== 业务逻辑方法 ====================

    /**
     * @brief 添加或更新一个任务
     * @param task 任务实例
     */
    void AddOrUpdateTask(const Task& task) {
        m_tasks[task.GetTaskID()] = task;
    }

    /**
     * @brief 根据任务ID查找任务
     * @param taskID 任务ID
     * @return 任务实例，如果未找到返回std::nullopt
     */
    std::optional<Task> FindTask(const std::string& taskID) const {
        auto it = m_tasks.find(taskID);
        if (it != m_tasks.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    /**
     * @brief 根据任务ID获取任务的可变引用
     * @param taskID 任务ID
     * @return 任务指针，如果未找到返回nullptr
     */
    Task* GetTaskByID(const std::string& taskID) {
        auto it = m_tasks.find(taskID);
        if (it != m_tasks.end()) {
            return &(it->second);
        }
        return nullptr;
    }

    /**
     * @brief 移除一个任务
     * @param taskID 任务ID
     * @return true 如果任务被移除，false 如果任务不存在
     */
    bool RemoveTask(const std::string& taskID) {
        return m_tasks.erase(taskID) > 0;
    }

    /**
     * @brief 获取任务数量
     */
    size_t GetTaskCount() const {
        return m_tasks.size();
    }

    /**
     * @brief 检查组件是否正常运行
     */
    bool IsRunning() const {
        return m_status == ServiceStatus::Running;
    }

    /**
     * @brief 检查组件是否异常
     */
    bool IsAbnormal() const {
        return m_status == ServiceStatus::Abnormal;
    }

    /**
     * @brief 计算组件的整体资源使用情况
     * 
     * 聚合所有任务的资源使用，计算总和。
     * 
     * @return 聚合后的资源使用情况
     */
    ResourceUsage CalculateTotalResources() const {
        ResourceUsage total;
        std::memset(&total, 0, sizeof(ResourceUsage));

        for (const auto& [taskID, task] : m_tasks) {
            const auto& res = task.GetResources();
            total.cpuCores += res.cpuCores;
            total.cpuUsed += res.cpuUsed;
            total.memorySize += res.memorySize;
            total.memoryUsed += res.memoryUsed;
            total.netReceive += res.netReceive;
            total.netSent += res.netSent;
            total.gpuMemUsed += res.gpuMemUsed;
        }

        // 计算平均使用率
        if (!m_tasks.empty()) {
            float taskCount = static_cast<float>(m_tasks.size());
            total.cpuUsage = (total.cpuCores > 0) ? (total.cpuUsed / total.cpuCores) * 100.0f : 0.0f;
            total.memoryUsage = (total.memorySize > 0) ? (total.memoryUsed / total.memorySize) * 100.0f : 0.0f;
        }

        return total;
    }

    /**
     * @brief 根据所有任务的状态重新计算组件状态
     * 
     * 业务规则：
     * - 如果所有任务都正常运行，组件状态为Running
     * - 如果任何任务异常，组件状态为Abnormal
     * - 如果没有任务，保持当前状态
     */
    void RecalculateStatus() {
        if (m_tasks.empty()) {
            return;  // 保持当前状态
        }

        bool allRunning = true;
        bool hasAbnormal = false;

        for (const auto& [taskID, task] : m_tasks) {
            if (!task.IsRunning()) {
                allRunning = false;
                hasAbnormal = true;
                break;
            }
        }

        if (allRunning) {
            m_status = ServiceStatus::Running;
        } else if (hasAbnormal) {
            m_status = ServiceStatus::Abnormal;
        }
    }

    /**
     * @brief 获取所有任务的ID列表
     */
    std::vector<std::string> GetTaskIDs() const {
        std::vector<std::string> ids;
        ids.reserve(m_tasks.size());
        for (const auto& [taskID, task] : m_tasks) {
            ids.push_back(taskID);
        }
        return ids;
    }

private:
    // ==================== 基本信息 ====================
    std::string m_serviceUUID;              // 组件UUID（唯一标识）
    std::string m_serviceName;              // 组件名称
    ServiceStatus m_status;                 // 组件状态
    ServiceType m_type;                     // 组件类型

    // ==================== 子实体集合 ====================
    std::map<std::string, Task> m_tasks;    // 任务集合（Key: TaskID）
};

} // namespace zygl::domain

