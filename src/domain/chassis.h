#pragma once

#include "board.h"
#include <array>
#include <optional>
#include <cstring>

namespace zygl::domain {

// 常量定义
constexpr int BOARDS_PER_CHASSIS = 14;  // 每个机箱14块板卡

#pragma pack(push, 1)

/**
 * @brief Chassis聚合根 - 机箱
 * 
 * 机箱是硬件拓扑的顶层容器，包含14块固定槽位的板卡。
 * 
 * 业务规则：
 * - 系统中有9个机箱（固定配置）
 * - 每个机箱有14块板卡（槽位1-14）
 * - 槽位6、7是交换板卡，13、14是电源板卡
 * 
 * 作为聚合根，Chassis负责维护其内部所有Board实体的一致性。
 */
class Chassis {
public:
    /**
     * @brief 构造函数 - 创建一个配置好的机箱
     * @param number 机箱号（1-9）
     * @param name 机箱名称
     */
    Chassis(int32_t number, const char* name)
        : m_chassisNumber(number) {
        SetChassisName(name);
    }

    Chassis()
        : m_chassisNumber(0) {
        std::memset(m_chassisName, 0, sizeof(m_chassisName));
    }

    // ==================== 访问器（Getters） ====================

    int32_t GetChassisNumber() const { return m_chassisNumber; }
    const char* GetChassisName() const { return m_chassisName; }
    
    const std::array<Board, BOARDS_PER_CHASSIS>& GetAllBoards() const {
        return m_boards;
    }

    std::array<Board, BOARDS_PER_CHASSIS>& GetAllBoards() {
        return m_boards;
    }

    // ==================== 业务逻辑方法 ====================

    /**
     * @brief 添加或更新一个已配置的板卡
     * 
     * 在启动时，将配置好的板卡添加到机箱中。
     * 板卡的槽位号决定了它在数组中的位置。
     * 
     * @param board 板卡实例
     */
    void AddOrUpdateBoard(const Board& board) {
        int32_t slotIndex = board.GetBoardNumber() - 1;  // 槽位号从1开始，数组索引从0开始
        
        if (slotIndex >= 0 && slotIndex < BOARDS_PER_CHASSIS) {
            m_boards[slotIndex] = board;
        }
    }

    /**
     * @brief 根据IP地址查找板卡
     * @param address 板卡IP地址
     * @return Board指针，如果未找到返回nullptr
     */
    Board* GetBoardByAddress(const char* address) {
        for (auto& board : m_boards) {
            if (std::strcmp(board.GetBoardAddress(), address) == 0) {
                return &board;
            }
        }
        return nullptr;
    }

    const Board* GetBoardByAddress(const char* address) const {
        for (const auto& board : m_boards) {
            if (std::strcmp(board.GetBoardAddress(), address) == 0) {
                return &board;
            }
        }
        return nullptr;
    }

    /**
     * @brief 根据槽位号获取板卡
     * @param boardNumber 槽位号（1-14）
     * @return Board指针，如果槽位号无效返回nullptr
     */
    Board* GetBoardByNumber(int32_t boardNumber) {
        int32_t slotIndex = boardNumber - 1;
        if (slotIndex >= 0 && slotIndex < BOARDS_PER_CHASSIS) {
            return &m_boards[slotIndex];
        }
        return nullptr;
    }

    const Board* GetBoardByNumber(int32_t boardNumber) const {
        int32_t slotIndex = boardNumber - 1;
        if (slotIndex >= 0 && slotIndex < BOARDS_PER_CHASSIS) {
            return &m_boards[slotIndex];
        }
        return nullptr;
    }

    /**
     * @brief 统计处于正常状态的板卡数量
     */
    int32_t CountNormalBoards() const {
        int32_t count = 0;
        for (const auto& board : m_boards) {
            if (board.GetStatus() == BoardOperationalStatus::Normal) {
                count++;
            }
        }
        return count;
    }

    /**
     * @brief 统计处于异常状态的板卡数量
     */
    int32_t CountAbnormalBoards() const {
        int32_t count = 0;
        for (const auto& board : m_boards) {
            if (board.IsAbnormal()) {
                count++;
            }
        }
        return count;
    }

    /**
     * @brief 统计离线的板卡数量
     */
    int32_t CountOfflineBoards() const {
        int32_t count = 0;
        for (const auto& board : m_boards) {
            if (board.GetStatus() == BoardOperationalStatus::Offline) {
                count++;
            }
        }
        return count;
    }

    /**
     * @brief 统计所有计算板卡上的任务总数
     */
    int32_t CountTotalTasks() const {
        int32_t count = 0;
        for (const auto& board : m_boards) {
            if (board.CanRunTasks()) {
                count += board.GetTaskCount();
            }
        }
        return count;
    }

    /**
     * @brief 设置机箱名称
     */
    void SetChassisName(const char* name) {
        std::strncpy(m_chassisName, name, sizeof(m_chassisName) - 1);
        m_chassisName[sizeof(m_chassisName) - 1] = '\0';
    }

private:
    // ==================== 配置信息（固定的） ====================
    char m_chassisName[64];                         // 机箱名称
    int32_t m_chassisNumber;                        // 机箱号（1-9）

    // ==================== 聚合内部实体 ====================
    std::array<Board, BOARDS_PER_CHASSIS> m_boards; // 14块板卡（槽位1-14）
};

#pragma pack(pop)

} // namespace zygl::domain

