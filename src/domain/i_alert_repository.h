#pragma once

#include "alert.h"
#include <memory>
#include <vector>
#include <optional>
#include <string>

namespace zygl::domain {

/**
 * @brief IAlertRepository接口 - 告警仓储
 * 
 * 定义了对Alert聚合根的存储和查询操作。
 * 
 * 注意：
 * - 此接口由infrastructure层实现（InMemoryAlertRepository）
 * - 实现类必须使用std::shared_mutex保证线程安全
 * - 告警是动态产生的，由WebhookListener接收并存储
 * - 告警仓储中只保留"活动告警"（未确认或最近的告警）
 */
class IAlertRepository {
public:
    virtual ~IAlertRepository() = default;

    /**
     * @brief 保存一个告警
     * 
     * 如果告警已存在（相同alertUUID），则更新；否则插入。
     * 
     * @param alert 告警实例
     */
    virtual void Save(const Alert& alert) = 0;

    /**
     * @brief 根据告警UUID查找告警
     * 
     * @param alertUUID 告警UUID
     * @return Alert实例，如果未找到返回std::nullopt
     */
    virtual std::optional<Alert> FindByUUID(const std::string& alertUUID) const = 0;

    /**
     * @brief 获取所有活动告警
     * 
     * 用于UDP广播，将活动告警发送给前端。
     * 
     * @return 所有活动告警的列表
     */
    virtual std::vector<Alert> GetAllActive() const = 0;

    /**
     * @brief 获取未确认的告警
     * 
     * @return 所有未确认的告警列表
     */
    virtual std::vector<Alert> GetUnacknowledged() const = 0;

    /**
     * @brief 根据类型获取告警
     * 
     * @param type 告警类型（板卡或组件）
     * @return 指定类型的告警列表
     */
    virtual std::vector<Alert> FindByType(AlertType type) const = 0;

    /**
     * @brief 根据相关实体查找告警
     * 
     * @param entityID 实体ID（BoardAddress 或 TaskID）
     * @return 相关的告警列表
     */
    virtual std::vector<Alert> FindByEntity(const std::string& entityID) const = 0;

    /**
     * @brief 根据板卡地址查找板卡告警
     * 
     * @param boardAddress 板卡IP地址
     * @return 该板卡的告警列表
     */
    virtual std::vector<Alert> FindByBoardAddress(const std::string& boardAddress) const = 0;

    /**
     * @brief 根据业务链路UUID查找组件告警
     * 
     * @param stackUUID 业务链路UUID
     * @return 该业务链路的告警列表
     */
    virtual std::vector<Alert> FindByStackUUID(const std::string& stackUUID) const = 0;

    /**
     * @brief 确认一个告警
     * 
     * @param alertUUID 告警UUID
     * @return true 如果确认成功，false 如果告警不存在
     */
    virtual bool Acknowledge(const std::string& alertUUID) = 0;

    /**
     * @brief 批量确认多个告警
     * 
     * @param alertUUIDs 告警UUID列表
     * @return 成功确认的告警数量
     */
    virtual size_t AcknowledgeMultiple(const std::vector<std::string>& alertUUIDs) = 0;

    /**
     * @brief 移除一个告警
     * 
     * @param alertUUID 告警UUID
     * @return true 如果移除成功，false 如果告警不存在
     */
    virtual bool Remove(const std::string& alertUUID) = 0;

    /**
     * @brief 移除过期的告警
     * 
     * 删除超过指定时间的已确认告警，防止内存无限增长。
     * 
     * @param maxAgeSeconds 最大保留时间（秒），超过此时间的已确认告警将被删除
     * @return 删除的告警数量
     */
    virtual size_t RemoveExpired(uint64_t maxAgeSeconds) = 0;

    /**
     * @brief 清空所有告警
     * 
     * 用于重新初始化或测试。
     */
    virtual void Clear() = 0;

    /**
     * @brief 获取活动告警总数
     */
    virtual size_t Count() const = 0;

    /**
     * @brief 获取未确认的告警数量
     */
    virtual size_t CountUnacknowledged() const = 0;

    /**
     * @brief 获取板卡告警数量
     */
    virtual size_t CountBoardAlerts() const = 0;

    /**
     * @brief 获取组件告警数量
     */
    virtual size_t CountComponentAlerts() const = 0;
};

} // namespace zygl::domain

