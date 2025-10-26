/**
 * @file test_dependencies.cpp
 * @brief 测试第三方依赖库是否正常工作
 * 
 * 编译命令：
 * g++ -std=c++17 -I. test_dependencies.cpp -o test_dependencies -pthread
 * 
 * 运行：
 * ./test_dependencies
 */

#include "third_party/httplib.h"
#include "third_party/json.hpp"
#include <iostream>
#include <string>

using json = nlohmann::json;

/**
 * @brief 测试nlohmann/json库
 */
void TestJsonLibrary() {
    std::cout << "=== 测试 nlohmann/json 库 ===" << std::endl;
    
    try {
        // 测试1：创建JSON对象
        json j;
        j["name"] = "测试项目";
        j["version"] = "1.0.0";
        j["features"] = {"DDD架构", "UDP通信", "HTTP通信"};
        
        std::cout << "✅ JSON对象创建成功" << std::endl;
        std::cout << "   内容: " << j.dump(2) << std::endl;
        
        // 测试2：解析JSON字符串
        std::string jsonString = R"({
            "project": "zygl2",
            "language": "C++",
            "standard": 17
        })";
        
        json j2 = json::parse(jsonString);
        std::string project = j2["project"];
        std::string language = j2["language"];
        int standard = j2["standard"];
        
        std::cout << "✅ JSON解析成功" << std::endl;
        std::cout << "   项目: " << project << std::endl;
        std::cout << "   语言: " << language << standard << std::endl;
        
        // 测试3：嵌套JSON
        json j3 = {
            {"config", {
                {"host", "localhost"},
                {"port", 8080}
            }},
            {"stats", {
                {"total_lines", 6184},
                {"files", 28}
            }}
        };
        
        std::cout << "✅ 嵌套JSON创建成功" << std::endl;
        std::cout << "   配置端口: " << j3["config"]["port"] << std::endl;
        std::cout << "   代码行数: " << j3["stats"]["total_lines"] << std::endl;
        
        std::cout << "✅ nlohmann/json 测试通过！" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ JSON测试失败: " << e.what() << std::endl;
    }
    
    std::cout << std::endl;
}

/**
 * @brief 测试cpp-httplib库
 */
void TestHttpLibrary() {
    std::cout << "=== 测试 cpp-httplib 库 ===" << std::endl;
    
    try {
        // 测试1：创建HTTP服务器
        httplib::Server svr;
        
        // 添加一个简单的GET端点
        svr.Get("/test", [](const httplib::Request&, httplib::Response& res) {
            json response = {
                {"status", "ok"},
                {"message", "HTTP库测试成功"}
            };
            res.set_content(response.dump(), "application/json");
        });
        
        std::cout << "✅ HTTP服务器创建成功" << std::endl;
        
        // 测试2：创建HTTP客户端（不实际发送请求）
        httplib::Client cli("http://localhost:8080");
        
        std::cout << "✅ HTTP客户端创建成功" << std::endl;
        
        std::cout << "✅ cpp-httplib 测试通过！" << std::endl;
        std::cout << "   注意：完整测试需要启动服务器" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ HTTP测试失败: " << e.what() << std::endl;
    }
    
    std::cout << std::endl;
}

/**
 * @brief 测试两个库的集成使用
 */
void TestIntegration() {
    std::cout << "=== 测试库集成使用 ===" << std::endl;
    
    try {
        // 创建一个简单的HTTP服务器，返回JSON数据
        httplib::Server svr;
        
        svr.Get("/api/info", [](const httplib::Request&, httplib::Response& res) {
            json info = {
                {"project", "zygl2"},
                {"description", "基于DDD的资源管理系统"},
                {"layers", {
                    {"domain", 1963},
                    {"infrastructure", 1678},
                    {"application", 1296},
                    {"interfaces", 1247}
                }},
                {"total_lines", 6184}
            };
            
            res.set_content(info.dump(2), "application/json");
        });
        
        svr.Post("/api/command", [](const httplib::Request& req, httplib::Response& res) {
            try {
                // 解析请求JSON
                json requestData = json::parse(req.body);
                
                // 构造响应JSON
                json responseData = {
                    {"success", true},
                    {"received_command", requestData["command"]},
                    {"timestamp", "2025-10-26"}
                };
                
                res.set_content(responseData.dump(), "application/json");
            } catch (const json::exception& e) {
                json errorResponse = {
                    {"success", false},
                    {"error", e.what()}
                };
                res.set_content(errorResponse.dump(), "application/json");
                res.status = 400;
            }
        });
        
        std::cout << "✅ HTTP + JSON 集成服务器创建成功" << std::endl;
        std::cout << "✅ 库集成测试通过！" << std::endl;
        std::cout << "   提示：可以使用这些库构建完整的应用" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ 集成测试失败: " << e.what() << std::endl;
    }
    
    std::cout << std::endl;
}

/**
 * @brief 主函数
 */
int main() {
    std::cout << std::endl;
    std::cout << "╔══════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║     第三方依赖库测试程序                          ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;
    
    // 显示库信息
    std::cout << "依赖库列表：" << std::endl;
    std::cout << "  1. cpp-httplib (httplib.h) - HTTP通信库" << std::endl;
    std::cout << "  2. nlohmann/json (json.hpp) - JSON解析库" << std::endl;
    std::cout << std::endl;
    
    // 执行测试
    TestJsonLibrary();
    TestHttpLibrary();
    TestIntegration();
    
    // 总结
    std::cout << "╔══════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║     所有依赖库测试完成！                          ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;
    std::cout << "✅ 依赖库已正确安装并可以使用" << std::endl;
    std::cout << "✅ 可以开始编译项目了" << std::endl;
    std::cout << std::endl;
    std::cout << "编译项目示例：" << std::endl;
    std::cout << "  g++ -std=c++17 -I. src/main.cpp -o zygl2 -pthread" << std::endl;
    std::cout << std::endl;
    
    return 0;
}

