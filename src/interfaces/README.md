# Interfaces层实现说明

## 概述

Interfaces层（接口层）是系统的最外层，负责处理所有外部通信。本层实现了与前端显示软件（UDP多播）和后端API（HTTP Webhook）的通信协议。

## 设计原则

### 1. 职责分离
- **状态广播**：`StateBroadcaster` 负责定期向前端多播系统状态
- **命令接收**：`CommandListener` 负责接收并处理前端发送的控制命令
- **Webhook接收**：`WebhookListener` 负责接收后端API的推送通知

### 2. 协议设计
- **UDP多播**：使用原始数据对象（`#pragma pack(1)`）确保内存布局可预测
- **字节序处理**：所有网络传输的整数类型使用网络字节序（大端）
- **固定大小数组**：使用`char[]`和固定大小数组，避免动态内存分配
- **数据包头**：所有UDP包都包含统一的包头（类型、版本、序列号、时间戳）

### 3. 线程模型
- 每个组件运行在独立线程中
- 使用`std::atomic<bool>`控制启停
- 线程安全地访问Application层服务

## 目录结构

```
src/interfaces/
├── interfaces.h              # Interfaces层统一头文件
├── README.md                 # 本文档
├── udp/                      # UDP通信相关
│   ├── udp_protocol.h        # UDP协议定义（数据包格式）
│   ├── state_broadcaster.h   # 状态广播器
│   └── command_listener.h    # 命令监听器
└── http/                     # HTTP通信相关
    └── webhook_listener.h    # Webhook监听器
```

## 核心组件

### 1. UDP通信协议 (`udp_protocol.h`)

定义了所有UDP通信的数据包格式。

#### 多播配置
```cpp
MULTICAST_GROUP = "239.255.0.1"  // 多播组地址
STATE_BROADCAST_PORT = 9001       // 状态广播端口
COMMAND_LISTEN_PORT = 9002        // 命令监听端口
```

#### 数据包类型

**状态广播包（服务端 -> 前端）**
- `ChassisState (0x0001)`: 机箱/板卡状态包
- `AlertMessage (0x0002)`: 告警消息包
- `StackLabel (0x0003)`: 业务链标签包

**命令包（前端 -> 服务端）**
- `DeployStack (0x1001)`: 部署业务链命令
- `UndeployStack (0x1002)`: 卸载业务链命令
- `AcknowledgeAlert (0x1003)`: 确认告警命令

**响应包（服务端 -> 前端）**
- `CommandResponse (0x2001)`: 命令响应包

#### 数据包结构

所有数据包使用`#pragma pack(1)`确保内存对齐：

```cpp
struct UdpPacketHeader {
    uint16_t packetType;        // 数据包类型
    uint16_t version;           // 协议版本
    uint32_t sequenceNumber;    // 序列号
    uint64_t timestamp;         // 时间戳（毫秒）
    uint32_t dataLength;        // 数据长度
    char reserved[4];           // 保留字段
};
```

**机箱状态包**：包含1个机箱的完整信息（14个板卡）
**告警消息包**：包含最多32条告警
**业务链标签包**：包含最多64个业务链的标签信息

### 2. 状态广播器 (`StateBroadcaster`)

#### 功能
定期向前端多播系统状态，包括：
- 机箱/板卡状态（每个机箱一个数据包）
- 告警消息（每包最多32条）
- 业务链标签（每包最多64个业务链）

#### 实现要点
```cpp
// 创建广播器
auto broadcaster = std::make_shared<StateBroadcaster>(
    monitoringService,
    1000,  // 机箱状态广播间隔（1秒）
    2000,  // 告警广播间隔（2秒）
    5000   // 标签广播间隔（5秒）
);

// 启动广播
broadcaster->Start();

// 停止广播
broadcaster->Stop();
```

#### 关键特性
- **独立线程**：运行在单独的线程中，定期广播
- **多播发送**：使用UDP多播协议，支持多个前端同时接收
- **可配置间隔**：三种状态的广播周期可独立配置
- **序列号管理**：自动递增序列号，用于检测丢包

### 3. 命令监听器 (`CommandListener`)

#### 功能
监听前端发送的控制命令，并返回执行结果：
- **部署业务链**：根据标签UUID批量部署
- **卸载业务链**：根据标签UUID批量卸载
- **确认告警**：标记告警为已确认

#### 实现要点
```cpp
// 创建监听器
auto listener = std::make_shared<CommandListener>(
    stackControlService,
    alertService
);

// 启动监听
listener->Start();

// 停止监听
listener->Stop();
```

#### 关键特性
- **多播接收**：加入多播组，接收前端命令
- **命令分发**：根据数据包类型分发到相应的处理函数
- **响应反馈**：执行命令后立即发送响应包到前端
- **错误处理**：捕获异常并返回错误信息

#### 命令反馈机制
每个命令包含唯一的`commandID`，响应包使用相同的`commandID`进行匹配：

```cpp
struct CommandResponsePacket {
    uint64_t commandID;          // 命令ID（匹配请求）
    uint16_t originalCommandType; // 原始命令类型
    uint16_t result;             // 命令结果（Success/Failed/...）
    char message[256];           // 结果消息
};
```

### 4. Webhook监听器 (`WebhookListener`)

#### 功能
接收后端API的HTTP推送通知：
- **告警webhook** (`POST /webhook/alert`): 接收新告警
- **状态变化webhook** (`POST /webhook/status`): 接收状态变化通知
- **板卡上下线webhook** (`POST /webhook/board`): 接收板卡上下线通知

#### 实现要点
```cpp
// 创建监听器
auto webhookListener = std::make_shared<WebhookListener>(
    alertService,
    8080  // 监听端口
);

// 启动监听
webhookListener->Start();

// 停止监听
webhookListener->Stop();
```

#### API端点

**健康检查**
```
GET /health
```

**告警webhook**
```
POST /webhook/alert
Content-Type: application/json

{
  "alertID": "alert-123",
  "alertLevel": 2,
  "alertMessage": "CPU使用率过高",
  "relatedEntity": "chassis-1-board-3",
  "timestamp": 1609459200000
}
```

**状态变化webhook**
```
POST /webhook/status
Content-Type: application/json

{
  "eventType": "stack_status_change",
  "stackUUID": "stack-123",
  "newStatus": 1,
  "timestamp": 1609459200000
}
```

**板卡上下线webhook**
```
POST /webhook/board
Content-Type: application/json

{
  "boardAddress": "192.168.1.10",
  "chassisNumber": 1,
  "slotNumber": 3,
  "eventType": "offline",
  "timestamp": 1609459200000
}
```

#### 关键特性
- **HTTP服务器**：使用`cpp-httplib`库实现HTTP服务器
- **JSON解析**：使用`nlohmann/json`库解析请求数据
- **错误处理**：捕获JSON解析异常并返回适当的HTTP状态码
- **异步处理**：在独立线程中运行HTTP服务器

## 依赖关系

### 外部依赖
- **cpp-httplib**: HTTP服务器库（header-only）
- **nlohmann/json**: JSON解析库（header-only）
- **POSIX socket API**: UDP网络通信

### 内部依赖
- **Application层**: 调用`MonitoringService`、`StackControlService`、`AlertService`
- **Domain层**: 使用`Board`、`Alert`等领域对象和值对象

## 通信流程

### 状态广播流程
1. `StateBroadcaster`定期调用`MonitoringService`获取系统状态
2. 将DTOs转换为UDP数据包格式
3. 通过多播发送到前端

### 命令处理流程
1. 前端发送命令包到多播组
2. `CommandListener`接收并解析命令包
3. 调用相应的Application服务执行业务逻辑
4. 构造响应包并发送到前端

### Webhook处理流程
1. 后端API通过HTTP POST推送通知
2. `WebhookListener`接收并解析JSON数据
3. 调用相应的Application服务处理通知
4. 返回JSON响应和HTTP状态码

## 线程安全

### 线程模型
- `StateBroadcaster`: 单独的广播线程
- `CommandListener`: 单独的监听线程
- `WebhookListener`: cpp-httplib的线程池（每个请求一个线程）

### 并发控制
- Application层的服务使用Repository（带锁保护）
- 多个读取操作可以并发执行
- 写入操作通过Repository的锁机制保证线程安全

## 性能考虑

### UDP多播优化
1. **零拷贝**：直接在栈上构造数据包
2. **固定大小**：避免动态内存分配
3. **批量发送**：告警和标签分批发送，每包最多32/64条

### 网络性能
- **多播**：一次发送，多个前端接收
- **非阻塞**：发送操作不会阻塞主业务逻辑
- **序列号**：用于检测丢包，但不重传（UDP特性）

### 资源使用
- **内存**：固定大小的数据包，可预测的内存使用
- **CPU**：独立线程，不影响主业务处理
- **网络**：根据广播间隔控制带宽使用

## 错误处理

### UDP通信错误
- Socket创建失败：返回false，不启动线程
- 发送失败：静默忽略（UDP特性）
- 接收超时：继续等待下一个数据包

### HTTP通信错误
- JSON解析错误：返回400 Bad Request
- 业务逻辑错误：返回具体的错误信息和状态码
- 服务器启动失败：返回false，不启动线程

## 扩展性

### 新增数据包类型
1. 在`PacketType`枚举中添加新类型
2. 定义新的数据包结构（使用`#pragma pack(1)`）
3. 在相应的组件中添加处理逻辑

### 新增Webhook端点
1. 在`WebhookListener::SetupRoutes()`中添加新路由
2. 实现处理函数
3. 调用相应的Application服务

## 测试建议

### UDP通信测试
- 使用`nc`或自定义工具监听多播组
- 验证数据包格式和内容
- 测试丢包情况下的行为

### HTTP通信测试
- 使用`curl`或Postman发送webhook请求
- 验证JSON格式和响应
- 测试错误处理和边界情况

## 注意事项

1. **字节序**：所有网络传输的整数必须使用网络字节序（大端）
2. **内存对齐**：UDP数据包必须使用`#pragma pack(1)`确保跨平台兼容
3. **字符串终止**：所有`char[]`字段必须正确处理null终止符
4. **UDP可靠性**：UDP不保证数据到达，设计时应考虑丢包情况
5. **多播组管理**：确保前端正确加入多播组才能接收广播

## 代码示例

### 完整的Interfaces层初始化
```cpp
// 创建所有Interfaces层组件
auto broadcaster = std::make_shared<StateBroadcaster>(
    monitoringService, 1000, 2000, 5000);
    
auto commandListener = std::make_shared<CommandListener>(
    stackControlService, alertService);
    
auto webhookListener = std::make_shared<WebhookListener>(
    alertService, 8080);

// 启动所有组件
broadcaster->Start();
commandListener->Start();
webhookListener->Start();

// ... 运行业务逻辑 ...

// 停止所有组件
broadcaster->Stop();
commandListener->Stop();
webhookListener->Stop();
```

## 总结

Interfaces层完成了系统与外部世界的所有通信：
- ✅ UDP多播状态广播
- ✅ UDP命令接收和响应
- ✅ HTTP Webhook接收
- ✅ 线程安全和并发控制
- ✅ 错误处理和异常恢复
- ✅ 性能优化和资源管理

所有组件都设计为独立、可测试、可扩展的模块。

