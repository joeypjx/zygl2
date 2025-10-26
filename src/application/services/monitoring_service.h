#pragma once

#include "../../domain/i_chassis_repository.h"
#include "../../domain/i_stack_repository.h"
#include "../../domain/i_alert_repository.h"
#include "../dtos/dtos.h"
#include <memory>
#include <optional>
#include <vector>

namespace zygl::application {

/**
 * @brief MonitoringService - 监控服务
 * 
 * 职责：
 * 1. 提供系统状态查询（机箱、板卡、任务）
 * 2. 提供业务链路信息查询
 * 3. 提供告警信息查询
 * 4. 将领域对象转换为DTOs
 * 
 * 这是一个只读服务，不修改数据。
 */
class MonitoringService {
public:
    /**
     * @brief 构造函数
     */
    MonitoringService(
        std::shared_ptr<domain::IChassisRepository> chassisRepo,
        std::shared_ptr<domain::IStackRepository> stackRepo,
        std::shared_ptr<domain::IAlertRepository> alertRepo)
        : m_chassisRepo(chassisRepo),
          m_stackRepo(stackRepo),
          m_alertRepo(alertRepo) {
    }

    // ==================== 机箱和板卡查询 ====================

    /**
     * @brief 获取系统概览
     * 
     * 返回所有机箱、板卡和任务的状态信息。
     * 用于前端的主界面展示和UDP状态广播。
     * 
     * @return 系统概览DTO
     */
    ResponseDTO<SystemOverviewDTO> GetSystemOverview() const {
        try {
            SystemOverviewDTO overview;
            
            // 从仓储获取所有机箱
            auto allChassis = m_chassisRepo->GetAll();
            
            // 转换为DTOs
            for (const auto& chassis : allChassis) {
                if (chassis.GetChassisNumber() == 0) {
                    continue;  // 跳过未初始化的机箱
                }
                
                ChassisDTO chassisDTO = ConvertChassisToDTO(chassis);
                overview.chassis.push_back(chassisDTO);
            }
            
            // 计算系统级统计
            overview.totalChassis = static_cast<int32_t>(overview.chassis.size());
            overview.totalBoards = m_chassisRepo->CountTotalBoards();
            overview.totalNormalBoards = m_chassisRepo->CountNormalBoards();
            overview.totalAbnormalBoards = m_chassisRepo->CountAbnormalBoards();
            overview.totalOfflineBoards = m_chassisRepo->CountOfflineBoards();
            overview.totalTasks = m_chassisRepo->CountTotalTasks();
            
            return ResponseDTO<SystemOverviewDTO>::Success(overview);
        } catch (const std::exception& e) {
            return ResponseDTO<SystemOverviewDTO>::Failure(
                std::string("获取系统概览失败: ") + e.what()
            );
        }
    }

    /**
     * @brief 根据机箱号获取机箱信息
     * 
     * @param chassisNumber 机箱号（1-9）
     * @return 机箱DTO
     */
    ResponseDTO<ChassisDTO> GetChassisByNumber(int32_t chassisNumber) const {
        try {
            auto chassisOpt = m_chassisRepo->FindByNumber(chassisNumber);
            if (!chassisOpt.has_value()) {
                return ResponseDTO<ChassisDTO>::Failure("机箱不存在");
            }
            
            ChassisDTO dto = ConvertChassisToDTO(chassisOpt.value());
            return ResponseDTO<ChassisDTO>::Success(dto);
        } catch (const std::exception& e) {
            return ResponseDTO<ChassisDTO>::Failure(
                std::string("获取机箱信息失败: ") + e.what()
            );
        }
    }

    /**
     * @brief 根据板卡地址获取板卡所在的机箱
     * 
     * @param boardAddress 板卡IP地址
     * @return 机箱DTO
     */
    ResponseDTO<ChassisDTO> GetChassisByBoardAddress(const std::string& boardAddress) const {
        try {
            auto chassisOpt = m_chassisRepo->FindByBoardAddress(boardAddress.c_str());
            if (!chassisOpt.has_value()) {
                return ResponseDTO<ChassisDTO>::Failure("板卡不存在");
            }
            
            ChassisDTO dto = ConvertChassisToDTO(chassisOpt.value());
            return ResponseDTO<ChassisDTO>::Success(dto);
        } catch (const std::exception& e) {
            return ResponseDTO<ChassisDTO>::Failure(
                std::string("获取机箱信息失败: ") + e.what()
            );
        }
    }

    // ==================== 业务链路查询 ====================

    /**
     * @brief 获取所有业务链路列表
     * 
     * @return 业务链路列表DTO
     */
    ResponseDTO<StackListDTO> GetAllStacks() const {
        try {
            StackListDTO stackList;
            
            // 从仓储获取所有业务链路
            auto allStacks = m_stackRepo->GetAll();
            
            // 转换为DTOs
            for (const auto& stack : allStacks) {
                StackDTO dto = ConvertStackToDTO(stack);
                stackList.stacks.push_back(dto);
            }
            
            // 计算统计
            stackList.totalStacks = static_cast<int32_t>(stackList.stacks.size());
            stackList.deployedStacks = static_cast<int32_t>(m_stackRepo->CountDeployed());
            stackList.normalRunningStacks = static_cast<int32_t>(m_stackRepo->CountRunningNormally());
            stackList.abnormalStacks = static_cast<int32_t>(m_stackRepo->CountAbnormal());
            
            return ResponseDTO<StackListDTO>::Success(stackList);
        } catch (const std::exception& e) {
            return ResponseDTO<StackListDTO>::Failure(
                std::string("获取业务链路列表失败: ") + e.what()
            );
        }
    }

    /**
     * @brief 根据UUID获取业务链路详情
     * 
     * @param stackUUID 业务链路UUID
     * @return 业务链路DTO
     */
    ResponseDTO<StackDTO> GetStackByUUID(const std::string& stackUUID) const {
        try {
            auto stackOpt = m_stackRepo->FindByUUID(stackUUID);
            if (!stackOpt.has_value()) {
                return ResponseDTO<StackDTO>::Failure("业务链路不存在");
            }
            
            StackDTO dto = ConvertStackToDTO(stackOpt.value());
            return ResponseDTO<StackDTO>::Success(dto);
        } catch (const std::exception& e) {
            return ResponseDTO<StackDTO>::Failure(
                std::string("获取业务链路失败: ") + e.what()
            );
        }
    }

    /**
     * @brief 根据任务ID获取任务资源使用情况（方案B核心）
     * 
     * 这是"按需查询"的核心方法。
     * 当前端点击任务时，调用此方法获取详细的资源使用情况。
     * 
     * @param taskID 任务ID
     * @return 任务资源DTO
     */
    ResponseDTO<TaskResourceDTO> GetTaskResource(const std::string& taskID) const {
        try {
            // 从Stack仓储查找任务资源（方案B）
            auto resourceOpt = m_stackRepo->FindTaskResources(taskID);
            if (!resourceOpt.has_value()) {
                return ResponseDTO<TaskResourceDTO>::Failure("任务不存在");
            }
            
            // 查找任务所属的业务链路，获取位置信息
            auto stackOpt = m_stackRepo->FindStackByTaskID(taskID);
            if (!stackOpt.has_value()) {
                return ResponseDTO<TaskResourceDTO>::Failure("任务所属业务链路不存在");
            }
            
            const auto& stack = stackOpt.value();
            auto taskOpt = stack.FindTask(taskID);
            if (!taskOpt.has_value()) {
                return ResponseDTO<TaskResourceDTO>::Failure("任务详情不存在");
            }
            
            // 转换为DTO
            TaskResourceDTO dto = ConvertTaskResourceToDTO(taskID, taskOpt.value());
            return ResponseDTO<TaskResourceDTO>::Success(dto);
        } catch (const std::exception& e) {
            return ResponseDTO<TaskResourceDTO>::Failure(
                std::string("获取任务资源失败: ") + e.what()
            );
        }
    }

    // ==================== 告警查询 ====================

    /**
     * @brief 获取所有活动告警
     * 
     * 用于UDP广播和前端告警面板显示。
     * 
     * @return 告警列表DTO
     */
    ResponseDTO<AlertListDTO> GetActiveAlerts() const {
        try {
            AlertListDTO alertList;
            
            // 从仓储获取所有活动告警
            auto allAlerts = m_alertRepo->GetAllActive();
            
            // 转换为DTOs
            for (const auto& alert : allAlerts) {
                AlertDTO dto = ConvertAlertToDTO(alert);
                alertList.alerts.push_back(dto);
            }
            
            // 计算统计
            alertList.totalAlerts = static_cast<int32_t>(alertList.alerts.size());
            alertList.unacknowledgedCount = static_cast<int32_t>(m_alertRepo->CountUnacknowledged());
            alertList.boardAlertCount = static_cast<int32_t>(m_alertRepo->CountBoardAlerts());
            alertList.componentAlertCount = static_cast<int32_t>(m_alertRepo->CountComponentAlerts());
            
            return ResponseDTO<AlertListDTO>::Success(alertList);
        } catch (const std::exception& e) {
            return ResponseDTO<AlertListDTO>::Failure(
                std::string("获取告警列表失败: ") + e.what()
            );
        }
    }

    /**
     * @brief 获取未确认的告警
     * 
     * @return 告警列表DTO
     */
    ResponseDTO<AlertListDTO> GetUnacknowledgedAlerts() const {
        try {
            AlertListDTO alertList;
            
            auto unackAlerts = m_alertRepo->GetUnacknowledged();
            
            for (const auto& alert : unackAlerts) {
                AlertDTO dto = ConvertAlertToDTO(alert);
                alertList.alerts.push_back(dto);
            }
            
            alertList.totalAlerts = static_cast<int32_t>(alertList.alerts.size());
            alertList.unacknowledgedCount = alertList.totalAlerts;
            alertList.boardAlertCount = 0;
            alertList.componentAlertCount = 0;
            
            for (const auto& dto : alertList.alerts) {
                if (dto.alertType == 0) alertList.boardAlertCount++;
                else alertList.componentAlertCount++;
            }
            
            return ResponseDTO<AlertListDTO>::Success(alertList);
        } catch (const std::exception& e) {
            return ResponseDTO<AlertListDTO>::Failure(
                std::string("获取未确认告警失败: ") + e.what()
            );
        }
    }

private:
    // ==================== 转换方法 ====================

    /**
     * @brief 转换Chassis为DTO
     */
    ChassisDTO ConvertChassisToDTO(const domain::Chassis& chassis) const {
        ChassisDTO dto;
        dto.chassisNumber = chassis.GetChassisNumber();
        dto.chassisName = chassis.GetChassisName();
        
        // 转换所有板卡
        const auto& boards = chassis.GetAllBoards();
        for (const auto& board : boards) {
            BoardDTO boardDTO = ConvertBoardToDTO(board);
            dto.boards.push_back(boardDTO);
        }
        
        // 统计信息
        dto.totalBoards = domain::BOARDS_PER_CHASSIS;
        dto.normalBoards = chassis.CountNormalBoards();
        dto.abnormalBoards = chassis.CountAbnormalBoards();
        dto.offlineBoards = chassis.CountOfflineBoards();
        dto.totalTasks = chassis.CountTotalTasks();
        
        return dto;
    }

    /**
     * @brief 转换Board为DTO
     */
    BoardDTO ConvertBoardToDTO(const domain::Board& board) const {
        BoardDTO dto;
        dto.boardAddress = board.GetBoardAddress();
        dto.boardNumber = board.GetBoardNumber();
        dto.boardType = static_cast<int32_t>(board.GetBoardType());
        dto.boardStatus = static_cast<int32_t>(board.GetStatus());
        dto.taskCount = board.GetTaskCount();
        
        // 提取任务ID和状态
        const auto& tasks = board.GetTasks();
        for (int i = 0; i < board.GetTaskCount() && i < domain::MAX_TASKS_PER_BOARD; ++i) {
            dto.taskIDs.push_back(tasks[i].taskID);
            dto.taskStatuses.push_back(tasks[i].taskStatus);
        }
        
        return dto;
    }

    /**
     * @brief 转换Stack为DTO
     */
    StackDTO ConvertStackToDTO(const domain::Stack& stack) const {
        StackDTO dto;
        dto.stackUUID = stack.GetStackUUID();
        dto.stackName = stack.GetStackName();
        dto.deployStatus = static_cast<int32_t>(stack.GetDeployStatus());
        dto.runningStatus = static_cast<int32_t>(stack.GetRunningStatus());
        
        // 标签
        const auto& labels = stack.GetLabels();
        for (int i = 0; i < stack.GetLabelCount(); ++i) {
            dto.labelNames.push_back(labels[i].labelName);
            dto.labelUUIDs.push_back(labels[i].labelUUID);
        }
        
        // 组件（简化版，不包含任务详情）
        const auto& services = stack.GetAllServices();
        for (const auto& [uuid, service] : services) {
            ServiceDTO serviceDTO;
            serviceDTO.serviceUUID = service.GetServiceUUID();
            serviceDTO.serviceName = service.GetServiceName();
            serviceDTO.serviceStatus = static_cast<int32_t>(service.GetStatus());
            serviceDTO.serviceType = static_cast<int32_t>(service.GetType());
            serviceDTO.taskCount = static_cast<int32_t>(service.GetTaskCount());
            serviceDTO.taskIDs = service.GetTaskIDs();
            dto.services.push_back(serviceDTO);
        }
        
        // 统计
        dto.serviceCount = static_cast<int32_t>(stack.GetServiceCount());
        dto.totalTaskCount = static_cast<int32_t>(stack.GetTotalTaskCount());
        
        return dto;
    }

    /**
     * @brief 转换Task资源为DTO
     */
    TaskResourceDTO ConvertTaskResourceToDTO(const std::string& taskID, const domain::Task& task) const {
        TaskResourceDTO dto;
        dto.taskID = taskID;
        dto.taskStatus = task.GetTaskStatus();
        
        // 资源使用
        const auto& resources = task.GetResources();
        dto.cpuCores = resources.cpuCores;
        dto.cpuUsed = resources.cpuUsed;
        dto.cpuUsage = resources.cpuUsage;
        dto.memorySize = resources.memorySize;
        dto.memoryUsed = resources.memoryUsed;
        dto.memoryUsage = resources.memoryUsage;
        dto.netReceive = resources.netReceive;
        dto.netSent = resources.netSent;
        dto.gpuMemUsed = resources.gpuMemUsed;
        
        // 位置信息
        const auto& location = task.GetLocation();
        dto.chassisName = location.chassisName;
        dto.chassisNumber = location.chassisNumber;
        dto.boardName = location.boardName;
        dto.boardNumber = location.boardNumber;
        dto.boardAddress = location.boardAddress;
        
        return dto;
    }

    /**
     * @brief 转换Alert为DTO
     */
    AlertDTO ConvertAlertToDTO(const domain::Alert& alert) const {
        AlertDTO dto;
        dto.alertUUID = alert.GetAlertUUID();
        dto.alertType = static_cast<int32_t>(alert.GetAlertType());
        dto.timestamp = alert.GetTimestamp();
        dto.isAcknowledged = alert.IsAcknowledged();
        dto.relatedEntity = alert.GetRelatedEntity();
        
        // 告警消息
        const auto& messages = alert.GetMessages();
        for (int i = 0; i < alert.GetMessageCount(); ++i) {
            dto.messages.push_back(messages[i].message);
        }
        
        // 位置信息
        const auto& location = alert.GetLocation();
        dto.chassisName = location.chassisName;
        dto.chassisNumber = location.chassisNumber;
        dto.boardName = location.boardName;
        dto.boardNumber = location.boardNumber;
        dto.boardAddress = location.boardAddress;
        
        // 组件告警专用字段
        if (alert.IsComponentAlert()) {
            dto.stackName = alert.GetStackName();
            dto.stackUUID = alert.GetStackUUID();
            dto.serviceName = alert.GetServiceName();
            dto.serviceUUID = alert.GetServiceUUID();
            dto.taskID = alert.GetTaskID();
        }
        
        return dto;
    }

private:
    std::shared_ptr<domain::IChassisRepository> m_chassisRepo;
    std::shared_ptr<domain::IStackRepository> m_stackRepo;
    std::shared_ptr<domain::IAlertRepository> m_alertRepo;
};

} // namespace zygl::application

