#pragma once

#include "../../domain/i_chassis_repository.h"
#include "../../domain/chassis.h"
#include <array>
#include <atomic>
#include <memory>
#include <optional>
#include <cstring>

namespace zygl::infrastructure {

/**
 * @brief InMemoryChassisRepository - 机箱仓储的内存实现（双缓冲）
 * 
 * 实现要点：
 * 1. 使用双缓冲（Double Buffering）机制实现无锁读取
 * 2. 写入者更新非活动缓冲，完成后原子交换指针
 * 3. 读取者始终从活动缓冲读取，无需加锁
 * 4. 适用于读多写少的场景（如状态广播）
 * 
 * 线程安全：
 * - GetAll() 是无锁的，可以被多个读取线程并发调用
 * - SaveAll() 应该由单一写入线程调用（DataCollector）
 * - 如果有多个写入者，需要额外的互斥保护
 */
class InMemoryChassisRepository : public domain::IChassisRepository {
public:
    /**
     * @brief 构造函数
     */
    InMemoryChassisRepository() {
        // 初始化两个缓冲区为空机箱
        for (int i = 0; i < domain::TOTAL_CHASSIS_COUNT; ++i) {
            m_buffer_A[i] = domain::Chassis();
            m_buffer_B[i] = domain::Chassis();
        }
        
        // 活动缓冲初始指向buffer_A
        m_activeBuffer.store(&m_buffer_A, std::memory_order_release);
        m_backBuffer = &m_buffer_B;
    }

    /**
     * @brief 析构函数
     */
    ~InMemoryChassisRepository() override = default;

    // 禁止拷贝和移动
    InMemoryChassisRepository(const InMemoryChassisRepository&) = delete;
    InMemoryChassisRepository& operator=(const InMemoryChassisRepository&) = delete;

    /**
     * @brief 保存单个机箱（更新后台缓冲）
     * 
     * 注意：此方法更新后台缓冲，需要调用CommitChanges()才能生效
     */
    void Save(const domain::Chassis& chassis) override {
        int index = chassis.GetChassisNumber() - 1;
        if (index >= 0 && index < domain::TOTAL_CHASSIS_COUNT) {
            m_backBuffer->at(index) = chassis;
        }
    }

    /**
     * @brief 批量保存所有机箱并原子交换缓冲（核心方法）
     * 
     * 这是双缓冲的核心：
     * 1. 将新数据写入后台缓冲
     * 2. 原子交换活动缓冲指针
     * 3. 读取者立即看到新数据，整个过程无锁
     * 
     * @param allChassis 所有9个机箱的新状态
     */
    void SaveAll(const std::array<domain::Chassis, domain::TOTAL_CHASSIS_COUNT>& allChassis) override {
        // 步骤1：将新数据写入后台缓冲
        *m_backBuffer = allChassis;
        
        // 步骤2：原子交换缓冲指针
        auto* oldActive = m_activeBuffer.exchange(m_backBuffer, std::memory_order_acq_rel);
        
        // 步骤3：更新后台缓冲指针（之前的活动缓冲变成新的后台缓冲）
        m_backBuffer = oldActive;
    }

    /**
     * @brief 根据机箱号查找机箱（从活动缓冲读取，无锁）
     */
    std::optional<domain::Chassis> FindByNumber(int32_t chassisNumber) const override {
        if (chassisNumber < 1 || chassisNumber > domain::TOTAL_CHASSIS_COUNT) {
            return std::nullopt;
        }
        
        auto* active = m_activeBuffer.load(std::memory_order_acquire);
        int index = chassisNumber - 1;
        
        // 检查机箱是否已初始化（机箱号不为0表示已初始化）
        if (active->at(index).GetChassisNumber() == 0) {
            return std::nullopt;
        }
        
        return active->at(index);
    }

    /**
     * @brief 获取所有机箱（从活动缓冲读取，无锁）
     * 
     * 这是最常用的方法，用于UDP状态广播。
     * 完全无锁，可以被多个线程并发调用。
     */
    std::array<domain::Chassis, domain::TOTAL_CHASSIS_COUNT> GetAll() const override {
        // 无锁读取活动缓冲
        auto* active = m_activeBuffer.load(std::memory_order_acquire);
        return *active;
    }

    /**
     * @brief 根据板卡地址查找板卡所在的机箱
     */
    std::optional<domain::Chassis> FindByBoardAddress(const char* boardAddress) const override {
        auto* active = m_activeBuffer.load(std::memory_order_acquire);
        
        for (const auto& chassis : *active) {
            // 跳过未初始化的机箱
            if (chassis.GetChassisNumber() == 0) {
                continue;
            }
            
            const auto* board = chassis.GetBoardByAddress(boardAddress);
            if (board != nullptr) {
                return chassis;
            }
        }
        
        return std::nullopt;
    }

    /**
     * @brief 统计所有机箱中的板卡总数
     */
    int32_t CountTotalBoards() const override {
        // 应该始终是 9 * 14 = 126（如果所有机箱都已初始化）
        int32_t count = 0;
        auto* active = m_activeBuffer.load(std::memory_order_acquire);
        
        for (const auto& chassis : *active) {
            if (chassis.GetChassisNumber() != 0) {
                count += domain::BOARDS_PER_CHASSIS;
            }
        }
        
        return count;
    }

    /**
     * @brief 统计所有正常运行的板卡数量
     */
    int32_t CountNormalBoards() const override {
        int32_t count = 0;
        auto* active = m_activeBuffer.load(std::memory_order_acquire);
        
        for (const auto& chassis : *active) {
            if (chassis.GetChassisNumber() != 0) {
                count += chassis.CountNormalBoards();
            }
        }
        
        return count;
    }

    /**
     * @brief 统计所有异常的板卡数量
     */
    int32_t CountAbnormalBoards() const override {
        int32_t count = 0;
        auto* active = m_activeBuffer.load(std::memory_order_acquire);
        
        for (const auto& chassis : *active) {
            if (chassis.GetChassisNumber() != 0) {
                count += chassis.CountAbnormalBoards();
            }
        }
        
        return count;
    }

    /**
     * @brief 统计所有离线的板卡数量
     */
    int32_t CountOfflineBoards() const override {
        int32_t count = 0;
        auto* active = m_activeBuffer.load(std::memory_order_acquire);
        
        for (const auto& chassis : *active) {
            if (chassis.GetChassisNumber() != 0) {
                count += chassis.CountOfflineBoards();
            }
        }
        
        return count;
    }

    /**
     * @brief 统计所有任务总数
     */
    int32_t CountTotalTasks() const override {
        int32_t count = 0;
        auto* active = m_activeBuffer.load(std::memory_order_acquire);
        
        for (const auto& chassis : *active) {
            if (chassis.GetChassisNumber() != 0) {
                count += chassis.CountTotalTasks();
            }
        }
        
        return count;
    }

    /**
     * @brief 初始化仓储（在系统启动时调用一次）
     * 
     * 将固定的9x14拓扑加载到两个缓冲中。
     * 
     * @param initialChassis 初始的9个机箱（由ChassisFactory创建）
     */
    void Initialize(const std::array<domain::Chassis, domain::TOTAL_CHASSIS_COUNT>& initialChassis) override {
        // 初始化两个缓冲
        m_buffer_A = initialChassis;
        m_buffer_B = initialChassis;
        
        // 确保活动缓冲指向buffer_A
        m_activeBuffer.store(&m_buffer_A, std::memory_order_release);
        m_backBuffer = &m_buffer_B;
    }

private:
    using ChassisArray = std::array<domain::Chassis, domain::TOTAL_CHASSIS_COUNT>;
    
    // 双缓冲
    ChassisArray m_buffer_A;
    ChassisArray m_buffer_B;
    
    // 活动缓冲指针（原子操作，支持无锁读取）
    std::atomic<ChassisArray*> m_activeBuffer;
    
    // 后台缓冲指针（非原子，只由写入线程访问）
    ChassisArray* m_backBuffer;
};

} // namespace zygl::infrastructure

