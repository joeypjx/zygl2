#pragma once

#include "value_objects.h"
#include <string>
#include <cstring>

namespace zygl::domain {

/**
 * @brief Task实体 - 任务
 * 
 * Task是算法组件的运行实例（类似容器），包含详细的资源使用情况。
 * Task运行在特定的板卡上，并消耗CPU、内存、网络和GPU资源。
 * 
 * 注意：此类用于内部状态管理（stackinfo数据），可以使用std::string。
 * 与UDP传输相关的任务信息使用TaskStatusInfo值对象。
 */
class Task {
public:
    /**
     * @brief 构造函数
     * @param taskID 任务唯一标识
     */
    explicit Task(const std::string& taskID)
        : m_taskID(taskID),
          m_taskStatus(""),
          m_boardAddress("") {
        std::memset(&m_resources, 0, sizeof(ResourceUsage));
        std::memset(&m_location, 0, sizeof(LocationInfo));
    }

    Task() : m_taskID(""), m_taskStatus(""), m_boardAddress("") {
        std::memset(&m_resources, 0, sizeof(ResourceUsage));
        std::memset(&m_location, 0, sizeof(LocationInfo));
    }

    // ==================== 访问器（Getters） ====================

    const std::string& GetTaskID() const { return m_taskID; }
    const std::string& GetTaskStatus() const { return m_taskStatus; }
    const std::string& GetBoardAddress() const { return m_boardAddress; }
    const ResourceUsage& GetResources() const { return m_resources; }
    const LocationInfo& GetLocation() const { return m_location; }

    // ==================== 修改器（Setters） ====================

    void SetTaskStatus(const std::string& status) {
        m_taskStatus = status;
    }

    void SetBoardAddress(const std::string& address) {
        m_boardAddress = address;
    }

    // ==================== 业务逻辑方法 ====================

    /**
     * @brief 更新任务的资源使用情况
     * @param resources 新的资源使用数据
     */
    void UpdateResources(const ResourceUsage& resources) {
        m_resources = resources;
    }

    /**
     * @brief 更新任务的位置信息
     * @param location 新的位置信息
     */
    void UpdateLocation(const LocationInfo& location) {
        m_location = location;
        
        // 同步板卡地址
        if (location.boardAddress[0] != '\0') {
            m_boardAddress = location.boardAddress;
        }
    }

    /**
     * @brief 检查任务是否正常运行
     * @return true 如果任务状态表示正常
     */
    bool IsRunning() const {
        // 根据实际业务逻辑判断任务状态
        // 这里简单判断状态字符串不为空
        return !m_taskStatus.empty() && m_taskStatus != "stopped" && m_taskStatus != "failed";
    }

    /**
     * @brief 获取CPU使用率
     * @return CPU使用率百分比
     */
    float GetCpuUsagePercent() const {
        return m_resources.cpuUsage;
    }

    /**
     * @brief 获取内存使用率
     * @return 内存使用率百分比
     */
    float GetMemoryUsagePercent() const {
        return m_resources.memoryUsage;
    }

    /**
     * @brief 判断任务是否资源使用过高
     * @param cpuThreshold CPU阈值（百分比）
     * @param memThreshold 内存阈值（百分比）
     * @return true 如果任何资源超过阈值
     */
    bool IsResourceOverloaded(float cpuThreshold = 90.0f, float memThreshold = 90.0f) const {
        return m_resources.cpuUsage > cpuThreshold || 
               m_resources.memoryUsage > memThreshold;
    }

private:
    // ==================== 基本信息 ====================
    std::string m_taskID;               // 任务ID（唯一标识）
    std::string m_taskStatus;           // 任务状态
    std::string m_boardAddress;         // 运行的板卡地址（引用Board聚合根的ID）

    // ==================== 资源和位置信息 ====================
    ResourceUsage m_resources;          // 资源使用情况
    LocationInfo m_location;            // 运行位置信息
};

} // namespace zygl::domain

