#pragma once

#include "../../../third_party/json.hpp"
#include <string>
#include <fstream>
#include <iostream>
#include <optional>

namespace zygl::infrastructure {

/**
 * @brief 系统配置结构
 */
struct SystemConfig {
    // 后端API配置
    struct {
        std::string apiUrl = "http://localhost:8080";
        int timeoutSeconds = 10;
    } backend;
    
    // 数据采集配置
    struct {
        int intervalSeconds = 5;
    } dataCollector;
    
    // UDP通信配置
    struct {
        std::string multicastAddress = "239.0.0.1";
        int stateBroadcastPort = 5000;
        int commandListenerPort = 5001;
        int broadcastIntervalMs = 1000;
    } udp;
    
    // Webhook配置
    struct {
        int listenPort = 9000;
    } webhook;
    
    // 硬件拓扑配置
    struct {
        int chassisCount = 9;
        int boardsPerChassis = 14;
        std::string ipBasePattern = "192.168.%d";
        int ipOffset = 100;
    } hardware;
    
    // 系统限制配置
    struct {
        int maxTasksPerBoard = 8;
        int maxLabelsPerStack = 8;
        int maxAlertMessages = 16;
    } limits;
};

/**
 * @brief 配置加载器
 * 
 * 职责：
 * 1. 从 JSON 配置文件加载配置
 * 2. 验证配置有效性
 * 3. 提供默认配置
 */
class ConfigLoader {
public:
    /**
     * @brief 从文件加载配置
     * 
     * @param configPath 配置文件路径
     * @return 配置对象，如果加载失败返回默认配置
     */
    static SystemConfig LoadFromFile(const std::string& configPath) {
        SystemConfig config;
        
        try {
            // 打开配置文件
            std::ifstream file(configPath);
            if (!file.is_open()) {
                std::cerr << "⚠️  无法打开配置文件: " << configPath << std::endl;
                std::cerr << "   使用默认配置" << std::endl;
                return config;
            }
            
            // 解析 JSON
            nlohmann::json j;
            file >> j;
            
            // 读取后端配置
            if (j.contains("backend")) {
                auto& backend = j["backend"];
                if (backend.contains("api_url")) {
                    config.backend.apiUrl = backend["api_url"].get<std::string>();
                }
                if (backend.contains("timeout_seconds")) {
                    config.backend.timeoutSeconds = backend["timeout_seconds"].get<int>();
                }
            }
            
            // 读取数据采集配置
            if (j.contains("data_collector")) {
                auto& dc = j["data_collector"];
                if (dc.contains("interval_seconds")) {
                    config.dataCollector.intervalSeconds = dc["interval_seconds"].get<int>();
                }
            }
            
            // 读取UDP配置
            if (j.contains("udp")) {
                auto& udp = j["udp"];
                if (udp.contains("multicast_address")) {
                    config.udp.multicastAddress = udp["multicast_address"].get<std::string>();
                }
                if (udp.contains("state_broadcast_port")) {
                    config.udp.stateBroadcastPort = udp["state_broadcast_port"].get<int>();
                }
                if (udp.contains("command_listener_port")) {
                    config.udp.commandListenerPort = udp["command_listener_port"].get<int>();
                }
                if (udp.contains("broadcast_interval_ms")) {
                    config.udp.broadcastIntervalMs = udp["broadcast_interval_ms"].get<int>();
                }
            }
            
            // 读取Webhook配置
            if (j.contains("webhook")) {
                auto& webhook = j["webhook"];
                if (webhook.contains("listen_port")) {
                    config.webhook.listenPort = webhook["listen_port"].get<int>();
                }
            }
            
            // 读取硬件配置
            if (j.contains("hardware")) {
                auto& hw = j["hardware"];
                if (hw.contains("chassis_count")) {
                    config.hardware.chassisCount = hw["chassis_count"].get<int>();
                }
                if (hw.contains("boards_per_chassis")) {
                    config.hardware.boardsPerChassis = hw["boards_per_chassis"].get<int>();
                }
                if (hw.contains("ip_base_pattern")) {
                    config.hardware.ipBasePattern = hw["ip_base_pattern"].get<std::string>();
                }
                if (hw.contains("ip_offset")) {
                    config.hardware.ipOffset = hw["ip_offset"].get<int>();
                }
            }
            
            // 读取限制配置
            if (j.contains("limits")) {
                auto& limits = j["limits"];
                if (limits.contains("max_tasks_per_board")) {
                    config.limits.maxTasksPerBoard = limits["max_tasks_per_board"].get<int>();
                }
                if (limits.contains("max_labels_per_stack")) {
                    config.limits.maxLabelsPerStack = limits["max_labels_per_stack"].get<int>();
                }
                if (limits.contains("max_alert_messages")) {
                    config.limits.maxAlertMessages = limits["max_alert_messages"].get<int>();
                }
            }
            
            std::cout << "✅ 配置文件加载成功: " << configPath << std::endl;
            
        } catch (const nlohmann::json::exception& e) {
            std::cerr << "⚠️  配置文件解析错误: " << e.what() << std::endl;
            std::cerr << "   使用默认配置" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "⚠️  配置加载异常: " << e.what() << std::endl;
            std::cerr << "   使用默认配置" << std::endl;
        }
        
        return config;
    }
    
    /**
     * @brief 获取默认配置
     */
    static SystemConfig GetDefaultConfig() {
        return SystemConfig();
    }
    
    /**
     * @brief 验证配置有效性
     */
    static bool ValidateConfig(const SystemConfig& config) {
        bool valid = true;
        
        // 验证端口范围
        if (config.udp.stateBroadcastPort < 1024 || config.udp.stateBroadcastPort > 65535) {
            std::cerr << "❌ 配置错误: UDP状态广播端口无效 (" << config.udp.stateBroadcastPort << ")" << std::endl;
            valid = false;
        }
        
        if (config.udp.commandListenerPort < 1024 || config.udp.commandListenerPort > 65535) {
            std::cerr << "❌ 配置错误: UDP命令监听端口无效 (" << config.udp.commandListenerPort << ")" << std::endl;
            valid = false;
        }
        
        if (config.webhook.listenPort < 1024 || config.webhook.listenPort > 65535) {
            std::cerr << "❌ 配置错误: Webhook监听端口无效 (" << config.webhook.listenPort << ")" << std::endl;
            valid = false;
        }
        
        // 验证间隔时间
        if (config.dataCollector.intervalSeconds < 1) {
            std::cerr << "❌ 配置错误: 数据采集间隔必须 >= 1秒" << std::endl;
            valid = false;
        }
        
        if (config.udp.broadcastIntervalMs < 100) {
            std::cerr << "❌ 配置错误: 广播间隔必须 >= 100ms" << std::endl;
            valid = false;
        }
        
        // 验证硬件配置
        if (config.hardware.chassisCount < 1 || config.hardware.chassisCount > 100) {
            std::cerr << "❌ 配置错误: 机箱数量无效 (" << config.hardware.chassisCount << ")" << std::endl;
            valid = false;
        }
        
        if (config.hardware.boardsPerChassis < 1 || config.hardware.boardsPerChassis > 100) {
            std::cerr << "❌ 配置错误: 每机箱板卡数无效 (" << config.hardware.boardsPerChassis << ")" << std::endl;
            valid = false;
        }
        
        return valid;
    }
    
    /**
     * @brief 打印配置信息
     */
    static void PrintConfig(const SystemConfig& config) {
        std::cout << "【当前配置】\n";
        std::cout << "  后端API:\n";
        std::cout << "    - 地址: " << config.backend.apiUrl << "\n";
        std::cout << "    - 超时: " << config.backend.timeoutSeconds << "秒\n";
        std::cout << "  数据采集:\n";
        std::cout << "    - 间隔: " << config.dataCollector.intervalSeconds << "秒\n";
        std::cout << "  UDP通信:\n";
        std::cout << "    - 组播地址: " << config.udp.multicastAddress << "\n";
        std::cout << "    - 状态广播端口: " << config.udp.stateBroadcastPort << "\n";
        std::cout << "    - 命令监听端口: " << config.udp.commandListenerPort << "\n";
        std::cout << "    - 广播间隔: " << config.udp.broadcastIntervalMs << "ms\n";
        std::cout << "  Webhook:\n";
        std::cout << "    - 监听端口: " << config.webhook.listenPort << "\n";
        std::cout << "  硬件拓扑:\n";
        std::cout << "    - 机箱数量: " << config.hardware.chassisCount << "\n";
        std::cout << "    - 每机箱板卡数: " << config.hardware.boardsPerChassis << "\n";
        std::cout << "\n";
    }
};

} // namespace zygl::infrastructure

