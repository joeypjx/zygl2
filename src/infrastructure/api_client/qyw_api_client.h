#pragma once

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <sstream>
#include <iostream>

// HTTP库和JSON库
#include "../../../third_party/httplib.h"
#include "../../../third_party/json.hpp"

namespace zygl::infrastructure {

/**
 * @brief API响应结构（通用）
 */
struct ApiResponse {
    int statusCode;         // HTTP状态码
    std::string body;       // 响应体（JSON字符串）
    bool success;           // 是否成功
    std::string errorMsg;   // 错误消息
};

/**
 * @brief BoardInfo原始数据结构（从API返回）
 */
struct BoardInfoData {
    std::string chassisName;
    int chassisNumber;
    std::string boardName;
    int boardNumber;
    int boardType;
    std::string boardAddress;
    int boardStatus;
    
    struct TaskInfo {
        std::string taskID;
        std::string taskStatus;
        std::string serviceName;
        std::string serviceUUID;
        std::string stackName;
        std::string stackUUID;
    };
    
    std::vector<TaskInfo> taskInfos;
};

/**
 * @brief StackInfo原始数据结构（从API返回）
 */
struct StackInfoData {
    std::string stackName;
    std::string stackUUID;
    int stackDeployStatus;
    int stackRunningStatus;
    
    struct LabelInfo {
        std::string labelName;
        std::string labelUUID;
    };
    std::vector<LabelInfo> stackLabelInfos;
    
    struct ServiceInfo {
        std::string serviceName;
        std::string serviceUUID;
        int serviceStatus;
        int serviceType;
        
        struct TaskInfo {
            std::string taskID;
            std::string taskStatus;
            float cpuCores;
            float cpuUsed;
            float cpuUsage;
            float memorySize;
            float memoryUsed;
            float memoryUsage;
            float netReceive;
            float netSent;
            float gpuMemUsed;
            std::string chassisName;
            int chassisNumber;
            std::string boardName;
            int boardNumber;
            std::string boardAddress;
        };
        
        std::vector<TaskInfo> taskInfos;
    };
    
    std::vector<ServiceInfo> serviceInfos;
};

/**
 * @brief Deploy/Undeploy响应结构
 */
struct DeployResponse {
    struct StackResult {
        std::string stackName;
        std::string stackUUID;
        std::string message;
    };
    
    std::vector<StackResult> successStackInfos;
    std::vector<StackResult> failureStackInfos;
};

/**
 * @brief QywApiClient - 后端API客户端
 * 
 * 职责：
 * 1. 封装对后端API的HTTP调用
 * 2. 解析JSON响应为C++结构
 * 3. 错误处理和重试
 * 
 * 使用cpp-httplib库进行HTTP通信。
 * 
 * 注意：本头文件提供接口定义，实际实现需要：
 * 1. 引入cpp-httplib库
 * 2. 引入JSON解析库（如nlohmann/json）
 * 3. 实现JSON到C++结构的转换
 */
class QywApiClient {
public:
    /**
     * @brief 构造函数
     * @param baseUrl API基础URL（如 "http://192.168.1.100:8080"）
     * @param timeout 超时时间（秒），默认10秒
     */
    explicit QywApiClient(const std::string& baseUrl, int timeout = 10)
        : m_baseUrl(baseUrl), m_timeout(timeout) {
        InitializeClient();
    }

    /**
     * @brief 获取所有板卡信息和状态
     * 
     * 接口：GET /api/v1/external/qyw/boardinfo
     * 
     * @return 板卡信息列表，如果失败返回空optional
     */
    std::optional<std::vector<BoardInfoData>> GetBoardInfo() const {
        try {
            // 发送GET请求（复用HTTP客户端）
            auto res = m_client->Get("/api/v1/external/qyw/boardinfo");
            
            if (!res) {
                std::cerr << "GetBoardInfo: 请求失败 - 无响应" << std::endl;
                return std::nullopt;
            }
            
            if (res->status != 200) {
                std::cerr << "GetBoardInfo: HTTP错误 " << res->status << std::endl;
                return std::nullopt;
            }
            
            // 解析JSON响应
            return ParseBoardInfoResponse(res->body);
            
        } catch (const std::exception& e) {
            std::cerr << "GetBoardInfo: 异常 - " << e.what() << std::endl;
            return std::nullopt;
        }
    }

    /**
     * @brief 获取所有业务链路详情
     * 
     * 接口：GET /api/v1/external/qyw/stackinfo
     * 
     * @return 业务链路信息列表，如果失败返回空optional
     */
    std::optional<std::vector<StackInfoData>> GetStackInfo() const {
        try {
            // 发送GET请求（复用HTTP客户端）
            auto res = m_client->Get("/api/v1/external/qyw/stackinfo");
            
            if (!res) {
                std::cerr << "GetStackInfo: 请求失败 - 无响应" << std::endl;
                return std::nullopt;
            }
            
            if (res->status != 200) {
                std::cerr << "GetStackInfo: HTTP错误 " << res->status << std::endl;
                return std::nullopt;
            }
            
            // 解析JSON响应
            return ParseStackInfoResponse(res->body);
            
        } catch (const std::exception& e) {
            std::cerr << "GetStackInfo: 异常 - " << e.what() << std::endl;
            return std::nullopt;
        }
    }

    /**
     * @brief 批量启用业务链路
     * 
     * 接口：POST /api/v1/external/qyw/deploy
     * 
     * @param stackLabels 业务链路标签UUID列表
     * @return 部署结果（成功和失败的业务链路），如果失败返回空optional
     */
    std::optional<DeployResponse> Deploy(const std::vector<std::string>& stackLabels) const {
        try {
            // 构建JSON请求体
            nlohmann::json body;
            body["stackLabels"] = stackLabels;
            std::string jsonStr = body.dump();
            
            // 发送POST请求（复用HTTP客户端）
            auto res = m_client->Post("/api/v1/external/qyw/deploy",
                                     jsonStr,
                                     "application/json");
            
            if (!res) {
                std::cerr << "Deploy: 请求失败 - 无响应" << std::endl;
                return std::nullopt;
            }
            
            if (res->status != 200) {
                std::cerr << "Deploy: HTTP错误 " << res->status << std::endl;
                return std::nullopt;
            }
            
            // 解析JSON响应
            return ParseDeployResponse(res->body);
            
        } catch (const std::exception& e) {
            std::cerr << "Deploy: 异常 - " << e.what() << std::endl;
            return std::nullopt;
        }
    }

    /**
     * @brief 批量停用业务链路
     * 
     * 接口：POST /api/v1/external/qyw/undeploy
     * 
     * @param stackLabels 业务链路标签UUID列表
     * @return 停用结果（成功和失败的业务链路），如果失败返回空optional
     */
    std::optional<DeployResponse> Undeploy(const std::vector<std::string>& stackLabels) const {
        try {
            // 构建JSON请求体
            nlohmann::json body;
            body["stackLabels"] = stackLabels;
            std::string jsonStr = body.dump();
            
            // 发送POST请求（复用HTTP客户端）
            auto res = m_client->Post("/api/v1/external/qyw/undeploy",
                                     jsonStr,
                                     "application/json");
            
            if (!res) {
                std::cerr << "Undeploy: 请求失败 - 无响应" << std::endl;
                return std::nullopt;
            }
            
            if (res->status != 200) {
                std::cerr << "Undeploy: HTTP错误 " << res->status << std::endl;
                return std::nullopt;
            }
            
            // 解析JSON响应
            return ParseDeployResponse(res->body);
            
        } catch (const std::exception& e) {
            std::cerr << "Undeploy: 异常 - " << e.what() << std::endl;
            return std::nullopt;
        }
    }

    /**
     * @brief 测试连接
     * @return true 如果能成功连接到API服务器
     */
    bool TestConnection() const {
        try {
            auto res = m_client->Get("/api/v1/external/qyw/boardinfo");
            
            // 只要能连接上并收到响应（200或其他状态码），都认为连接成功
            return res && (res->status == 200 || res->status == 401 || res->status >= 100);
        } catch (const std::exception& e) {
            std::cerr << "TestConnection: 异常 - " << e.what() << std::endl;
            return false;
        }
    }

    /**
     * @brief 设置超时时间
     * @param timeout 超时时间（秒）
     */
    void SetTimeout(int timeout) {
        m_timeout = timeout;
        // 重新初始化客户端以应用新超时
        InitializeClient();
    }

    /**
     * @brief 获取基础URL
     */
    const std::string& GetBaseUrl() const {
        return m_baseUrl;
    }

private:
    std::string m_baseUrl;      // API基础URL
    int m_timeout;              // 超时时间（秒）
    mutable std::unique_ptr<httplib::Client> m_client;  // 复用的HTTP客户端
    
    /**
     * @brief 初始化HTTP客户端
     */
    void InitializeClient() const {
        m_client = std::make_unique<httplib::Client>(m_baseUrl);
        m_client->set_connection_timeout(0, m_timeout * 1000000);  // 微秒
        m_client->set_read_timeout(m_timeout, 0);  // 秒
    }

    /**
     * @brief 解析板卡信息JSON响应
     */
    std::optional<std::vector<BoardInfoData>> ParseBoardInfoResponse(const std::string& jsonStr) const {
        try {
            auto json = nlohmann::json::parse(jsonStr);
            
            // 检查响应格式
            if (!json.contains("data") || !json["data"].is_array()) {
                std::cerr << "ParseBoardInfoResponse: 响应格式错误" << std::endl;
                return std::nullopt;
            }
            
            std::vector<BoardInfoData> result;
            
            for (const auto& item : json["data"]) {
                BoardInfoData boardInfo;
                
                // 解析基本信息
                boardInfo.chassisName = item.value("chassisName", "");
                boardInfo.chassisNumber = item.value("chassisNumber", 0);
                boardInfo.boardName = item.value("boardName", "");
                boardInfo.boardNumber = item.value("boardNumber", 0);
                boardInfo.boardType = item.value("boardType", 0);
                boardInfo.boardAddress = item.value("boardAddress", "");
                boardInfo.boardStatus = item.value("boardStatus", 0);
                
                // 解析任务列表
                if (item.contains("taskInfos") && item["taskInfos"].is_array()) {
                    for (const auto& taskItem : item["taskInfos"]) {
                        BoardInfoData::TaskInfo task;
                        task.taskID = taskItem.value("taskID", "");
                        task.taskStatus = taskItem.value("taskStatus", "");
                        task.serviceName = taskItem.value("serviceName", "");
                        task.serviceUUID = taskItem.value("serviceUUID", "");
                        task.stackName = taskItem.value("stackName", "");
                        task.stackUUID = taskItem.value("stackUUID", "");
                        boardInfo.taskInfos.push_back(task);
                    }
                }
                
                result.push_back(boardInfo);
            }
            
            return result;
            
        } catch (const nlohmann::json::exception& e) {
            std::cerr << "ParseBoardInfoResponse: JSON解析错误 - " << e.what() << std::endl;
            return std::nullopt;
        }
    }

    /**
     * @brief 解析业务链路信息JSON响应
     */
    std::optional<std::vector<StackInfoData>> ParseStackInfoResponse(const std::string& jsonStr) const {
        try {
            auto json = nlohmann::json::parse(jsonStr);
            
            if (!json.contains("data") || !json["data"].is_array()) {
                std::cerr << "ParseStackInfoResponse: 响应格式错误" << std::endl;
                return std::nullopt;
            }
            
            std::vector<StackInfoData> result;
            
            for (const auto& item : json["data"]) {
                StackInfoData stackInfo;
                
                // 解析基本信息
                stackInfo.stackName = item.value("stackName", "");
                stackInfo.stackUUID = item.value("stackUUID", "");
                stackInfo.stackDeployStatus = item.value("stackDeployStatus", 0);
                stackInfo.stackRunningStatus = item.value("stackRunningStatus", 1);
                
                // 解析标签
                if (item.contains("stackLabelInfos") && item["stackLabelInfos"].is_array()) {
                    for (const auto& labelItem : item["stackLabelInfos"]) {
                        StackInfoData::LabelInfo label;
                        label.labelName = labelItem.value("labelName", "");
                        label.labelUUID = labelItem.value("labelUUID", "");
                        stackInfo.stackLabelInfos.push_back(label);
                    }
                }
                
                // 解析组件
                if (item.contains("serviceInfos") && item["serviceInfos"].is_array()) {
                    for (const auto& serviceItem : item["serviceInfos"]) {
                        StackInfoData::ServiceInfo service;
                        service.serviceName = serviceItem.value("serviceName", "");
                        service.serviceUUID = serviceItem.value("serviceUUID", "");
                        service.serviceStatus = serviceItem.value("serviceStatus", 0);
                        service.serviceType = serviceItem.value("serviceType", 0);
                        
                        // 解析任务
                        if (serviceItem.contains("taskInfos") && serviceItem["taskInfos"].is_array()) {
                            for (const auto& taskItem : serviceItem["taskInfos"]) {
                                StackInfoData::ServiceInfo::TaskInfo task;
                                task.taskID = taskItem.value("taskID", "");
                                task.taskStatus = taskItem.value("taskStatus", "");
                                task.cpuCores = taskItem.value("cpuCores", 0.0f);
                                task.cpuUsed = taskItem.value("cpuUsed", 0.0f);
                                task.cpuUsage = taskItem.value("cpuUsage", 0.0f);
                                task.memorySize = taskItem.value("memorySize", 0.0f);
                                task.memoryUsed = taskItem.value("memoryUsed", 0.0f);
                                task.memoryUsage = taskItem.value("memoryUsage", 0.0f);
                                task.netReceive = taskItem.value("netReceive", 0.0f);
                                task.netSent = taskItem.value("netSent", 0.0f);
                                task.gpuMemUsed = taskItem.value("gpuMemUsed", 0.0f);
                                task.chassisName = taskItem.value("chassisName", "");
                                task.chassisNumber = taskItem.value("chassisNumber", 0);
                                task.boardName = taskItem.value("boardName", "");
                                task.boardNumber = taskItem.value("boardNumber", 0);
                                task.boardAddress = taskItem.value("boardAddress", "");
                                service.taskInfos.push_back(task);
                            }
                        }
                        
                        stackInfo.serviceInfos.push_back(service);
                    }
                }
                
                result.push_back(stackInfo);
            }
            
            return result;
            
        } catch (const nlohmann::json::exception& e) {
            std::cerr << "ParseStackInfoResponse: JSON解析错误 - " << e.what() << std::endl;
            return std::nullopt;
        }
    }

    /**
     * @brief 解析Deploy/Undeploy响应
     */
    std::optional<DeployResponse> ParseDeployResponse(const std::string& jsonStr) const {
        try {
            auto json = nlohmann::json::parse(jsonStr);
            
            DeployResponse response;
            
            // 解析成功的业务链路
            if (json.contains("successStackInfos") && json["successStackInfos"].is_array()) {
                for (const auto& item : json["successStackInfos"]) {
                    DeployResponse::StackResult result;
                    result.stackName = item.value("stackName", "");
                    result.stackUUID = item.value("stackUUID", "");
                    result.message = item.value("message", "");
                    response.successStackInfos.push_back(result);
                }
            }
            
            // 解析失败的业务链路
            if (json.contains("failureStackInfos") && json["failureStackInfos"].is_array()) {
                for (const auto& item : json["failureStackInfos"]) {
                    DeployResponse::StackResult result;
                    result.stackName = item.value("stackName", "");
                    result.stackUUID = item.value("stackUUID", "");
                    result.message = item.value("message", "");
                    response.failureStackInfos.push_back(result);
                }
            }
            
            return response;
            
        } catch (const nlohmann::json::exception& e) {
            std::cerr << "ParseDeployResponse: JSON解析错误 - " << e.what() << std::endl;
            return std::nullopt;
        }
    }
};

} // namespace zygl::infrastructure

