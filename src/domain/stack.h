#pragma once

#include "service.h"
#include "value_objects.h"
#include <string>
#include <map>
#include <vector>
#include <optional>
#include <array>

namespace zygl::domain {

// 常量定义
constexpr int MAX_LABELS_PER_STACK = 8;  // 每个业务链路最多8个标签

/**
 * @brief Stack聚合根 - 业务链路
 * 
 * Stack是顶层业务单元，由一个或多个Service（算法组件）组成。
 * Stack可以通过标签（StackLabel）进行批量管理（启用/停用）。
 * 
 * 作为聚合根，Stack负责维护其内部所有Service和Task的一致性，
 * 并根据子实体的状态计算自身的运行状态。
 */
class Stack {
public:
    /**
     * @brief 构造函数
     * @param stackUUID 业务链路唯一标识
     * @param stackName 业务链路名称
     */
    Stack(const std::string& stackUUID, const std::string& stackName)
        : m_stackUUID(stackUUID),
          m_stackName(stackName),
          m_deployStatus(StackDeployStatus::Undeployed),
          m_runningStatus(StackRunningStatus::Normal),
          m_labelCount(0) {
        std::memset(m_labels.data(), 0, sizeof(StackLabelInfo) * MAX_LABELS_PER_STACK);
    }

    Stack()
        : m_stackUUID(""),
          m_stackName(""),
          m_deployStatus(StackDeployStatus::Undeployed),
          m_runningStatus(StackRunningStatus::Normal),
          m_labelCount(0) {
        std::memset(m_labels.data(), 0, sizeof(StackLabelInfo) * MAX_LABELS_PER_STACK);
    }

    // ==================== 访问器（Getters） ====================

    const std::string& GetStackUUID() const { return m_stackUUID; }
    const std::string& GetStackName() const { return m_stackName; }
    StackDeployStatus GetDeployStatus() const { return m_deployStatus; }
    StackRunningStatus GetRunningStatus() const { return m_runningStatus; }
    int32_t GetLabelCount() const { return m_labelCount; }
    
    const std::array<StackLabelInfo, MAX_LABELS_PER_STACK>& GetLabels() const {
        return m_labels;
    }

    const std::map<std::string, Service>& GetAllServices() const {
        return m_services;
    }

    // ==================== 修改器（Setters） ====================

    void SetStackName(const std::string& name) {
        m_stackName = name;
    }

    void SetDeployStatus(StackDeployStatus status) {
        m_deployStatus = status;
    }

    void SetRunningStatus(StackRunningStatus status) {
        m_runningStatus = status;
    }

    // ==================== 业务逻辑方法 ====================

    /**
     * @brief 添加或更新一个算法组件
     * @param service 组件实例
     */
    void AddOrUpdateService(const Service& service) {
        m_services[service.GetServiceUUID()] = service;
    }

    /**
     * @brief 根据组件UUID查找组件
     * @param serviceUUID 组件UUID
     * @return 组件实例，如果未找到返回std::nullopt
     */
    std::optional<Service> FindService(const std::string& serviceUUID) const {
        auto it = m_services.find(serviceUUID);
        if (it != m_services.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    /**
     * @brief 根据组件UUID获取组件的可变引用
     * @param serviceUUID 组件UUID
     * @return 组件指针，如果未找到返回nullptr
     */
    Service* GetServiceByUUID(const std::string& serviceUUID) {
        auto it = m_services.find(serviceUUID);
        if (it != m_services.end()) {
            return &(it->second);
        }
        return nullptr;
    }

    /**
     * @brief 移除一个组件
     * @param serviceUUID 组件UUID
     * @return true 如果组件被移除，false 如果组件不存在
     */
    bool RemoveService(const std::string& serviceUUID) {
        return m_services.erase(serviceUUID) > 0;
    }

    /**
     * @brief 根据任务ID查找任务的资源使用情况
     * 
     * 这是为"方案B（按需查询）"设计的核心方法。
     * 遍历所有组件及其任务，查找指定的任务。
     * 
     * @param taskID 任务ID
     * @return 资源使用情况，如果未找到返回std::nullopt
     */
    std::optional<ResourceUsage> GetTaskResources(const std::string& taskID) const {
        for (const auto& [serviceUUID, service] : m_services) {
            auto taskOpt = service.FindTask(taskID);
            if (taskOpt.has_value()) {
                return taskOpt->GetResources();
            }
        }
        return std::nullopt;
    }

    /**
     * @brief 根据任务ID查找任务
     * @param taskID 任务ID
     * @return 任务实例，如果未找到返回std::nullopt
     */
    std::optional<Task> FindTask(const std::string& taskID) const {
        for (const auto& [serviceUUID, service] : m_services) {
            auto taskOpt = service.FindTask(taskID);
            if (taskOpt.has_value()) {
                return taskOpt;
            }
        }
        return std::nullopt;
    }

    /**
     * @brief 添加标签
     * @param label 标签信息
     * @return true 如果添加成功，false 如果已满
     */
    bool AddLabel(const StackLabelInfo& label) {
        if (m_labelCount >= MAX_LABELS_PER_STACK) {
            return false;
        }
        m_labels[m_labelCount++] = label;
        return true;
    }

    /**
     * @brief 清除所有标签
     */
    void ClearLabels() {
        m_labelCount = 0;
        std::memset(m_labels.data(), 0, sizeof(StackLabelInfo) * MAX_LABELS_PER_STACK);
    }

    /**
     * @brief 检查是否包含指定标签UUID
     * @param labelUUID 标签UUID
     * @return true 如果包含此标签
     */
    bool HasLabel(const std::string& labelUUID) const {
        for (int32_t i = 0; i < m_labelCount; ++i) {
            if (std::strcmp(m_labels[i].labelUUID, labelUUID.c_str()) == 0) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief 检查业务链路是否已部署
     */
    bool IsDeployed() const {
        return m_deployStatus == StackDeployStatus::Deployed;
    }

    /**
     * @brief 检查业务链路是否正常运行
     */
    bool IsRunningNormally() const {
        return m_runningStatus == StackRunningStatus::Normal;
    }

    /**
     * @brief 根据所有组件和任务的状态重新计算业务链路的运行状态
     * 
     * 业务规则：
     * - 如果任何组件处于异常状态，业务链路状态为Abnormal
     * - 如果所有组件都正常，业务链路状态为Normal
     */
    void RecalculateRunningStatus() {
        if (m_services.empty()) {
            m_runningStatus = StackRunningStatus::Normal;
            return;
        }

        for (const auto& [serviceUUID, service] : m_services) {
            if (service.IsAbnormal()) {
                m_runningStatus = StackRunningStatus::Abnormal;
                return;
            }
        }

        // 所有组件都正常
        m_runningStatus = StackRunningStatus::Normal;
    }

    /**
     * @brief 获取组件数量
     */
    size_t GetServiceCount() const {
        return m_services.size();
    }

    /**
     * @brief 获取任务总数
     */
    size_t GetTotalTaskCount() const {
        size_t count = 0;
        for (const auto& [serviceUUID, service] : m_services) {
            count += service.GetTaskCount();
        }
        return count;
    }

    /**
     * @brief 计算业务链路的总资源使用情况
     * 
     * 聚合所有组件的资源使用。
     * 
     * @return 聚合后的资源使用情况
     */
    ResourceUsage CalculateTotalResources() const {
        ResourceUsage total;
        std::memset(&total, 0, sizeof(ResourceUsage));

        for (const auto& [serviceUUID, service] : m_services) {
            ResourceUsage serviceRes = service.CalculateTotalResources();
            total.cpuCores += serviceRes.cpuCores;
            total.cpuUsed += serviceRes.cpuUsed;
            total.memorySize += serviceRes.memorySize;
            total.memoryUsed += serviceRes.memoryUsed;
            total.netReceive += serviceRes.netReceive;
            total.netSent += serviceRes.netSent;
            total.gpuMemUsed += serviceRes.gpuMemUsed;
        }

        // 计算使用率
        if (total.cpuCores > 0) {
            total.cpuUsage = (total.cpuUsed / total.cpuCores) * 100.0f;
        }
        if (total.memorySize > 0) {
            total.memoryUsage = (total.memoryUsed / total.memorySize) * 100.0f;
        }

        return total;
    }

    /**
     * @brief 获取所有组件的UUID列表
     */
    std::vector<std::string> GetServiceUUIDs() const {
        std::vector<std::string> uuids;
        uuids.reserve(m_services.size());
        for (const auto& [uuid, service] : m_services) {
            uuids.push_back(uuid);
        }
        return uuids;
    }

private:
    // ==================== 基本信息 ====================
    std::string m_stackUUID;                                    // 业务链路UUID
    std::string m_stackName;                                    // 业务链路名称
    StackDeployStatus m_deployStatus;                           // 部署状态
    StackRunningStatus m_runningStatus;                         // 运行状态

    // ==================== 标签信息 ====================
    std::array<StackLabelInfo, MAX_LABELS_PER_STACK> m_labels; // 标签列表
    int32_t m_labelCount;                                       // 有效标签数量

    // ==================== 子实体集合 ====================
    std::map<std::string, Service> m_services;                  // 组件集合（Key: ServiceUUID）
};

} // namespace zygl::domain

