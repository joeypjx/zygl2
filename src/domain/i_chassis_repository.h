#pragma once

#include "chassis.h"
#include <memory>
#include <vector>
#include <optional>
#include <array>

namespace zygl::domain {

// 系统常量
constexpr int TOTAL_CHASSIS_COUNT = 9;  // 系统中总共9个机箱

/**
 * @brief IChassisRepository接口 - 机箱仓储
 * 
 * 定义了对Chassis聚合根的存储和查询操作。
 * 
 * 注意：
 * - 此接口由infrastructure层实现（InMemoryChassisRepository）
 * - 实现类必须保证线程安全（双缓冲机制）
 * - 系统中固定有9个机箱，使用std::array存储
 */
class IChassisRepository {
public:
    virtual ~IChassisRepository() = default;

    /**
     * @brief 保存一个机箱（更新）
     * 
     * 如果机箱已存在（相同chassisNumber），则更新；否则插入。
     * 
     * @param chassis 机箱实例
     */
    virtual void Save(const Chassis& chassis) = 0;

    /**
     * @brief 批量保存所有机箱
     * 
     * 用于DataCollector批量更新所有机箱状态后，原子性地提交。
     * 实现类应该在此方法中执行双缓冲的指针交换。
     * 
     * @param allChassis 所有9个机箱
     */
    virtual void SaveAll(const std::array<Chassis, TOTAL_CHASSIS_COUNT>& allChassis) = 0;

    /**
     * @brief 根据机箱号查找机箱
     * 
     * @param chassisNumber 机箱号（1-9）
     * @return Chassis实例，如果未找到返回std::nullopt
     */
    virtual std::optional<Chassis> FindByNumber(int32_t chassisNumber) const = 0;

    /**
     * @brief 获取所有机箱
     * 
     * 此方法应该是无锁的（从活动缓冲读取），用于UDP广播。
     * 
     * @return 所有9个机箱的数组
     */
    virtual std::array<Chassis, TOTAL_CHASSIS_COUNT> GetAll() const = 0;

    /**
     * @brief 根据板卡地址查找板卡所在的机箱
     * 
     * @param boardAddress 板卡IP地址
     * @return Chassis实例，如果未找到返回std::nullopt
     */
    virtual std::optional<Chassis> FindByBoardAddress(const char* boardAddress) const = 0;

    /**
     * @brief 统计所有机箱中的板卡总数
     * @return 板卡总数（应该是9 * 14 = 126）
     */
    virtual int32_t CountTotalBoards() const = 0;

    /**
     * @brief 统计所有机箱中正常运行的板卡数量
     */
    virtual int32_t CountNormalBoards() const = 0;

    /**
     * @brief 统计所有机箱中异常的板卡数量
     */
    virtual int32_t CountAbnormalBoards() const = 0;

    /**
     * @brief 统计所有机箱中离线的板卡数量
     */
    virtual int32_t CountOfflineBoards() const = 0;

    /**
     * @brief 统计所有机箱中的任务总数
     */
    virtual int32_t CountTotalTasks() const = 0;

    /**
     * @brief 初始化仓储
     * 
     * 用于启动时加载固定的9x14拓扑配置。
     * 
     * @param initialChassis 初始的9个机箱（由ChassisFactory创建）
     */
    virtual void Initialize(const std::array<Chassis, TOTAL_CHASSIS_COUNT>& initialChassis) = 0;
};

} // namespace zygl::domain

