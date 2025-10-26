/**
 * @file main.cpp
 * @brief 资源管理系统主程序入口
 * @version 1.0.0
 * @date 2025-10-26
 * 
 * 基于DDD架构的资源管理系统
 * - 领域层（Domain Layer）: 核心业务逻辑
 * - 基础设施层（Infrastructure Layer）: 数据持久化、API客户端
 * - 应用层（Application Layer）: 应用服务
 * - 接口层（Interfaces Layer）: UDP/HTTP通信
 */

#include "domain/domain.h"
#include "infrastructure/infrastructure.h"
#include "infrastructure/config/config_loader.h"
#include "application/application.h"
#include "interfaces/interfaces.h"

#include <iostream>
#include <memory>
#include <thread>
#include <csignal>
#include <atomic>
#include <cstring>
#include <ctime>

using namespace std;

// 全局运行标志
atomic<bool> g_running(true);

/**
 * @brief 信号处理函数
 * @param signal 信号编号
 */
void SignalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        cout << "\n收到停止信号，准备关闭系统..." << endl;
        g_running = false;
    }
}

/**
 * @brief 显示系统启动信息
 */
void PrintBanner() {
    cout << "\n";
    cout << "╔══════════════════════════════════════════════════════════════╗\n";
    cout << "║           资源管理系统 (ZYGL2)                                ║\n";
    cout << "╚══════════════════════════════════════════════════════════════╝\n";
    cout << "\n";
    cout << "版本: " << zygl::domain::DOMAIN_VERSION << "\n";
    cout << "架构: DDD (领域驱动设计)\n";
    cout << "\n";
    cout << "系统拓扑:\n";
    cout << "  - 机箱数量: " << zygl::domain::SystemTopology::TOTAL_CHASSIS << "\n";
    cout << "  - 每机箱板卡数: " << zygl::domain::SystemTopology::BOARDS_PER_CHASSIS << "\n";
    cout << "  - 总板卡数: " << zygl::domain::SystemTopology::TOTAL_BOARDS << "\n";
    cout << "  - 计算板卡数: " << zygl::domain::SystemTopology::TOTAL_COMPUTING_BOARDS << "\n";
    cout << "  - 每板卡最多任务数: " << zygl::domain::MAX_TASKS_PER_BOARD << "\n";
    cout << "\n";
    cout << "通信协议:\n";
    cout << "  - UDP组播: 前端状态广播和命令接收\n";
    cout << "  - HTTP客户端: 后端API数据采集\n";
    cout << "  - HTTP服务器: 后端Webhook接收\n";
    cout << "\n";
    cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    cout << "\n";
}


/**
 * @brief 应用程序引导器 - 负责整个系统的启动和关闭
 * 
 * 职责：
 * - 协调基础设施层、应用层、接口层的初始化
 * - 管理所有组件的生命周期
 * - 提供优雅启动和关闭
 */
class ApplicationBootstrap {
public:
    /**
     * @brief 初始化系统
     * @return 初始化是否成功
     */
    bool Initialize() {
        cout << "【系统初始化】\n";
        
        // 1. 初始化基础设施层
        cout << "  [1/4] 初始化基础设施层...\n";
        if (!InitializeInfrastructure()) {
            cerr << "    ❌ 基础设施层初始化失败" << endl;
            return false;
        }
        cout << "    ✅ 基础设施层初始化完成\n";
        
        // 2. 初始化应用层
        cout << "  [2/4] 初始化应用层...\n";
        if (!InitializeApplication()) {
            cerr << "    ❌ 应用层初始化失败" << endl;
            return false;
        }
        cout << "    ✅ 应用层初始化完成\n";
        
        // 3. 初始化接口层
        cout << "  [3/4] 初始化接口层...\n";
        if (!InitializeInterfaces()) {
            cerr << "    ❌ 接口层初始化失败" << endl;
            return false;
        }
        cout << "    ✅ 接口层初始化完成\n";
        
        // 4. 启动后台服务
        cout << "  [4/4] 启动后台服务...\n";
        if (!StartBackgroundServices()) {
            cerr << "    ❌ 后台服务启动失败" << endl;
            return false;
        }
        cout << "    ✅ 后台服务启动完成\n";
        
        cout << "\n";
        cout << "✅ 系统初始化成功！\n";
        cout << "\n";
        return true;
    }
    
    /**
     * @brief 关闭系统
     */
    void Shutdown() {
        cout << "\n";
        cout << "【系统关闭】\n";
        
        cout << "  [1/3] 停止后台服务...\n";
        StopBackgroundServices();
        cout << "    ✅ 后台服务已停止\n";
        
        cout << "  [2/3] 关闭接口层...\n";
        ShutdownInterfaces();
        cout << "    ✅ 接口层已关闭\n";
        
        cout << "  [3/3] 清理资源...\n";
        CleanupResources();
        cout << "    ✅ 资源已清理\n";
        
        cout << "\n";
        cout << "✅ 系统已安全关闭\n";
        cout << "\n";
    }
    
    /**
     * @brief 获取监控服务（用于主循环显示状态）
     */
    shared_ptr<zygl::application::MonitoringService> GetMonitoringService() const {
        return m_monitoringService;
    }
    
    /**
     * @brief 加载配置
     */
    void LoadConfiguration(const std::string& configPath) {
        m_config = zygl::infrastructure::ConfigLoader::LoadFromFile(configPath);
        
        // 验证配置
        if (!zygl::infrastructure::ConfigLoader::ValidateConfig(m_config)) {
            cerr << "⚠️  配置验证失败，将使用默认值" << endl;
        }
        
        // 打印配置信息
        zygl::infrastructure::ConfigLoader::PrintConfig(m_config);
    }

private:
    // 系统配置
    zygl::infrastructure::SystemConfig m_config;
    // 基础设施层组件
    shared_ptr<zygl::domain::IChassisRepository> m_chassisRepo;
    shared_ptr<zygl::domain::IStackRepository> m_stackRepo;
    shared_ptr<zygl::domain::IAlertRepository> m_alertRepo;
    shared_ptr<zygl::infrastructure::QywApiClient> m_apiClient;
    shared_ptr<zygl::infrastructure::DataCollectorService> m_dataCollector;
    
    // 应用层组件
    shared_ptr<zygl::application::MonitoringService> m_monitoringService;
    shared_ptr<zygl::application::StackControlService> m_stackControlService;
    shared_ptr<zygl::application::AlertService> m_alertService;
    
    // 接口层组件
    shared_ptr<zygl::interfaces::StateBroadcaster> m_stateBroadcaster;
    shared_ptr<zygl::interfaces::CommandListener> m_commandListener;
    shared_ptr<zygl::interfaces::WebhookListener> m_webhookListener;
    
    /**
     * @brief 初始化基础设施层
     */
    bool InitializeInfrastructure() {
        try {
            // 1. 创建所有Repository实例
            auto repos = zygl::infrastructure::RepositoryFactory::CreateAll();
            m_chassisRepo = repos.chassisRepo;
            m_stackRepo = repos.stackRepo;
            m_alertRepo = repos.alertRepo;
            
            // 2. 初始化系统拓扑（9×14机箱配置）
            zygl::infrastructure::SystemInitializer::InitializeTopology(m_chassisRepo);
            
            // 3. 创建API客户端（使用配置）
            m_apiClient = zygl::infrastructure::ServiceFactory::CreateApiClient(
                m_config.backend.apiUrl,           // 后端API地址
                m_config.backend.timeoutSeconds    // 超时时间（秒）
            );
            
            // 4. 创建数据采集服务（使用配置）
            m_dataCollector = zygl::infrastructure::ServiceFactory::CreateDataCollector(
                m_apiClient,
                m_chassisRepo,
                m_stackRepo,
                m_config.dataCollector.intervalSeconds  // 采集间隔
            );
            
            return true;
        } catch (const exception& e) {
            cerr << "    初始化基础设施层异常: " << e.what() << endl;
            return false;
        }
    }
    
    /**
     * @brief 初始化应用层
     */
    bool InitializeApplication() {
        try {
            // 1. 创建监控服务（系统状态查询）
            m_monitoringService = make_shared<zygl::application::MonitoringService>(
                m_chassisRepo,
                m_stackRepo,
                m_alertRepo
            );
            
            // 2. 创建业务链路控制服务（deploy/undeploy）
            m_stackControlService = make_shared<zygl::application::StackControlService>(
                m_stackRepo,
                m_apiClient
            );
            
            // 3. 创建告警服务（告警处理）
            m_alertService = make_shared<zygl::application::AlertService>(
                m_alertRepo,
                m_chassisRepo
            );
            
            return true;
        } catch (const exception& e) {
            cerr << "    初始化应用层异常: " << e.what() << endl;
            return false;
        }
    }
    
    /**
     * @brief 初始化接口层
     */
    bool InitializeInterfaces() {
        try {
            // 1. 创建状态广播器（UDP组播，向前端推送状态，使用配置）
            m_stateBroadcaster = make_shared<zygl::interfaces::StateBroadcaster>(
                m_monitoringService,
                m_config.udp.broadcastIntervalMs
            );
            
            // 2. 创建命令监听器（UDP组播，接收前端命令）
            m_commandListener = make_shared<zygl::interfaces::CommandListener>(
                m_stackControlService,
                m_alertService
            );
            
            // 3. 创建Webhook监听器（HTTP服务器，接收后端告警，使用配置）
            m_webhookListener = make_shared<zygl::interfaces::WebhookListener>(
                m_alertService,
                m_config.webhook.listenPort
            );
            
            return true;
        } catch (const exception& e) {
            cerr << "    初始化接口层异常: " << e.what() << endl;
            return false;
        }
    }
    
    /**
     * @brief 启动后台服务
     */
    bool StartBackgroundServices() {
        try {
            // 1. 启动数据采集服务（定时从后端API获取数据）
            if (m_dataCollector) {
                m_dataCollector->Start();
                cout << "      ✓ 数据采集服务已启动\n";
            }
            
            // 2. 启动状态广播（定时向前端UDP广播系统状态）
            if (m_stateBroadcaster) {
                m_stateBroadcaster->Start();
                cout << "      ✓ 状态广播服务已启动\n";
            }
            
            // 3. 启动命令监听（接收前端UDP命令）
            if (m_commandListener) {
                m_commandListener->Start();
                cout << "      ✓ 命令监听服务已启动\n";
            }
            
            // 4. 启动Webhook服务（接收后端告警推送）
            if (m_webhookListener) {
                m_webhookListener->Start();
                cout << "      ✓ Webhook服务已启动\n";
            }
            
            return true;
        } catch (const exception& e) {
            cerr << "    启动后台服务异常: " << e.what() << endl;
            return false;
        }
    }
    
    /**
     * @brief 停止后台服务
     */
    void StopBackgroundServices() {
        // 按启动的相反顺序停止服务
        
        // 1. 停止Webhook服务
        if (m_webhookListener) {
            m_webhookListener->Stop();
            cout << "      ✓ Webhook服务已停止\n";
        }
        
        // 2. 停止命令监听
        if (m_commandListener) {
            m_commandListener->Stop();
            cout << "      ✓ 命令监听服务已停止\n";
        }
        
        // 3. 停止状态广播
        if (m_stateBroadcaster) {
            m_stateBroadcaster->Stop();
            cout << "      ✓ 状态广播服务已停止\n";
        }
        
        // 4. 停止数据采集服务
        if (m_dataCollector) {
            m_dataCollector->Stop();
            cout << "      ✓ 数据采集服务已停止\n";
        }
    }
    
    /**
     * @brief 关闭接口层
     */
    void ShutdownInterfaces() {
        // 接口层已在StopBackgroundServices中停止
    }
    
    /**
     * @brief 清理资源
     */
    void CleanupResources() {
        // 智能指针会自动清理
    }
};

/**
 * @brief 主函数
 */
int main(int argc, char* argv[]) {
    // 设置信号处理
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);
    
    // 显示启动信息
    PrintBanner();
    
    // 初始化系统
    ApplicationBootstrap bootstrap;
    
    // 加载配置（支持命令行参数指定配置文件）
    string configPath = "config.json";  // 默认配置文件
    if (argc >= 2) {
        configPath = argv[1];
        cout << "使用自定义配置文件: " << configPath << "\n\n";
    } else {
        cout << "使用默认配置文件: " << configPath << "\n\n";
    }
    
    bootstrap.LoadConfiguration(configPath);
    
    // 初始化系统组件
    if (!bootstrap.Initialize()) {
        cerr << "❌ 系统初始化失败，退出程序" << endl;
        return 1;
    }
    
    // TODO: 生产环境可以移除控制台状态打印（日志记录应该在各个服务内部）
    // 主循环
    cout << "【系统运行中】\n";
    cout << "  系统正在运行，按 Ctrl+C 停止...\n";
    cout << "\n";
    
    // 定期显示系统状态
    int statusCounter = 0;
    auto monitoringService = bootstrap.GetMonitoringService();
    
    while (g_running) {
        // 休眠1秒
        this_thread::sleep_for(chrono::seconds(1));
        
        // 每10秒显示一次心跳和系统状态
        if (++statusCounter >= 10) {
            statusCounter = 0;
            time_t now = time(nullptr);
            char* timeStr = ctime(&now);
            // 去除换行符
            if (timeStr && timeStr[strlen(timeStr) - 1] == '\n') {
                timeStr[strlen(timeStr) - 1] = '\0';
            }
            cout << "  [心跳] " << timeStr << "\n";
            
            // 显示系统概览
            if (monitoringService) {
                try {
                    auto response = monitoringService->GetSystemOverview();
                    if (response.success) {
                        const auto& overview = response.data;
                        cout << "    └─ 机箱: " << overview.totalChassis 
                             << " | 正常板卡: " << overview.totalNormalBoards 
                             << "/" << overview.totalBoards
                             << " (异常: " << overview.totalAbnormalBoards
                             << ", 离线: " << overview.totalOfflineBoards << ")";
                        
                        // 获取业务链路统计
                        auto stacksResponse = monitoringService->GetAllStacks();
                        if (stacksResponse.success) {
                            cout << " | 业务链路: " << stacksResponse.data.deployedStacks;
                        }
                        
                        // 获取告警统计
                        auto alertsResponse = monitoringService->GetActiveAlerts();
                        if (alertsResponse.success) {
                            cout << " | 未确认告警: " << alertsResponse.data.unacknowledgedCount;
                        }
                        
                        cout << "\n";
                    } else {
                        cout << "    └─ 获取状态失败: " << response.message << "\n";
                    }
                } catch (const exception& e) {
                    cout << "    └─ 获取状态异常: " << e.what() << "\n";
                }
            }
        }
    }
    
    // 优雅关闭
    bootstrap.Shutdown();
    
    cout << "╔══════════════════════════════════════════════════════════════╗\n";
    cout << "║              程序已退出，感谢使用！                           ║\n";
    cout << "╚══════════════════════════════════════════════════════════════╝\n";
    cout << "\n";
    
    return 0;
}

