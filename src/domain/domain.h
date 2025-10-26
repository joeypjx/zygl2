#pragma once

/**
 * @file domain.h
 * @brief 领域层统一头文件
 * 
 * 引入此文件即可使用所有领域层组件。
 * 
 * 使用方式：
 *   #include "domain/domain.h"
 *   using namespace zygl::domain;
 */

// 值对象和枚举
#include "value_objects.h"

// 实体和聚合根
#include "board.h"
#include "chassis.h"
#include "task.h"
#include "service.h"
#include "stack.h"
#include "alert.h"

// 仓储接口
#include "i_chassis_repository.h"
#include "i_stack_repository.h"
#include "i_alert_repository.h"

namespace zygl::domain {

/**
 * @brief 领域层版本信息
 */
constexpr const char* DOMAIN_VERSION = "1.0.0";

/**
 * @brief 系统拓扑常量
 */
struct SystemTopology {
    static constexpr int TOTAL_CHASSIS = 9;           // 总机箱数
    static constexpr int BOARDS_PER_CHASSIS = 14;     // 每机箱板卡数
    static constexpr int TOTAL_BOARDS = 126;          // 总板卡数（9 * 14）
    static constexpr int COMPUTING_BOARDS_PER_CHASSIS = 10;  // 每机箱计算板卡数（14 - 2交换 - 2电源）
    static constexpr int TOTAL_COMPUTING_BOARDS = 90; // 总计算板卡数（9 * 10）
};

/**
 * @brief 板卡槽位类型查询
 */
class BoardSlotHelper {
public:
    /**
     * @brief 根据槽位号判断板卡类型
     * @param slotNumber 槽位号（1-14）
     * @return 板卡类型
     */
    static BoardType GetBoardTypeBySlot(int32_t slotNumber) {
        if (slotNumber == 6 || slotNumber == 7) {
            return BoardType::Switch;
        }
        if (slotNumber == 13 || slotNumber == 14) {
            return BoardType::Power;
        }
        return BoardType::Computing;
    }

    /**
     * @brief 检查槽位号是否有效
     * @param slotNumber 槽位号
     * @return true 如果槽位号在1-14之间
     */
    static bool IsValidSlotNumber(int32_t slotNumber) {
        return slotNumber >= 1 && slotNumber <= BOARDS_PER_CHASSIS;
    }

    /**
     * @brief 检查槽位是否为计算板卡槽位
     * @param slotNumber 槽位号（1-14）
     * @return true 如果是计算板卡槽位
     */
    static bool IsComputingSlot(int32_t slotNumber) {
        return IsValidSlotNumber(slotNumber) && 
               GetBoardTypeBySlot(slotNumber) == BoardType::Computing;
    }
};

} // namespace zygl::domain

