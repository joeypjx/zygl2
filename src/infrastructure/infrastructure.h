#pragma once

/**
 * @file infrastructure.h
 * @brief 基础设施层统一头文件
 * 
 * 引入此文件即可使用所有基础设施层组件。
 * 
 * 使用方式：
 *   #include "infrastructure/infrastructure.h"
 *   using namespace zygl::infrastructure;
 */

// 仓储实现
#include "persistence/in_memory_chassis_repository.h"
#include "persistence/in_memory_stack_repository.h"
#include "persistence/in_memory_alert_repository.h"

// API客户端
#include "api_client/qyw_api_client.h"

// 配置和工厂
#include "config/chassis_factory.h"

// 数据采集器
#include "collectors/data_collector_service.h"

namespace zygl::infrastructure {

/**
 * @brief 基础设施层版本信息
 */
constexpr const char* INFRASTRUCTURE_VERSION = "1.0.0";

/**
 * @brief 仓储工厂 - 创建所有仓储实例
 * 
 * 用于依赖注入的辅助类。
 */
class RepositoryFactory {
public:
    /**
     * @brief 创建机箱仓储实例（双缓冲）
     */
    static std::shared_ptr<domain::IChassisRepository> CreateChassisRepository() {
        return std::make_shared<InMemoryChassisRepository>();
    }

    /**
     * @brief 创建业务链路仓储实例（shared_mutex）
     */
    static std::shared_ptr<domain::IStackRepository> CreateStackRepository() {
        return std::make_shared<InMemoryStackRepository>();
    }

    /**
     * @brief 创建告警仓储实例（shared_mutex）
     */
    static std::shared_ptr<domain::IAlertRepository> CreateAlertRepository() {
        return std::make_shared<InMemoryAlertRepository>();
    }

    /**
     * @brief 创建所有仓储实例
     */
    struct AllRepositories {
        std::shared_ptr<domain::IChassisRepository> chassisRepo;
        std::shared_ptr<domain::IStackRepository> stackRepo;
        std::shared_ptr<domain::IAlertRepository> alertRepo;
    };

    static AllRepositories CreateAll() {
        AllRepositories repos;
        repos.chassisRepo = CreateChassisRepository();
        repos.stackRepo = CreateStackRepository();
        repos.alertRepo = CreateAlertRepository();
        return repos;
    }
};

/**
 * @brief 服务工厂 - 创建所有服务实例
 */
class ServiceFactory {
public:
    /**
     * @brief 创建API客户端
     * @param baseUrl API基础URL
     * @param timeout 超时时间（秒）
     */
    static std::shared_ptr<QywApiClient> CreateApiClient(
        const std::string& baseUrl, 
        int timeout = 10) {
        return std::make_shared<QywApiClient>(baseUrl, timeout);
    }

    /**
     * @brief 创建数据采集服务
     * @param apiClient API客户端
     * @param chassisRepo 机箱仓储
     * @param stackRepo 业务链路仓储
     * @param intervalSeconds 采集间隔（秒）
     */
    static std::shared_ptr<DataCollectorService> CreateDataCollector(
        std::shared_ptr<QywApiClient> apiClient,
        std::shared_ptr<domain::IChassisRepository> chassisRepo,
        std::shared_ptr<domain::IStackRepository> stackRepo,
        int intervalSeconds = 10) {
        return std::make_shared<DataCollectorService>(
            apiClient, chassisRepo, stackRepo, intervalSeconds
        );
    }
};

/**
 * @brief 系统初始化器
 * 
 * 负责系统启动时的初始化工作。
 */
class SystemInitializer {
public:
    /**
     * @brief 初始化系统拓扑
     * 
     * 创建9x14的固定拓扑并加载到机箱仓储。
     * 
     * @param chassisRepo 机箱仓储
     */
    static void InitializeTopology(std::shared_ptr<domain::IChassisRepository> chassisRepo) {
        ChassisFactory factory;
        auto topology = factory.CreateFullTopology();
        chassisRepo->Initialize(topology);
    }

    /**
     * @brief 初始化系统拓扑（使用自定义配置）
     * 
     * @param chassisRepo 机箱仓储
     * @param configs 机箱配置列表
     */
    static void InitializeTopology(
        std::shared_ptr<domain::IChassisRepository> chassisRepo,
        const std::array<ChassisFactory::ChassisConfig, domain::TOTAL_CHASSIS_COUNT>& configs) {
        ChassisFactory factory;
        auto topology = factory.CreateFullTopology(configs);
        chassisRepo->Initialize(topology);
    }
};

} // namespace zygl::infrastructure

