#pragma once

#include "../../application/services/alert_service.h"
#include "../../application/services/monitoring_service.h"
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <thread>
#include <atomic>
#include <memory>
#include <string>

namespace zygl::interfaces {

using json = nlohmann::json;

/**
 * @brief WebhookListener - HTTP Webhook监听器
 * 
 * 职责：
 * 1. 接收后端API推送的告警和状态变化通知
 * 2. 解析JSON数据并调用相应的应用服务
 * 3. 支持三种webhook端点：
 *    - POST /webhook/alert - 接收告警
 *    - POST /webhook/status - 接收状态变化
 *    - POST /webhook/board - 接收板卡上下线通知
 * 
 * 线程安全：
 * - 运行在独立线程中（cpp-httplib的HTTP服务器）
 * - 通过std::atomic<bool>控制启停
 */
class WebhookListener {
public:
    /**
     * @brief 构造函数
     * 
     * @param alertService 告警服务
     * @param listenPort 监听端口
     */
    WebhookListener(
        std::shared_ptr<application::AlertService> alertService,
        uint16_t listenPort = 8080)
        : m_alertService(alertService),
          m_listenPort(listenPort),
          m_running(false),
          m_server(std::make_unique<httplib::Server>()) {
        
        SetupRoutes();
    }

    /**
     * @brief 析构函数
     */
    ~WebhookListener() {
        Stop();
    }

    /**
     * @brief 启动监听器
     * 
     * @return true 如果启动成功，false 如果失败
     */
    bool Start() {
        if (m_running.load()) {
            return false;  // 已经在运行
        }

        m_running.store(true);

        // 在独立线程中启动HTTP服务器
        m_serverThread = std::thread([this]() {
            m_server->listen("0.0.0.0", m_listenPort);
        });

        return true;
    }

    /**
     * @brief 停止监听器
     */
    void Stop() {
        if (!m_running.load()) {
            return;  // 未运行
        }

        m_running.store(false);
        
        // 停止HTTP服务器
        if (m_server) {
            m_server->stop();
        }

        if (m_serverThread.joinable()) {
            m_serverThread.join();
        }
    }

    /**
     * @brief 检查是否正在运行
     */
    bool IsRunning() const {
        return m_running.load();
    }

private:
    /**
     * @brief 设置HTTP路由
     */
    void SetupRoutes() {
        // 健康检查端点
        m_server->Get("/health", [](const httplib::Request&, httplib::Response& res) {
            json response = {
                {"status", "ok"},
                {"service", "zygl-webhook-listener"}
            };
            res.set_content(response.dump(), "application/json");
        });

        // 接收告警webhook
        m_server->Post("/webhook/alert", [this](const httplib::Request& req, httplib::Response& res) {
            HandleAlertWebhook(req, res);
        });

        // 接收状态变化webhook
        m_server->Post("/webhook/status", [this](const httplib::Request& req, httplib::Response& res) {
            HandleStatusWebhook(req, res);
        });

        // 接收板卡上下线webhook
        m_server->Post("/webhook/board", [this](const httplib::Request& req, httplib::Response& res) {
            HandleBoardWebhook(req, res);
        });
    }

    /**
     * @brief 处理告警webhook
     * 
     * 请求格式（板卡告警）：
     * {
     *   "alertType": "board",
     *   "boardAddress": "192.168.1.10",
     *   "chassisName": "机箱1",
     *   "chassisNumber": 1,
     *   "boardName": "板卡3",
     *   "boardNumber": 3,
     *   "boardStatus": 1,
     *   "messages": ["CPU使用率过高", "温度异常"]
     * }
     */
    void HandleAlertWebhook(const httplib::Request& req, httplib::Response& res) {
        try {
            json requestData = json::parse(req.body);

            std::string alertType = requestData.value("alertType", "board");
            
            if (alertType == "board") {
                // 板卡告警
                std::string boardAddress = requestData.value("boardAddress", "");
                std::string chassisName = requestData.value("chassisName", "");
                int chassisNumber = requestData.value("chassisNumber", 0);
                std::string boardName = requestData.value("boardName", "");
                int boardNumber = requestData.value("boardNumber", 0);
                int boardStatus = requestData.value("boardStatus", 1);
                std::vector<std::string> messages = requestData.value("messages", std::vector<std::string>{});

                // 调用AlertService处理板卡告警
                auto response = m_alertService->HandleBoardAlert(
                    boardAddress, chassisName, chassisNumber,
                    boardName, boardNumber, boardStatus, messages
                );

                // 返回响应
                json responseData = {
                    {"success", response.success},
                    {"message", response.message},
                    {"alertUUID", response.data}
                };
                res.set_content(responseData.dump(), "application/json");
                res.status = response.success ? 200 : 400;
            } else {
                // 未支持的告警类型
                json errorResponse = {
                    {"success", false},
                    {"message", "不支持的告警类型"}
                };
                res.set_content(errorResponse.dump(), "application/json");
                res.status = 400;
            }

        } catch (const json::exception& e) {
            json errorResponse = {
                {"success", false},
                {"message", std::string("JSON解析错误: ") + e.what()}
            };
            res.set_content(errorResponse.dump(), "application/json");
            res.status = 400;
        }
    }

    /**
     * @brief 处理状态变化webhook
     * 
     * 请求格式：
     * {
     *   "eventType": "stack_status_change",
     *   "stackUUID": "stack-123",
     *   "newStatus": 1,
     *   "timestamp": 1609459200000
     * }
     */
    void HandleStatusWebhook(const httplib::Request& req, httplib::Response& res) {
        try {
            json requestData = json::parse(req.body);

            std::string eventType = requestData.value("eventType", "");
            std::string stackUUID = requestData.value("stackUUID", "");
            int newStatus = requestData.value("newStatus", 0);
            uint64_t timestamp = requestData.value("timestamp", 0ULL);

            // 根据eventType处理不同的状态变化
            // 这里可以根据需要调用相应的服务来处理状态变化
            // 目前只是记录日志并返回成功

            json responseData = {
                {"success", true},
                {"message", "状态变化已接收"}
            };
            res.set_content(responseData.dump(), "application/json");
            res.status = 200;

        } catch (const json::exception& e) {
            json errorResponse = {
                {"success", false},
                {"message", std::string("JSON解析错误: ") + e.what()}
            };
            res.set_content(errorResponse.dump(), "application/json");
            res.status = 400;
        }
    }

    /**
     * @brief 处理板卡上下线webhook
     * 
     * 请求格式：
     * {
     *   "boardAddress": "192.168.1.10",
     *   "chassisNumber": 1,
     *   "slotNumber": 3,
     *   "eventType": "offline",
     *   "timestamp": 1609459200000
     * }
     */
    void HandleBoardWebhook(const httplib::Request& req, httplib::Response& res) {
        try {
            json requestData = json::parse(req.body);

            std::string boardAddress = requestData.value("boardAddress", "");
            int chassisNumber = requestData.value("chassisNumber", 0);
            int slotNumber = requestData.value("slotNumber", 0);
            std::string eventType = requestData.value("eventType", "");
            uint64_t timestamp = requestData.value("timestamp", 0ULL);

            // 根据eventType处理板卡上下线
            // 如果是offline，应该创建一个告警
            if (eventType == "offline") {
                std::string chassisName = "机箱" + std::to_string(chassisNumber);
                std::string boardName = "槽位" + std::to_string(slotNumber);
                std::vector<std::string> messages = {"板卡离线"};
                
                auto response = m_alertService->HandleBoardAlert(
                    boardAddress,
                    chassisName,
                    chassisNumber,
                    boardName,
                    slotNumber,
                    2,  // 离线状态
                    messages
                );

                json responseData = {
                    {"success", response.success},
                    {"message", response.message},
                    {"alertUUID", response.data}
                };
                res.set_content(responseData.dump(), "application/json");
                res.status = response.success ? 200 : 400;
            } else {
                json responseData = {
                    {"success", true},
                    {"message", "板卡状态变化已接收"}
                };
                res.set_content(responseData.dump(), "application/json");
                res.status = 200;
            }

        } catch (const json::exception& e) {
            json errorResponse = {
                {"success", false},
                {"message", std::string("JSON解析错误: ") + e.what()}
            };
            res.set_content(errorResponse.dump(), "application/json");
            res.status = 400;
        }
    }

private:
    // 依赖服务
    std::shared_ptr<application::AlertService> m_alertService;
    
    // 配置参数
    uint16_t m_listenPort;                      // 监听端口
    
    // 运行状态
    std::atomic<bool> m_running;                // 是否正在运行
    std::thread m_serverThread;                 // 服务器线程
    
    // HTTP服务器
    std::unique_ptr<httplib::Server> m_server;  // cpp-httplib服务器实例
};

} // namespace zygl::interfaces

