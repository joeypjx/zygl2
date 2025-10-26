#pragma once

#include "../../domain/i_alert_repository.h"
#include "../../domain/i_chassis_repository.h"
#include "../../domain/alert.h"
#include "../dtos/dtos.h"
#include <memory>
#include <vector>
#include <string>
#include <chrono>
#include <random>
#include <sstream>
#include <iomanip>

namespace zygl::application {

/**
 * @brief AlertService - 告警服务
 * 
 * 职责：
 * 1. 处理板卡异常上报（来自WebhookListener）
 * 2. 处理组件异常上报（来自WebhookListener）
 * 3. 告警确认（单个/批量）
 * 4. 告警清理（过期告警）
 * 5. 更新相关实体状态
 */
class AlertService {
public:
    /**
     * @brief 构造函数
     */
    AlertService(
        std::shared_ptr<domain::IAlertRepository> alertRepo,
        std::shared_ptr<domain::IChassisRepository> chassisRepo)
        : m_alertRepo(alertRepo),
          m_chassisRepo(chassisRepo) {
    }

    /**
     * @brief 处理板卡异常上报
     * 
     * 工作流程：
     * 1. 接收板卡异常数据
     * 2. 创建Alert聚合根
     * 3. 保存到告警仓储
     * 4. 更新板卡状态（标记为异常）
     * 
     * @param boardAddress 板卡IP地址
     * @param chassisName 机箱名称
     * @param chassisNumber 机箱号
     * @param boardName 板卡名称
     * @param boardNumber 板卡槽位号
     * @param boardStatus 板卡状态
     * @param alertMessages 告警消息列表
     * @return 响应DTO
     */
    ResponseDTO<std::string> HandleBoardAlert(
        const std::string& boardAddress,
        const std::string& chassisName,
        int32_t chassisNumber,
        const std::string& boardName,
        int32_t boardNumber,
        int32_t boardStatus,
        const std::vector<std::string>& alertMessages) const {
        
        try {
            // 1. 创建位置信息
            domain::LocationInfo location;
            location.SetChassisName(chassisName.c_str());
            location.chassisNumber = chassisNumber;
            location.SetBoardName(boardName.c_str());
            location.boardNumber = boardNumber;
            location.SetBoardAddress(boardAddress.c_str());
            
            // 2. 生成告警UUID
            std::string alertUUID = GenerateAlertUUID("board");
            
            // 3. 创建Alert聚合根
            domain::Alert alert = domain::Alert::CreateBoardAlert(
                alertUUID.c_str(),
                location,
                alertMessages
            );
            
            // 4. 保存告警
            m_alertRepo->Save(alert);
            
            // 5. 更新板卡状态（如果需要）
            // 注意：板卡状态主要由DataCollector更新，这里不强制更新
            
            return ResponseDTO<std::string>::Success(
                alertUUID,
                "板卡告警已记录"
            );
        } catch (const std::exception& e) {
            return ResponseDTO<std::string>::Failure(
                std::string("处理板卡告警失败: ") + e.what()
            );
        }
    }

    /**
     * @brief 处理组件异常上报
     * 
     * 工作流程：
     * 1. 接收组件异常数据
     * 2. 创建Alert聚合根
     * 3. 保存到告警仓储
     * 
     * @param stackName 业务链路名称
     * @param stackUUID 业务链路UUID
     * @param serviceName 组件名称
     * @param serviceUUID 组件UUID
     * @param taskID 任务ID
     * @param location 运行位置
     * @param alertMessages 告警消息列表
     * @return 响应DTO
     */
    ResponseDTO<std::string> HandleComponentAlert(
        const std::string& stackName,
        const std::string& stackUUID,
        const std::string& serviceName,
        const std::string& serviceUUID,
        const std::string& taskID,
        const domain::LocationInfo& location,
        const std::vector<std::string>& alertMessages) const {
        
        try {
            // 1. 生成告警UUID
            std::string alertUUID = GenerateAlertUUID("component");
            
            // 2. 创建Alert聚合根
            domain::Alert alert = domain::Alert::CreateComponentAlert(
                alertUUID.c_str(),
                stackName.c_str(),
                stackUUID.c_str(),
                serviceName.c_str(),
                serviceUUID.c_str(),
                taskID.c_str(),
                location,
                alertMessages
            );
            
            // 3. 保存告警
            m_alertRepo->Save(alert);
            
            return ResponseDTO<std::string>::Success(
                alertUUID,
                "组件告警已记录"
            );
        } catch (const std::exception& e) {
            return ResponseDTO<std::string>::Failure(
                std::string("处理组件告警失败: ") + e.what()
            );
        }
    }

    /**
     * @brief 确认单个告警
     * 
     * @param alertUUID 告警UUID
     * @return 响应DTO
     */
    ResponseDTO<bool> AcknowledgeAlert(const std::string& alertUUID) const {
        try {
            bool success = m_alertRepo->Acknowledge(alertUUID);
            if (success) {
                return ResponseDTO<bool>::Success(true, "告警已确认");
            } else {
                return ResponseDTO<bool>::Failure("告警不存在");
            }
        } catch (const std::exception& e) {
            return ResponseDTO<bool>::Failure(
                std::string("确认告警失败: ") + e.what()
            );
        }
    }

    /**
     * @brief 批量确认告警
     * 
     * @param command 包含告警UUID列表的命令
     * @return 响应DTO，返回确认成功的数量
     */
    ResponseDTO<int32_t> AcknowledgeMultiple(const AlertAcknowledgeDTO& command) const {
        try {
            if (command.alertUUIDs.empty()) {
                return ResponseDTO<int32_t>::Failure("告警UUID列表不能为空");
            }
            
            size_t count = m_alertRepo->AcknowledgeMultiple(command.alertUUIDs);
            
            return ResponseDTO<int32_t>::Success(
                static_cast<int32_t>(count),
                "成功确认 " + std::to_string(count) + " 个告警"
            );
        } catch (const std::exception& e) {
            return ResponseDTO<int32_t>::Failure(
                std::string("批量确认告警失败: ") + e.what()
            );
        }
    }

    /**
     * @brief 清理过期告警
     * 
     * 删除超过指定时间的已确认告警，防止内存无限增长。
     * 建议定期调用（如每小时一次）。
     * 
     * @param maxAgeSeconds 最大保留时间（秒），默认24小时
     * @return 响应DTO，返回删除的告警数量
     */
    ResponseDTO<int32_t> CleanupExpiredAlerts(uint64_t maxAgeSeconds = 86400) const {
        try {
            size_t count = m_alertRepo->RemoveExpired(maxAgeSeconds);
            
            return ResponseDTO<int32_t>::Success(
                static_cast<int32_t>(count),
                "清理了 " + std::to_string(count) + " 个过期告警"
            );
        } catch (const std::exception& e) {
            return ResponseDTO<int32_t>::Failure(
                std::string("清理过期告警失败: ") + e.what()
            );
        }
    }

    /**
     * @brief 删除指定告警
     * 
     * @param alertUUID 告警UUID
     * @return 响应DTO
     */
    ResponseDTO<bool> RemoveAlert(const std::string& alertUUID) const {
        try {
            bool success = m_alertRepo->Remove(alertUUID);
            if (success) {
                return ResponseDTO<bool>::Success(true, "告警已删除");
            } else {
                return ResponseDTO<bool>::Failure("告警不存在");
            }
        } catch (const std::exception& e) {
            return ResponseDTO<bool>::Failure(
                std::string("删除告警失败: ") + e.what()
            );
        }
    }

private:
    /**
     * @brief 生成告警UUID
     * 
     * 格式：alert-{type}-{timestamp}-{random}
     * 例如：alert-board-1698765432-a1b2c3
     */
    std::string GenerateAlertUUID(const std::string& type) const {
        // 获取当前时间戳
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
            now.time_since_epoch()
        ).count();
        
        // 生成随机数
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 0xFFFFFF);
        uint32_t random = dis(gen);
        
        // 格式化UUID
        std::ostringstream oss;
        oss << "alert-" << type << "-" << timestamp << "-" 
            << std::hex << std::setfill('0') << std::setw(6) << random;
        
        return oss.str();
    }

private:
    std::shared_ptr<domain::IAlertRepository> m_alertRepo;
    std::shared_ptr<domain::IChassisRepository> m_chassisRepo;
};

} // namespace zygl::application

