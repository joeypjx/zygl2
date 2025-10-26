#pragma once

#include <string>
#include <vector>
#include <optional>
#include <memory>

// 注意：实际使用时需要包含cpp-httplib
// #include <httplib.h>

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
    }

    /**
     * @brief 获取所有板卡信息和状态
     * 
     * 接口：GET /api/v1/external/qyw/boardinfo
     * 
     * @return 板卡信息列表，如果失败返回空optional
     */
    std::optional<std::vector<BoardInfoData>> GetBoardInfo() const {
        // TODO: 实现HTTP GET请求
        // std::string url = m_baseUrl + "/api/v1/external/qyw/boardinfo";
        // httplib::Client client(m_baseUrl);
        // client.set_connection_timeout(m_timeout);
        // auto res = client.Get("/api/v1/external/qyw/boardinfo");
        // if (res && res->status == 200) {
        //     return ParseBoardInfoResponse(res->body);
        // }
        return std::nullopt;
    }

    /**
     * @brief 获取所有业务链路详情
     * 
     * 接口：GET /api/v1/external/qyw/stackinfo
     * 
     * @return 业务链路信息列表，如果失败返回空optional
     */
    std::optional<std::vector<StackInfoData>> GetStackInfo() const {
        // TODO: 实现HTTP GET请求
        // std::string url = m_baseUrl + "/api/v1/external/qyw/stackinfo";
        // httplib::Client client(m_baseUrl);
        // client.set_connection_timeout(m_timeout);
        // auto res = client.Get("/api/v1/external/qyw/stackinfo");
        // if (res && res->status == 200) {
        //     return ParseStackInfoResponse(res->body);
        // }
        return std::nullopt;
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
        // TODO: 实现HTTP POST请求
        // std::string url = m_baseUrl + "/api/v1/external/qyw/deploy";
        // httplib::Client client(m_baseUrl);
        // client.set_connection_timeout(m_timeout);
        // 
        // // 构建JSON请求体
        // nlohmann::json body;
        // body["stackLabels"] = stackLabels;
        // 
        // auto res = client.Post("/api/v1/external/qyw/deploy", 
        //                       body.dump(), 
        //                       "application/json");
        // if (res && res->status == 200) {
        //     return ParseDeployResponse(res->body);
        // }
        return std::nullopt;
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
        // TODO: 实现HTTP POST请求
        // std::string url = m_baseUrl + "/api/v1/external/qyw/undeploy";
        // httplib::Client client(m_baseUrl);
        // client.set_connection_timeout(m_timeout);
        // 
        // // 构建JSON请求体
        // nlohmann::json body;
        // body["stackLabels"] = stackLabels;
        // 
        // auto res = client.Post("/api/v1/external/qyw/undeploy", 
        //                       body.dump(), 
        //                       "application/json");
        // if (res && res->status == 200) {
        //     return ParseDeployResponse(res->body);
        // }
        return std::nullopt;
    }

    /**
     * @brief 测试连接
     * @return true 如果能成功连接到API服务器
     */
    bool TestConnection() const {
        // TODO: 实现连接测试
        // httplib::Client client(m_baseUrl);
        // client.set_connection_timeout(m_timeout);
        // auto res = client.Get("/api/v1/external/qyw/boardinfo");
        // return res && (res->status == 200 || res->status == 401);
        return false;
    }

    /**
     * @brief 设置超时时间
     * @param timeout 超时时间（秒）
     */
    void SetTimeout(int timeout) {
        m_timeout = timeout;
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

    // TODO: 实现JSON解析方法
    // std::optional<std::vector<BoardInfoData>> ParseBoardInfoResponse(const std::string& json) const;
    // std::optional<std::vector<StackInfoData>> ParseStackInfoResponse(const std::string& json) const;
    // std::optional<DeployResponse> ParseDeployResponse(const std::string& json) const;
};

} // namespace zygl::infrastructure

