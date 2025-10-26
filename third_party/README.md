# 第三方依赖库

本目录包含项目所需的第三方header-only库。

## 依赖库列表

### 1. cpp-httplib (httplib.h)

**版本**: Latest from master branch  
**大小**: 399KB  
**许可证**: MIT License  
**GitHub**: https://github.com/yhirose/cpp-httplib  

**用途**：
- HTTP客户端：用于与后端API通信（QywApiClient）
- HTTP服务器：用于接收Webhook通知（WebhookListener）

**特性**：
- Header-only库，无需编译
- 支持HTTP/HTTPS客户端和服务器
- 支持GET、POST、PUT、DELETE等方法
- 支持JSON数据传输
- 线程安全

**使用方法**：
```cpp
#include "third_party/httplib.h"

// HTTP客户端
httplib::Client cli("http://localhost:8080");
auto res = cli.Get("/api/data");

// HTTP服务器
httplib::Server svr;
svr.Get("/hello", [](const httplib::Request&, httplib::Response& res) {
    res.set_content("Hello World!", "text/plain");
});
svr.listen("0.0.0.0", 8080);
```

---

### 2. nlohmann/json (json.hpp)

**版本**: Latest from develop branch  
**大小**: 940KB  
**许可证**: MIT License  
**GitHub**: https://github.com/nlohmann/json  

**用途**：
- JSON解析：解析后端API返回的JSON数据
- JSON生成：构造请求和响应的JSON数据
- 数据序列化/反序列化

**特性**：
- Header-only库，无需编译
- 现代C++接口（C++11/14/17）
- 直观的API设计
- 完整的JSON标准支持
- 高性能

**使用方法**：
```cpp
#include "third_party/json.hpp"

using json = nlohmann::json;

// 解析JSON
std::string jsonString = R"({"name":"张三","age":30})";
json j = json::parse(jsonString);
std::string name = j["name"];
int age = j["age"];

// 生成JSON
json j2;
j2["name"] = "李四";
j2["age"] = 25;
std::string result = j2.dump();  // {"name":"李四","age":25}

// 使用结构化绑定
json j3 = {
    {"pi", 3.141},
    {"happy", true},
    {"name", "Niels"},
    {"nothing", nullptr},
    {"answer", {{"everything", 42}}},
    {"list", {1, 0, 2}},
    {"object", {{"currency", "USD"}, {"value", 42.99}}}
};
```

---

## 项目中的使用

### Infrastructure层

**QywApiClient** (`src/infrastructure/api_client/qyw_api_client.h`)
```cpp
#include "third_party/httplib.h"
#include "third_party/json.hpp"

class QywApiClient {
    std::unique_ptr<httplib::Client> m_client;
    
    std::optional<BoardInfo> GetBoardInfo() {
        auto res = m_client->Get("/api/boardinfo");
        if (res && res->status == 200) {
            json data = json::parse(res->body);
            // 解析数据...
        }
    }
};
```

**DataCollectorService** (`src/infrastructure/collectors/data_collector_service.h`)
- 使用httplib发起HTTP GET请求
- 使用json解析返回的数据

### Interfaces层

**WebhookListener** (`src/interfaces/http/webhook_listener.h`)
```cpp
#include "third_party/httplib.h"
#include "third_party/json.hpp"

class WebhookListener {
    std::unique_ptr<httplib::Server> m_server;
    
    void SetupRoutes() {
        m_server->Post("/webhook/alert", [](const httplib::Request& req, httplib::Response& res) {
            json requestData = json::parse(req.body);
            // 处理webhook...
            json responseData = {{"success", true}};
            res.set_content(responseData.dump(), "application/json");
        });
    }
};
```

---

## 编译配置

### 包含路径

在编译时添加项目根目录到包含路径：

```bash
# 使用g++
g++ -std=c++17 -I. your_source.cpp -o your_program

# 使用clang++
clang++ -std=c++17 -I. your_source.cpp -o your_program
```

### CMakeLists.txt配置

```cmake
cmake_minimum_required(VERSION 3.10)
project(zygl2)

set(CMAKE_CXX_STANDARD 17)

# 添加包含目录
include_directories(${CMAKE_SOURCE_DIR})

# 添加可执行文件
add_executable(zygl2 
    src/main.cpp
    # ... 其他源文件
)

# 如果需要OpenSSL支持（HTTPS）
find_package(OpenSSL)
if(OPENSSL_FOUND)
    target_link_libraries(zygl2 OpenSSL::SSL OpenSSL::Crypto)
    target_compile_definitions(zygl2 PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)
endif()

# 如果需要zlib支持（压缩）
find_package(ZLIB)
if(ZLIB_FOUND)
    target_link_libraries(zygl2 ZLIB::ZLIB)
    target_compile_definitions(zygl2 PRIVATE CPPHTTPLIB_ZLIB_SUPPORT)
endif()
```

### Makefile配置

```makefile
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -I.
LDFLAGS = -pthread

# 如果需要OpenSSL
# LDFLAGS += -lssl -lcrypto
# CXXFLAGS += -DCPPHTTPLIB_OPENSSL_SUPPORT

SOURCES = src/main.cpp
OBJECTS = $(SOURCES:.cpp=.o)
TARGET = zygl2

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)
```

---

## 可选依赖

### OpenSSL（用于HTTPS支持）

如果需要HTTPS支持，需要安装OpenSSL：

```bash
# macOS
brew install openssl

# Ubuntu/Debian
sudo apt-get install libssl-dev

# CentOS/RHEL
sudo yum install openssl-devel
```

编译时添加：
```bash
g++ -std=c++17 -I. -DCPPHTTPLIB_OPENSSL_SUPPORT your_source.cpp -o your_program -lssl -lcrypto
```

### zlib（用于压缩支持）

如果需要HTTP压缩支持：

```bash
# macOS
brew install zlib

# Ubuntu/Debian
sudo apt-get install zlib1g-dev

# CentOS/RHEL
sudo yum install zlib-devel
```

编译时添加：
```bash
g++ -std=c++17 -I. -DCPPHTTPLIB_ZLIB_SUPPORT your_source.cpp -o your_program -lz
```

---

## 更新依赖库

如果需要更新依赖库到最新版本：

```bash
cd third_party

# 更新cpp-httplib
curl -L -o httplib.h https://raw.githubusercontent.com/yhirose/cpp-httplib/master/httplib.h

# 更新nlohmann/json
curl -L -o json.hpp https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp
```

---

## 许可证

这两个库都采用MIT License，允许商业使用、修改和分发。

详细许可证信息：
- cpp-httplib: https://github.com/yhirose/cpp-httplib/blob/master/LICENSE
- nlohmann/json: https://github.com/nlohmann/json/blob/develop/LICENSE.MIT

---

## 版本信息

| 库名 | 下载时间 | 来源 |
|------|----------|------|
| cpp-httplib | 2025-10-26 | master branch |
| nlohmann/json | 2025-10-26 | develop branch |

---

## 故障排除

### 编译错误：找不到头文件

确保编译时包含了项目根目录：
```bash
g++ -I. ...  # -I. 表示当前目录
```

### 链接错误：pthread相关

添加pthread库：
```bash
g++ ... -pthread
```

### OpenSSL相关错误

如果不需要HTTPS，不要定义`CPPHTTPLIB_OPENSSL_SUPPORT`。
如果需要HTTPS，确保安装了OpenSSL并正确链接。

### 性能问题

1. 使用优化选项：`-O2` 或 `-O3`
2. 使用Release模式编译
3. 考虑启用LTO：`-flto`

---

**最后更新**: 2025-10-26  
**维护者**: zygl2项目团队

