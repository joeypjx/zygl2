#pragma once

#include "../../domain/chassis.h"
#include "../../domain/board.h"
#include "../../domain/domain.h"
#include <array>
#include <string>
#include <cstdio>

namespace zygl::infrastructure {

/**
 * @brief ChassisFactory - 机箱工厂
 * 
 * 职责：
 * 1. 在系统启动时创建完整的9x14硬件拓扑
 * 2. 根据配置信息为每个板卡分配IP地址
 * 3. 根据槽位号自动设置板卡类型（计算/交换/电源）
 * 
 * 业务规则：
 * - 9个机箱，每个机箱14块板卡
 * - 槽位6、7：交换板卡
 * - 槽位13、14：电源板卡
 * - 其他槽位：计算板卡
 */
class ChassisFactory {
public:
    /**
     * @brief 机箱配置结构
     */
    struct ChassisConfig {
        int32_t chassisNumber;              // 机箱号（1-9）
        std::string chassisName;            // 机箱名称
        std::string ipBaseAddress;          // IP地址基础（如"192.168.1"）
        int32_t ipStartOffset;              // IP起始偏移量
    };

    /**
     * @brief 默认构造函数
     */
    ChassisFactory() = default;

    /**
     * @brief 创建一个完整的机箱（包含14块板卡）
     * 
     * @param config 机箱配置
     * @return 完整的Chassis聚合根
     */
    domain::Chassis CreateChassis(const ChassisConfig& config) const {
        domain::Chassis chassis(config.chassisNumber, config.chassisName.c_str());
        
        // 创建14块板卡
        for (int32_t slot = 1; slot <= domain::BOARDS_PER_CHASSIS; ++slot) {
            domain::Board board = CreateBoard(config, slot);
            chassis.AddOrUpdateBoard(board);
        }
        
        return chassis;
    }

    /**
     * @brief 创建完整的系统拓扑（9个机箱，126块板卡）
     * 
     * 使用默认配置：
     * - 机箱名称：机箱-01 到 机箱-09
     * - IP地址：192.168.X.Y
     *   - X = 机箱号（1-9）
     *   - Y = 100 + 槽位号（101-114）
     * 
     * @return 所有9个机箱的数组
     */
    std::array<domain::Chassis, domain::TOTAL_CHASSIS_COUNT> CreateFullTopology() const {
        std::array<domain::Chassis, domain::TOTAL_CHASSIS_COUNT> topology;
        
        for (int32_t chassisNum = 1; chassisNum <= domain::TOTAL_CHASSIS_COUNT; ++chassisNum) {
            ChassisConfig config = CreateDefaultConfig(chassisNum);
            topology[chassisNum - 1] = CreateChassis(config);
        }
        
        return topology;
    }

    /**
     * @brief 创建完整的系统拓扑（使用自定义配置）
     * 
     * @param configs 9个机箱的配置列表
     * @return 所有9个机箱的数组
     */
    std::array<domain::Chassis, domain::TOTAL_CHASSIS_COUNT> 
    CreateFullTopology(const std::array<ChassisConfig, domain::TOTAL_CHASSIS_COUNT>& configs) const {
        std::array<domain::Chassis, domain::TOTAL_CHASSIS_COUNT> topology;
        
        for (size_t i = 0; i < domain::TOTAL_CHASSIS_COUNT; ++i) {
            topology[i] = CreateChassis(configs[i]);
        }
        
        return topology;
    }

private:
    /**
     * @brief 创建单个板卡
     * 
     * @param config 机箱配置
     * @param slotNumber 槽位号（1-14）
     * @return Board实体
     */
    domain::Board CreateBoard(const ChassisConfig& config, int32_t slotNumber) const {
        // 1. 确定板卡类型
        domain::BoardType type = domain::BoardSlotHelper::GetBoardTypeBySlot(slotNumber);
        
        // 2. 生成IP地址
        char ipAddress[16];
        std::snprintf(ipAddress, sizeof(ipAddress), "%s.%d", 
                     config.ipBaseAddress.c_str(), 
                     config.ipStartOffset + slotNumber);
        
        // 3. 创建板卡
        domain::Board board(ipAddress, slotNumber, type);
        
        return board;
    }

    /**
     * @brief 创建默认配置
     * 
     * @param chassisNumber 机箱号（1-9）
     * @return 默认配置
     */
    ChassisConfig CreateDefaultConfig(int32_t chassisNumber) const {
        ChassisConfig config;
        config.chassisNumber = chassisNumber;
        
        // 机箱名称：机箱-01, 机箱-02, ...
        char name[32];
        std::snprintf(name, sizeof(name), "机箱-%02d", chassisNumber);
        config.chassisName = name;
        
        // IP基础地址：192.168.X（X = 机箱号）
        char baseAddr[16];
        std::snprintf(baseAddr, sizeof(baseAddr), "192.168.%d", chassisNumber);
        config.ipBaseAddress = baseAddr;
        
        // IP起始偏移：100（板卡IP从101-114）
        config.ipStartOffset = 100;
        
        return config;
    }
};

} // namespace zygl::infrastructure

