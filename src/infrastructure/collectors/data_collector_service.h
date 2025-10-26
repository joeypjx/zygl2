#pragma once

#include "../../domain/i_chassis_repository.h"
#include "../../domain/i_stack_repository.h"
#include "../../domain/chassis.h"
#include "../../domain/stack.h"
#include "../../domain/service.h"
#include "../../domain/task.h"
#include "../api_client/qyw_api_client.h"
#include <memory>
#include <thread>
#include <atomic>
#include <chrono>
#include <set>
#include <string>

namespace zygl::infrastructure {

/**
 * @brief DataCollectorService - 数据采集服务
 * 
 * 职责：
 * 1. 定时调用后端API（GET /boardinfo和/stackinfo）
 * 2. 将API数据转换为领域对象
 * 3. 更新内存仓储（双缓冲机制）
 * 
 * 工作流程：
 * 1. 拉取boardinfo，更新Chassis聚合（非活动缓冲）
 * 2. 拉取stackinfo，更新Stack聚合
 * 3. 原子交换Chassis仓储的活动缓冲指针
 * 
 * 线程模型：
 * - 运行在独立的后台线程中
 * - 可以安全启动和停止
 */
class DataCollectorService {
public:
    /**
     * @brief 构造函数
     * @param apiClient API客户端
     * @param chassisRepo 机箱仓储
     * @param stackRepo 业务链路仓储
     * @param intervalSeconds 采集间隔（秒），默认10秒
     */
    DataCollectorService(
        std::shared_ptr<QywApiClient> apiClient,
        std::shared_ptr<domain::IChassisRepository> chassisRepo,
        std::shared_ptr<domain::IStackRepository> stackRepo,
        int intervalSeconds = 10)
        : m_apiClient(apiClient),
          m_chassisRepo(chassisRepo),
          m_stackRepo(stackRepo),
          m_intervalSeconds(intervalSeconds),
          m_running(false) {
    }

    /**
     * @brief 析构函数 - 确保线程正确停止
     */
    ~DataCollectorService() {
        Stop();
    }

    // 禁止拷贝和移动
    DataCollectorService(const DataCollectorService&) = delete;
    DataCollectorService& operator=(const DataCollectorService&) = delete;

    /**
     * @brief 启动采集服务
     */
    void Start() {
        if (m_running.exchange(true)) {
            return;  // 已经在运行
        }
        
        m_thread = std::thread(&DataCollectorService::CollectLoop, this);
    }

    /**
     * @brief 停止采集服务
     */
    void Stop() {
        if (!m_running.exchange(false)) {
            return;  // 已经停止
        }
        
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }

    /**
     * @brief 检查服务是否正在运行
     */
    bool IsRunning() const {
        return m_running.load();
    }

    /**
     * @brief 手动触发一次采集（用于测试）
     */
    void CollectOnce() {
        CollectBoardInfo();
        CollectStackInfo();
    }

    /**
     * @brief 设置采集间隔
     * @param intervalSeconds 间隔秒数
     */
    void SetInterval(int intervalSeconds) {
        m_intervalSeconds = intervalSeconds;
    }

private:
    /**
     * @brief 采集循环（运行在后台线程）
     */
    void CollectLoop() {
        while (m_running.load()) {
            // 执行采集
            CollectBoardInfo();
            CollectStackInfo();
            
            // 等待下一次采集
            auto start = std::chrono::steady_clock::now();
            while (m_running.load()) {
                auto elapsed = std::chrono::steady_clock::now() - start;
                auto elapsedSeconds = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
                
                if (elapsedSeconds >= m_intervalSeconds) {
                    break;
                }
                
                // 短暂睡眠，避免CPU占用
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    }

    /**
     * @brief 采集板卡信息
     * 
     * 从API获取板卡数据，更新Chassis聚合
     */
    void CollectBoardInfo() {
        try {
            // 1. 调用API
            auto boardInfosOpt = m_apiClient->GetBoardInfo();
            if (!boardInfosOpt.has_value()) {
                // API调用失败，跳过本次采集
                return;
            }
            
            const auto& boardInfos = boardInfosOpt.value();
        
            // 2. 获取当前所有机箱（从仓储）
            // 注意：使用堆分配避免栈溢出（Chassis数组约488KB）
            auto allChassisPtr = std::make_unique<std::array<domain::Chassis, domain::TOTAL_CHASSIS_COUNT>>(
                m_chassisRepo->GetAll()
            );
            auto& allChassis = *allChassisPtr;
        
        // 3. 创建已上报板卡的集合（用于判断离线）
        std::set<std::string> reportedBoards;
        for (const auto& boardInfo : boardInfos) {
            reportedBoards.insert(boardInfo.boardAddress);
        }
        
        // 4. 更新每个机箱中的板卡状态
        for (auto& chassis : allChassis) {
            if (chassis.GetChassisNumber() == 0) {
                continue;  // 跳过未初始化的机箱
            }
            
            auto& boards = chassis.GetAllBoards();
            for (auto& board : boards) {
                std::string boardAddr(board.GetBoardAddress());
                
                // 检查此板卡是否在API响应中
                bool found = false;
                for (const auto& boardInfo : boardInfos) {
                    if (boardInfo.boardAddress == boardAddr) {
                        // 找到了，更新状态
                        std::vector<domain::TaskStatusInfo> tasks = ConvertTasks(boardInfo.taskInfos);
                        board.UpdateFromApiData(boardInfo.boardStatus, tasks);
                        found = true;
                        break;
                    }
                }
                
                if (!found) {
                    // 未找到，标记为离线
                    board.MarkAsOffline();
                }
            }
        }
            
            // 5. 原子性地提交所有更新（双缓冲交换）
            m_chassisRepo->SaveAll(allChassis);
        } catch (const std::exception& e) {
            std::cerr << "CollectBoardInfo: 异常 - " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "CollectBoardInfo: 未知异常" << std::endl;
        }
    }

    /**
     * @brief 采集业务链路信息
     * 
     * 从API获取业务链路数据，更新Stack聚合
     */
    void CollectStackInfo() {
        try {
            // 1. 调用API
            auto stackInfosOpt = m_apiClient->GetStackInfo();
            if (!stackInfosOpt.has_value()) {
                return;
            }
            
            const auto& stackInfos = stackInfosOpt.value();
            
            // 2. 转换为领域对象
            std::vector<domain::Stack> stacks;
            for (const auto& stackInfo : stackInfos) {
                domain::Stack stack = ConvertToStack(stackInfo);
                stacks.push_back(stack);
            }
            
            // 3. 批量保存
            m_stackRepo->SaveAll(stacks);
        } catch (const std::exception& e) {
            std::cerr << "CollectStackInfo: 异常 - " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "CollectStackInfo: 未知异常" << std::endl;
        }
    }

    /**
     * @brief 转换任务信息（BoardInfo中的简化版）
     */
    std::vector<domain::TaskStatusInfo> ConvertTasks(
        const std::vector<BoardInfoData::TaskInfo>& taskInfos) const {
        
        std::vector<domain::TaskStatusInfo> result;
        result.reserve(taskInfos.size());
        
        for (const auto& taskInfo : taskInfos) {
            domain::TaskStatusInfo task;
            task.SetTaskID(taskInfo.taskID.c_str());
            task.SetTaskStatus(taskInfo.taskStatus.c_str());
            task.SetServiceName(taskInfo.serviceName.c_str());
            task.SetServiceUUID(taskInfo.serviceUUID.c_str());
            task.SetStackName(taskInfo.stackName.c_str());
            task.SetStackUUID(taskInfo.stackUUID.c_str());
            result.push_back(task);
        }
        
        return result;
    }

    /**
     * @brief 转换StackInfo为Stack聚合根
     */
    domain::Stack ConvertToStack(const StackInfoData& stackInfo) const {
        domain::Stack stack(stackInfo.stackUUID, stackInfo.stackName);
        
        // 设置状态
        stack.SetDeployStatus(static_cast<domain::StackDeployStatus>(stackInfo.stackDeployStatus));
        stack.SetRunningStatus(static_cast<domain::StackRunningStatus>(stackInfo.stackRunningStatus));
        
        // 添加标签
        for (const auto& labelInfo : stackInfo.stackLabelInfos) {
            domain::StackLabelInfo label;
            label.SetLabelName(labelInfo.labelName.c_str());
            label.SetLabelUUID(labelInfo.labelUUID.c_str());
            stack.AddLabel(label);
        }
        
        // 添加组件
        for (const auto& serviceInfo : stackInfo.serviceInfos) {
            domain::Service service(serviceInfo.serviceUUID, serviceInfo.serviceName);
            service.SetStatus(static_cast<domain::ServiceStatus>(serviceInfo.serviceStatus));
            service.SetType(static_cast<domain::ServiceType>(serviceInfo.serviceType));
            
            // 添加任务
            for (const auto& taskInfo : serviceInfo.taskInfos) {
                domain::Task task(taskInfo.taskID);
                task.SetTaskStatus(taskInfo.taskStatus);
                task.SetBoardAddress(taskInfo.boardAddress);
                
                // 设置资源使用
                domain::ResourceUsage resources;
                resources.cpuCores = taskInfo.cpuCores;
                resources.cpuUsed = taskInfo.cpuUsed;
                resources.cpuUsage = taskInfo.cpuUsage;
                resources.memorySize = taskInfo.memorySize;
                resources.memoryUsed = taskInfo.memoryUsed;
                resources.memoryUsage = taskInfo.memoryUsage;
                resources.netReceive = taskInfo.netReceive;
                resources.netSent = taskInfo.netSent;
                resources.gpuMemUsed = taskInfo.gpuMemUsed;
                task.UpdateResources(resources);
                
                // 设置位置信息
                domain::LocationInfo location;
                location.SetChassisName(taskInfo.chassisName.c_str());
                location.chassisNumber = taskInfo.chassisNumber;
                location.SetBoardName(taskInfo.boardName.c_str());
                location.boardNumber = taskInfo.boardNumber;
                location.SetBoardAddress(taskInfo.boardAddress.c_str());
                task.UpdateLocation(location);
                
                service.AddOrUpdateTask(task);
            }
            
            stack.AddOrUpdateService(service);
        }
        
        return stack;
    }

private:
    std::shared_ptr<QywApiClient> m_apiClient;
    std::shared_ptr<domain::IChassisRepository> m_chassisRepo;
    std::shared_ptr<domain::IStackRepository> m_stackRepo;
    
    int m_intervalSeconds;          // 采集间隔
    std::atomic<bool> m_running;    // 运行标志
    std::thread m_thread;           // 后台线程
};

} // namespace zygl::infrastructure

