#pragma once

#include "../../domain/i_stack_repository.h"
#include "../../domain/stack.h"
#include <map>
#include <vector>
#include <optional>
#include <mutex>
#include <shared_mutex>
#include <string>

namespace zygl::infrastructure {

/**
 * @brief InMemoryStackRepository - 业务链路仓储的内存实现
 * 
 * 实现要点：
 * 1. 使用std::map存储Stack聚合根（Key: stackUUID）
 * 2. 使用std::shared_mutex实现读写锁
 * 3. 多个读取者可以并发读取，写入者独占访问
 * 
 * 线程安全：
 * - 读取操作使用std::shared_lock（共享锁）
 * - 写入操作使用std::unique_lock（独占锁）
 * - 支持多读单写
 */
class InMemoryStackRepository : public domain::IStackRepository {
public:
    InMemoryStackRepository() = default;
    
    // 禁止拷贝和移动
    InMemoryStackRepository(const InMemoryStackRepository&) = delete;
    InMemoryStackRepository& operator=(const InMemoryStackRepository&) = delete;

    /**
     * @brief 保存一个业务链路
     * 
     * 如果UUID已存在则更新，否则插入。
     */
    void Save(const domain::Stack& stack) override {
        std::unique_lock lock(m_mutex);  // 写锁
        m_stacks[stack.GetStackUUID()] = stack;
    }

    /**
     * @brief 批量保存多个业务链路
     * 
     * 用于DataCollector从API拉取数据后批量更新。
     */
    void SaveAll(const std::vector<domain::Stack>& stacks) override {
        std::unique_lock lock(m_mutex);  // 写锁
        
        for (const auto& stack : stacks) {
            m_stacks[stack.GetStackUUID()] = stack;
        }
    }

    /**
     * @brief 根据UUID查找业务链路
     */
    std::optional<domain::Stack> FindByUUID(const std::string& stackUUID) const override {
        std::shared_lock lock(m_mutex);  // 读锁
        
        auto it = m_stacks.find(stackUUID);
        if (it != m_stacks.end()) {
            return it->second;
        }
        
        return std::nullopt;
    }

    /**
     * @brief 获取所有业务链路
     */
    std::vector<domain::Stack> GetAll() const override {
        std::shared_lock lock(m_mutex);  // 读锁
        
        std::vector<domain::Stack> result;
        result.reserve(m_stacks.size());
        
        for (const auto& [uuid, stack] : m_stacks) {
            result.push_back(stack);
        }
        
        return result;
    }

    /**
     * @brief 根据标签UUID查找包含该标签的所有业务链路
     * 
     * 核心方法：用于deploy/undeploy命令，根据标签批量操作。
     */
    std::vector<domain::Stack> FindByLabel(const std::string& labelUUID) const override {
        std::shared_lock lock(m_mutex);  // 读锁
        
        std::vector<domain::Stack> result;
        
        for (const auto& [uuid, stack] : m_stacks) {
            if (stack.HasLabel(labelUUID)) {
                result.push_back(stack);
            }
        }
        
        return result;
    }

    /**
     * @brief 根据任务ID查找任务的资源使用情况
     * 
     * 核心方法："方案B（按需查询）"的实现。
     * 遍历所有业务链路、组件和任务，查找指定的任务。
     */
    std::optional<domain::ResourceUsage> FindTaskResources(const std::string& taskID) const override {
        std::shared_lock lock(m_mutex);  // 读锁
        
        for (const auto& [uuid, stack] : m_stacks) {
            auto resources = stack.GetTaskResources(taskID);
            if (resources.has_value()) {
                return resources;
            }
        }
        
        return std::nullopt;
    }

    /**
     * @brief 根据任务ID查找任务所属的业务链路
     */
    std::optional<domain::Stack> FindStackByTaskID(const std::string& taskID) const override {
        std::shared_lock lock(m_mutex);  // 读锁
        
        for (const auto& [uuid, stack] : m_stacks) {
            auto task = stack.FindTask(taskID);
            if (task.has_value()) {
                return stack;
            }
        }
        
        return std::nullopt;
    }

    /**
     * @brief 移除一个业务链路
     */
    bool Remove(const std::string& stackUUID) override {
        std::unique_lock lock(m_mutex);  // 写锁
        return m_stacks.erase(stackUUID) > 0;
    }

    /**
     * @brief 清空所有业务链路
     */
    void Clear() override {
        std::unique_lock lock(m_mutex);  // 写锁
        m_stacks.clear();
    }

    /**
     * @brief 获取业务链路总数
     */
    size_t Count() const override {
        std::shared_lock lock(m_mutex);  // 读锁
        return m_stacks.size();
    }

    /**
     * @brief 统计已部署的业务链路数量
     */
    size_t CountDeployed() const override {
        std::shared_lock lock(m_mutex);  // 读锁
        
        size_t count = 0;
        for (const auto& [uuid, stack] : m_stacks) {
            if (stack.IsDeployed()) {
                count++;
            }
        }
        
        return count;
    }

    /**
     * @brief 统计正常运行的业务链路数量
     */
    size_t CountRunningNormally() const override {
        std::shared_lock lock(m_mutex);  // 读锁
        
        size_t count = 0;
        for (const auto& [uuid, stack] : m_stacks) {
            if (stack.IsRunningNormally()) {
                count++;
            }
        }
        
        return count;
    }

    /**
     * @brief 统计异常运行的业务链路数量
     */
    size_t CountAbnormal() const override {
        std::shared_lock lock(m_mutex);  // 读锁
        
        size_t count = 0;
        for (const auto& [uuid, stack] : m_stacks) {
            if (!stack.IsRunningNormally() && stack.IsDeployed()) {
                count++;
            }
        }
        
        return count;
    }

    /**
     * @brief 统计所有业务链路中的任务总数
     */
    size_t CountTotalTasks() const override {
        std::shared_lock lock(m_mutex);  // 读锁
        
        size_t count = 0;
        for (const auto& [uuid, stack] : m_stacks) {
            count += stack.GetTotalTaskCount();
        }
        
        return count;
    }

private:
    // 存储：Key = stackUUID, Value = Stack聚合根
    std::map<std::string, domain::Stack> m_stacks;
    
    // 读写锁（C++17）
    mutable std::shared_mutex m_mutex;
};

} // namespace zygl::infrastructure

