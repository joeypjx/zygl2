#pragma once

#include "value_objects.h"
#include <array>
#include <vector>
#include <cstring>

namespace zygl::domain {

// 常量定义
constexpr int MAX_TASKS_PER_BOARD = 8;  // 每块板卡最多8个任务

#pragma pack(push, 1)

/**
 * @brief Board实体 - 板卡
 * 
 * 板卡代表物理硬件层，运行在机箱的特定槽位上。
 * 板卡可以承载多个任务（最多8个），并维护自身的运行状态。
 * 
 * 业务规则：
 * - 交换板卡（槽位6、7）和电源板卡（槽位13、14）不运行任务
 * - 板卡状态由API动态更新或被标记为离线
 */
class Board {
public:
    /**
     * @brief 构造函数 - 创建一个配置好的板卡
     * @param address 板卡IP地址
     * @param number 板卡槽位号（1-14）
     * @param type 板卡类型
     */
    Board(const char* address, int32_t number, BoardType type)
        : m_boardNumber(number),
          m_boardType(type),
          m_status(BoardOperationalStatus::Unknown),
          m_taskCount(0) {
        SetBoardAddress(address);
        std::memset(m_tasks.data(), 0, sizeof(TaskStatusInfo) * MAX_TASKS_PER_BOARD);
    }

    Board() 
        : m_boardNumber(0),
          m_boardType(BoardType::Computing),
          m_status(BoardOperationalStatus::Unknown),
          m_taskCount(0) {
        std::memset(m_boardAddress, 0, sizeof(m_boardAddress));
        std::memset(m_tasks.data(), 0, sizeof(TaskStatusInfo) * MAX_TASKS_PER_BOARD);
    }

    // ==================== 访问器（Getters） ====================

    const char* GetBoardAddress() const { return m_boardAddress; }
    int32_t GetBoardNumber() const { return m_boardNumber; }
    BoardType GetBoardType() const { return m_boardType; }
    BoardOperationalStatus GetStatus() const { return m_status; }
    int32_t GetTaskCount() const { return m_taskCount; }
    
    const std::array<TaskStatusInfo, MAX_TASKS_PER_BOARD>& GetTasks() const {
        return m_tasks;
    }

    // ==================== 业务逻辑方法 ====================

    /**
     * @brief 检查此板卡类型是否允许运行任务
     * @return true 如果是计算板卡，false 如果是交换或电源板卡
     */
    bool CanRunTasks() const {
        return m_boardType == BoardType::Computing;
    }

    /**
     * @brief 判断板卡是否处于异常状态
     * @return true 如果异常或离线
     */
    bool IsAbnormal() const {
        return m_status == BoardOperationalStatus::Abnormal || 
               m_status == BoardOperationalStatus::Offline;
    }

    /**
     * @brief 判断板卡是否在线
     * @return true 如果正常或异常，false 如果离线或未知
     */
    bool IsOnline() const {
        return m_status == BoardOperationalStatus::Normal ||
               m_status == BoardOperationalStatus::Abnormal;
    }

    /**
     * @brief 用来自API的实时数据更新此板卡的状态
     * 
     * 核心业务规则：
     * - 根据API返回的状态码更新板卡状态
     * - 如果板卡类型不允许运行任务，强制清空任务列表
     * - 最多接受8个任务，超出部分被截断
     * 
     * @param statusFromApi 板卡状态（0-正常，1-异常）
     * @param tasksFromApi 板卡上的任务列表
     */
    void UpdateFromApiData(int32_t statusFromApi, 
                          const std::vector<TaskStatusInfo>& tasksFromApi) {
        // 更新状态
        m_status = (statusFromApi == 0) 
                   ? BoardOperationalStatus::Normal 
                   : BoardOperationalStatus::Abnormal;

        // 检查是否允许运行任务
        if (!CanRunTasks()) {
            m_taskCount = 0;
            return;
        }

        // 更新任务列表（从vector复制到固定大小的array）
        m_taskCount = 0;
        for (size_t i = 0; i < tasksFromApi.size() && i < MAX_TASKS_PER_BOARD; ++i) {
            m_tasks[i] = tasksFromApi[i];
            m_taskCount++;
        }

        // 清空剩余的槽位（防止脏数据）
        for (int32_t i = m_taskCount; i < MAX_TASKS_PER_BOARD; ++i) {
            std::memset(&m_tasks[i], 0, sizeof(TaskStatusInfo));
        }
    }

    /**
     * @brief 将此板卡标记为离线
     * 
     * 当API未上报此板卡信息时调用。
     * 离线板卡没有任务。
     */
    void MarkAsOffline() {
        m_status = BoardOperationalStatus::Offline;
        m_taskCount = 0;
        // 清空所有任务
        for (int32_t i = 0; i < MAX_TASKS_PER_BOARD; ++i) {
            std::memset(&m_tasks[i], 0, sizeof(TaskStatusInfo));
        }
    }

    /**
     * @brief 设置板卡地址
     */
    void SetBoardAddress(const char* address) {
        std::strncpy(m_boardAddress, address, sizeof(m_boardAddress) - 1);
        m_boardAddress[sizeof(m_boardAddress) - 1] = '\0';
    }

private:
    // ==================== 配置信息（固定的） ====================
    char m_boardAddress[16];                        // 板卡IP地址（IPv4）
    int32_t m_boardNumber;                          // 板卡槽位号（1-14）
    BoardType m_boardType;                          // 板卡类型

    // ==================== 动态状态（会变化的） ====================
    BoardOperationalStatus m_status;                // 板卡运行状态
    int32_t m_taskCount;                            // 当前有效任务数量
    std::array<TaskStatusInfo, MAX_TASKS_PER_BOARD> m_tasks;  // 任务列表
};

#pragma pack(pop)

} // namespace zygl::domain

