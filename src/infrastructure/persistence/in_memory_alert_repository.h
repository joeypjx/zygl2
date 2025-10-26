#pragma once

#include "../../domain/i_alert_repository.h"
#include "../../domain/alert.h"
#include <map>
#include <vector>
#include <optional>
#include <shared_mutex>
#include <string>
#include <algorithm>

namespace zygl::infrastructure {

/**
 * @brief InMemoryAlertRepository - 告警仓储的内存实现
 * 
 * 实现要点：
 * 1. 使用std::map存储Alert聚合根（Key: alertUUID）
 * 2. 使用std::shared_mutex实现读写锁
 * 3. 只保留活动告警，定期清理过期已确认告警
 * 
 * 线程安全：
 * - 读取操作使用std::shared_lock（共享锁）
 * - 写入操作使用std::unique_lock（独占锁）
 * - 支持多读单写
 */
class InMemoryAlertRepository : public domain::IAlertRepository {
public:
    InMemoryAlertRepository() = default;
    
    // 禁止拷贝和移动
    InMemoryAlertRepository(const InMemoryAlertRepository&) = delete;
    InMemoryAlertRepository& operator=(const InMemoryAlertRepository&) = delete;

    /**
     * @brief 保存一个告警
     */
    void Save(const domain::Alert& alert) override {
        std::unique_lock lock(m_mutex);  // 写锁
        std::string uuid(alert.GetAlertUUID());
        m_alerts[uuid] = alert;
    }

    /**
     * @brief 根据UUID查找告警
     */
    std::optional<domain::Alert> FindByUUID(const std::string& alertUUID) const override {
        std::shared_lock lock(m_mutex);  // 读锁
        
        auto it = m_alerts.find(alertUUID);
        if (it != m_alerts.end()) {
            return it->second;
        }
        
        return std::nullopt;
    }

    /**
     * @brief 获取所有活动告警
     * 
     * 用于UDP广播，将活动告警发送给前端。
     */
    std::vector<domain::Alert> GetAllActive() const override {
        std::shared_lock lock(m_mutex);  // 读锁
        
        std::vector<domain::Alert> result;
        result.reserve(m_alerts.size());
        
        for (const auto& [uuid, alert] : m_alerts) {
            result.push_back(alert);
        }
        
        return result;
    }

    /**
     * @brief 获取未确认的告警
     */
    std::vector<domain::Alert> GetUnacknowledged() const override {
        std::shared_lock lock(m_mutex);  // 读锁
        
        std::vector<domain::Alert> result;
        
        for (const auto& [uuid, alert] : m_alerts) {
            if (!alert.IsAcknowledged()) {
                result.push_back(alert);
            }
        }
        
        return result;
    }

    /**
     * @brief 根据类型获取告警
     */
    std::vector<domain::Alert> FindByType(domain::AlertType type) const override {
        std::shared_lock lock(m_mutex);  // 读锁
        
        std::vector<domain::Alert> result;
        
        for (const auto& [uuid, alert] : m_alerts) {
            if (alert.GetAlertType() == type) {
                result.push_back(alert);
            }
        }
        
        return result;
    }

    /**
     * @brief 根据相关实体查找告警
     */
    std::vector<domain::Alert> FindByEntity(const std::string& entityID) const override {
        std::shared_lock lock(m_mutex);  // 读锁
        
        std::vector<domain::Alert> result;
        
        for (const auto& [uuid, alert] : m_alerts) {
            if (std::string(alert.GetRelatedEntity()) == entityID) {
                result.push_back(alert);
            }
        }
        
        return result;
    }

    /**
     * @brief 根据板卡地址查找板卡告警
     */
    std::vector<domain::Alert> FindByBoardAddress(const std::string& boardAddress) const override {
        std::shared_lock lock(m_mutex);  // 读锁
        
        std::vector<domain::Alert> result;
        
        for (const auto& [uuid, alert] : m_alerts) {
            if (alert.IsBoardAlert() && 
                std::string(alert.GetLocation().boardAddress) == boardAddress) {
                result.push_back(alert);
            }
        }
        
        return result;
    }

    /**
     * @brief 根据业务链路UUID查找组件告警
     */
    std::vector<domain::Alert> FindByStackUUID(const std::string& stackUUID) const override {
        std::shared_lock lock(m_mutex);  // 读锁
        
        std::vector<domain::Alert> result;
        
        for (const auto& [uuid, alert] : m_alerts) {
            if (alert.IsComponentAlert() && 
                std::string(alert.GetStackUUID()) == stackUUID) {
                result.push_back(alert);
            }
        }
        
        return result;
    }

    /**
     * @brief 确认一个告警
     */
    bool Acknowledge(const std::string& alertUUID) override {
        std::unique_lock lock(m_mutex);  // 写锁
        
        auto it = m_alerts.find(alertUUID);
        if (it != m_alerts.end()) {
            it->second.Acknowledge();
            return true;
        }
        
        return false;
    }

    /**
     * @brief 批量确认多个告警
     */
    size_t AcknowledgeMultiple(const std::vector<std::string>& alertUUIDs) override {
        std::unique_lock lock(m_mutex);  // 写锁
        
        size_t count = 0;
        for (const auto& uuid : alertUUIDs) {
            auto it = m_alerts.find(uuid);
            if (it != m_alerts.end()) {
                it->second.Acknowledge();
                count++;
            }
        }
        
        return count;
    }

    /**
     * @brief 移除一个告警
     */
    bool Remove(const std::string& alertUUID) override {
        std::unique_lock lock(m_mutex);  // 写锁
        return m_alerts.erase(alertUUID) > 0;
    }

    /**
     * @brief 移除过期的告警
     * 
     * 删除超过指定时间的已确认告警，防止内存无限增长。
     * 
     * @param maxAgeSeconds 最大保留时间（秒），超过此时间的已确认告警将被删除
     * @return 删除的告警数量
     */
    size_t RemoveExpired(uint64_t maxAgeSeconds) override {
        std::unique_lock lock(m_mutex);  // 写锁
        
        size_t removedCount = 0;
        auto it = m_alerts.begin();
        
        while (it != m_alerts.end()) {
            const auto& alert = it->second;
            
            // 只删除已确认且超过保留时间的告警
            if (alert.IsAcknowledged() && alert.GetAgeInSeconds() > maxAgeSeconds) {
                it = m_alerts.erase(it);
                removedCount++;
            } else {
                ++it;
            }
        }
        
        return removedCount;
    }

    /**
     * @brief 清空所有告警
     */
    void Clear() override {
        std::unique_lock lock(m_mutex);  // 写锁
        m_alerts.clear();
    }

    /**
     * @brief 获取活动告警总数
     */
    size_t Count() const override {
        std::shared_lock lock(m_mutex);  // 读锁
        return m_alerts.size();
    }

    /**
     * @brief 获取未确认的告警数量
     */
    size_t CountUnacknowledged() const override {
        std::shared_lock lock(m_mutex);  // 读锁
        
        size_t count = 0;
        for (const auto& [uuid, alert] : m_alerts) {
            if (!alert.IsAcknowledged()) {
                count++;
            }
        }
        
        return count;
    }

    /**
     * @brief 获取板卡告警数量
     */
    size_t CountBoardAlerts() const override {
        std::shared_lock lock(m_mutex);  // 读锁
        
        size_t count = 0;
        for (const auto& [uuid, alert] : m_alerts) {
            if (alert.IsBoardAlert()) {
                count++;
            }
        }
        
        return count;
    }

    /**
     * @brief 获取组件告警数量
     */
    size_t CountComponentAlerts() const override {
        std::shared_lock lock(m_mutex);  // 读锁
        
        size_t count = 0;
        for (const auto& [uuid, alert] : m_alerts) {
            if (alert.IsComponentAlert()) {
                count++;
            }
        }
        
        return count;
    }

private:
    // 存储：Key = alertUUID, Value = Alert聚合根
    std::map<std::string, domain::Alert> m_alerts;
    
    // 读写锁（C++17）
    mutable std::shared_mutex m_mutex;
};

} // namespace zygl::infrastructure

