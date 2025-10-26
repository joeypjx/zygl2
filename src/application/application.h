#pragma once

/**
 * @file application.h
 * @brief 应用层统一头文件
 * 
 * 引入此文件即可使用所有应用层组件。
 * 
 * 使用方式：
 *   #include "application/application.h"
 *   using namespace zygl::application;
 */

// DTOs
#include "dtos/dtos.h"

// 服务
#include "services/monitoring_service.h"
#include "services/stack_control_service.h"
#include "services/alert_service.h"

namespace zygl::application {

/**
 * @brief 应用层版本信息
 */
constexpr const char* APPLICATION_VERSION = "1.0.0";

/**
 * @brief 应用服务工厂
 * 
 * 用于创建所有应用层服务实例，方便依赖注入。
 */
class ApplicationServiceFactory {
public:
    /**
     * @brief 创建监控服务
     */
    static std::shared_ptr<MonitoringService> CreateMonitoringService(
        std::shared_ptr<domain::IChassisRepository> chassisRepo,
        std::shared_ptr<domain::IStackRepository> stackRepo,
        std::shared_ptr<domain::IAlertRepository> alertRepo) {
        return std::make_shared<MonitoringService>(
            chassisRepo, stackRepo, alertRepo
        );
    }

    /**
     * @brief 创建业务控制服务
     */
    static std::shared_ptr<StackControlService> CreateStackControlService(
        std::shared_ptr<domain::IStackRepository> stackRepo,
        std::shared_ptr<infrastructure::QywApiClient> apiClient) {
        return std::make_shared<StackControlService>(
            stackRepo, apiClient
        );
    }

    /**
     * @brief 创建告警服务
     */
    static std::shared_ptr<AlertService> CreateAlertService(
        std::shared_ptr<domain::IAlertRepository> alertRepo,
        std::shared_ptr<domain::IChassisRepository> chassisRepo) {
        return std::make_shared<AlertService>(
            alertRepo, chassisRepo
        );
    }

    /**
     * @brief 创建所有应用服务
     */
    struct AllServices {
        std::shared_ptr<MonitoringService> monitoringService;
        std::shared_ptr<StackControlService> stackControlService;
        std::shared_ptr<AlertService> alertService;
    };

    static AllServices CreateAll(
        std::shared_ptr<domain::IChassisRepository> chassisRepo,
        std::shared_ptr<domain::IStackRepository> stackRepo,
        std::shared_ptr<domain::IAlertRepository> alertRepo,
        std::shared_ptr<infrastructure::QywApiClient> apiClient) {
        
        AllServices services;
        services.monitoringService = CreateMonitoringService(
            chassisRepo, stackRepo, alertRepo
        );
        services.stackControlService = CreateStackControlService(
            stackRepo, apiClient
        );
        services.alertService = CreateAlertService(
            alertRepo, chassisRepo
        );
        
        return services;
    }
};

} // namespace zygl::application

