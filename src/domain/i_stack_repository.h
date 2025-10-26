#pragma once

#include "stack.h"
#include <memory>
#include <vector>
#include <optional>
#include <string>

namespace zygl::domain {

/**
 * @brief IStackRepository接口 - 业务链路仓储
 * 
 * 定义了对Stack聚合根的存储和查询操作。
 * 
 * 注意：
 * - 此接口由infrastructure层实现（InMemoryStackRepository）
 * - 实现类必须使用std::shared_mutex保证线程安全
 * - Stack数量是动态的，由GET /stackinfo接口决定
 */
class IStackRepository {
public:
    virtual ~IStackRepository() = default;

    /**
     * @brief 保存一个业务链路
     * 
     * 如果业务链路已存在（相同stackUUID），则更新；否则插入。
     * 
     * @param stack 业务链路实例
     */
    virtual void Save(const Stack& stack) = 0;

    /**
     * @brief 批量保存多个业务链路
     * 
     * 用于DataCollector从API拉取数据后批量更新。
     * 
     * @param stacks 业务链路列表
     */
    virtual void SaveAll(const std::vector<Stack>& stacks) = 0;

    /**
     * @brief 根据业务链路UUID查找业务链路
     * 
     * @param stackUUID 业务链路UUID
     * @return Stack实例，如果未找到返回std::nullopt
     */
    virtual std::optional<Stack> FindByUUID(const std::string& stackUUID) const = 0;

    /**
     * @brief 获取所有业务链路
     * 
     * @return 所有业务链路的列表
     */
    virtual std::vector<Stack> GetAll() const = 0;

    /**
     * @brief 根据标签UUID查找包含该标签的所有业务链路
     * 
     * 用于deploy/undeploy命令，根据标签批量操作业务链路。
     * 
     * @param labelUUID 标签UUID
     * @return 包含该标签的业务链路列表
     */
    virtual std::vector<Stack> FindByLabel(const std::string& labelUUID) const = 0;

    /**
     * @brief 根据任务ID查找任务的资源使用情况
     * 
     * 这是"方案B（按需查询）"的核心方法。
     * 遍历所有业务链路、组件和任务，查找指定的任务。
     * 
     * @param taskID 任务ID
     * @return 资源使用情况，如果未找到返回std::nullopt
     */
    virtual std::optional<ResourceUsage> FindTaskResources(const std::string& taskID) const = 0;

    /**
     * @brief 根据任务ID查找任务所属的业务链路
     * 
     * @param taskID 任务ID
     * @return Stack实例，如果未找到返回std::nullopt
     */
    virtual std::optional<Stack> FindStackByTaskID(const std::string& taskID) const = 0;

    /**
     * @brief 移除一个业务链路
     * 
     * @param stackUUID 业务链路UUID
     * @return true 如果移除成功，false 如果业务链路不存在
     */
    virtual bool Remove(const std::string& stackUUID) = 0;

    /**
     * @brief 清空所有业务链路
     * 
     * 用于重新初始化或测试。
     */
    virtual void Clear() = 0;

    /**
     * @brief 获取业务链路总数
     */
    virtual size_t Count() const = 0;

    /**
     * @brief 统计已部署的业务链路数量
     */
    virtual size_t CountDeployed() const = 0;

    /**
     * @brief 统计正常运行的业务链路数量
     */
    virtual size_t CountRunningNormally() const = 0;

    /**
     * @brief 统计异常运行的业务链路数量
     */
    virtual size_t CountAbnormal() const = 0;

    /**
     * @brief 统计所有业务链路中的任务总数
     */
    virtual size_t CountTotalTasks() const = 0;
};

} // namespace zygl::domain

