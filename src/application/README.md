# 应用层 (Application Layer)

## 概述

应用层协调领域对象和仓储来完成具体的用例（Use Cases）。它不包含业务逻辑，而是编排领域对象来实现应用功能。

## 设计原则

### 1. 用例驱动

每个应用服务对应一组相关的用例：
- **MonitoringService**：查询用例（系统状态、业务链路、告警）
- **StackControlService**：控制用例（启停业务链路）
- **AlertService**：告警用例（接收告警、确认、清理）

### 2. 依赖倒置

应用层依赖领域层接口，不依赖具体实现：
```
Application → Domain (接口)
           ← Infrastructure (实现)
```

### 3. DTO模式

使用DTOs（数据传输对象）在各层之间传递数据，避免领域对象泄露：
- **输入**：CommandDTOs（DeployCommandDTO、AlertAcknowledgeDTO）
- **输出**：数据DTOs（ChassisDTO、StackDTO、AlertDTO）
- **包装**：ResponseDTO<T>（统一响应格式）

## 目录结构

```
application/
├── dtos/                        # 数据传输对象
│   └── dtos.h                  # 所有DTOs定义
├── services/                    # 应用服务
│   ├── monitoring_service.h    # 监控服务
│   ├── stack_control_service.h # 业务控制服务
│   └── alert_service.h         # 告警服务
└── application.h                # 统一头文件
```

## 核心组件

### 1. MonitoringService（监控服务）

**职责**：提供只读查询服务

**核心方法**：

#### 系统状态查询
```cpp
// 获取系统概览（所有机箱、板卡、任务）
ResponseDTO<SystemOverviewDTO> GetSystemOverview() const;

// 根据机箱号获取机箱信息
ResponseDTO<ChassisDTO> GetChassisByNumber(int32_t chassisNumber) const;

// 根据板卡地址获取机箱
ResponseDTO<ChassisDTO> GetChassisByBoardAddress(const std::string& boardAddress) const;
```

#### 业务链路查询
```cpp
// 获取所有业务链路列表
ResponseDTO<StackListDTO> GetAllStacks() const;

// 根据UUID获取业务链路详情
ResponseDTO<StackDTO> GetStackByUUID(const std::string& stackUUID) const;

// 【方案B核心】根据任务ID获取任务资源（按需查询）
ResponseDTO<TaskResourceDTO> GetTaskResource(const std::string& taskID) const;
```

#### 告警查询
```cpp
// 获取所有活动告警（用于UDP广播）
ResponseDTO<AlertListDTO> GetActiveAlerts() const;

// 获取未确认的告警
ResponseDTO<AlertListDTO> GetUnacknowledgedAlerts() const;
```

**使用示例**：
```cpp
auto monitoring = ApplicationServiceFactory::CreateMonitoringService(
    chassisRepo, stackRepo, alertRepo
);

// 获取系统概览
auto response = monitoring->GetSystemOverview();
if (response.success) {
    const auto& overview = response.data;
    std::cout << "总板卡数: " << overview.totalBoards << std::endl;
    std::cout << "正常板卡: " << overview.totalNormalBoards << std::endl;
}

// 按需查询任务资源（方案B）
auto taskRes = monitoring->GetTaskResource("task-123");
if (taskRes.success) {
    std::cout << "CPU使用率: " << taskRes.data.cpuUsage << "%" << std::endl;
}
```

### 2. StackControlService（业务控制服务）

**职责**：处理业务链路启停命令

**核心方法**：

```cpp
// 根据标签批量启用业务链路
ResponseDTO<DeployResultDTO> DeployByLabels(const DeployCommandDTO& command) const;

// 根据标签批量停用业务链路
ResponseDTO<DeployResultDTO> UndeployByLabels(const DeployCommandDTO& command) const;

// 根据单个标签启用（便捷方法）
ResponseDTO<DeployResultDTO> DeployByLabel(const std::string& labelUUID) const;

// 根据单个标签停用（便捷方法）
ResponseDTO<DeployResultDTO> UndeployByLabel(const std::string& labelUUID) const;

// 预览将要操作的业务链路（不执行操作）
ResponseDTO<std::vector<std::string>> PreviewStacksByLabel(const std::string& labelUUID) const;
```

**工作流程**：
```
1. 接收前端命令（包含标签列表）
2. 调用后端API（POST /deploy 或 /undeploy）
3. 接收API响应（成功和失败的业务链路）
4. 转换为DeployResultDTO
5. 返回给调用者
```

**使用示例**：
```cpp
auto stackControl = ApplicationServiceFactory::CreateStackControlService(
    stackRepo, apiClient
);

// 构建命令
DeployCommandDTO command;
command.stackLabels = {"label-prod", "label-video"};

// 执行部署
auto response = stackControl->DeployByLabels(command);
if (response.success) {
    const auto& result = response.data;
    std::cout << "成功: " << result.successCount << std::endl;
    std::cout << "失败: " << result.failureCount << std::endl;
}
```

### 3. AlertService（告警服务）

**职责**：处理告警的接收、确认和清理

**核心方法**：

#### 接收告警（来自WebhookListener）
```cpp
// 处理板卡异常上报
ResponseDTO<std::string> HandleBoardAlert(
    const std::string& boardAddress,
    const std::string& chassisName,
    int32_t chassisNumber,
    const std::string& boardName,
    int32_t boardNumber,
    int32_t boardStatus,
    const std::vector<std::string>& alertMessages) const;

// 处理组件异常上报
ResponseDTO<std::string> HandleComponentAlert(
    const std::string& stackName,
    const std::string& stackUUID,
    const std::string& serviceName,
    const std::string& serviceUUID,
    const std::string& taskID,
    const domain::LocationInfo& location,
    const std::vector<std::string>& alertMessages) const;
```

#### 告警管理
```cpp
// 确认单个告警
ResponseDTO<bool> AcknowledgeAlert(const std::string& alertUUID) const;

// 批量确认告警
ResponseDTO<int32_t> AcknowledgeMultiple(const AlertAcknowledgeDTO& command) const;

// 清理过期告警（建议定期调用）
ResponseDTO<int32_t> CleanupExpiredAlerts(uint64_t maxAgeSeconds = 86400) const;

// 删除指定告警
ResponseDTO<bool> RemoveAlert(const std::string& alertUUID) const;
```

**使用示例**：
```cpp
auto alertService = ApplicationServiceFactory::CreateAlertService(
    alertRepo, chassisRepo
);

// 处理板卡告警（来自Webhook）
std::vector<std::string> messages = {"板卡离线", "连接超时"};
auto response = alertService->HandleBoardAlert(
    "192.168.1.103", "机箱-01", 1, "板卡-03", 3, 1, messages
);

// 批量确认告警
AlertAcknowledgeDTO ackCmd;
ackCmd.alertUUIDs = {"alert-1", "alert-2", "alert-3"};
auto ackRes = alertService->AcknowledgeMultiple(ackCmd);

// 定期清理过期告警（如每小时调用一次）
auto cleanRes = alertService->CleanupExpiredAlerts(24 * 3600);  // 24小时
```

## DTOs（数据传输对象）

### 数据DTOs

#### 系统相关
- **BoardDTO**：板卡数据
- **ChassisDTO**：机箱数据
- **SystemOverviewDTO**：系统概览

#### 业务相关
- **TaskResourceDTO**：任务资源（方案B）
- **ServiceDTO**：组件数据
- **StackDTO**：业务链路数据
- **StackListDTO**：业务链路列表

#### 告警相关
- **AlertDTO**：告警数据
- **AlertListDTO**：告警列表

### 命令DTOs

- **DeployCommandDTO**：部署命令
- **TaskResourceQueryDTO**：任务资源查询
- **AlertAcknowledgeDTO**：告警确认命令

### 结果DTOs

- **DeployResultDTO**：部署结果（成功/失败列表）
- **ResponseDTO<T>**：通用响应包装

## 统一响应格式

所有服务方法都返回`ResponseDTO<T>`：

```cpp
template<typename T>
struct ResponseDTO {
    bool success;              // 是否成功
    std::string message;       // 消息
    T data;                   // 数据
    int32_t errorCode;        // 错误码

    // 便捷构造
    static ResponseDTO<T> Success(const T& data, const std::string& msg = "success");
    static ResponseDTO<T> Failure(const std::string& msg, int32_t code = -1);
};
```

**使用示例**：
```cpp
auto response = monitoring->GetSystemOverview();
if (response.success) {
    // 处理成功情况
    const auto& data = response.data;
    ProcessData(data);
} else {
    // 处理失败情况
    std::cerr << "Error: " << response.message << std::endl;
}
```

## 应用服务工厂

**ApplicationServiceFactory**提供统一的服务创建入口：

```cpp
// 创建单个服务
auto monitoring = ApplicationServiceFactory::CreateMonitoringService(...);
auto stackControl = ApplicationServiceFactory::CreateStackControlService(...);
auto alertService = ApplicationServiceFactory::CreateAlertService(...);

// 一次创建所有服务
auto services = ApplicationServiceFactory::CreateAll(
    chassisRepo, stackRepo, alertRepo, apiClient
);

// 使用服务
auto overview = services.monitoringService->GetSystemOverview();
auto deployResult = services.stackControlService->DeployByLabel("label-prod");
```

## 依赖关系

```
Application Layer
    ↓ 依赖
Domain Layer（接口）
    ↑ 实现
Infrastructure Layer
```

## 使用场景

### 场景1：UDP状态广播

```cpp
// StateBroadcaster定时调用
auto monitoring = ApplicationServiceFactory::CreateMonitoringService(...);

// 获取系统概览
auto overview = monitoring->GetSystemOverview();
if (overview.success) {
    // 将overview.data序列化为UDP包
    // 广播给前端
}

// 获取活动告警
auto alerts = monitoring->GetActiveAlerts();
if (alerts.success) {
    // 将alerts.data合并到UDP包
}
```

### 场景2：前端点击任务（方案B）

```cpp
// CommandListener接收到任务查询请求
auto monitoring = ApplicationServiceFactory::CreateMonitoringService(...);

// 按需查询任务资源
auto taskRes = monitoring->GetTaskResource("task-123");
if (taskRes.success) {
    // 将taskRes.data序列化为UDP响应包
    // 发送给前端
}
```

### 场景3：前端部署业务链路

```cpp
// CommandListener接收到部署命令
auto stackControl = ApplicationServiceFactory::CreateStackControlService(...);

DeployCommandDTO command;
command.stackLabels = {"label-prod"};

auto result = stackControl->DeployByLabels(command);
if (result.success) {
    // 将result.data序列化为UDP响应包
    // 发送给前端（包含成功和失败列表）
}
```

### 场景4：接收后端告警

```cpp
// WebhookListener接收到HTTP POST告警
auto alertService = ApplicationServiceFactory::CreateAlertService(...);

// 处理板卡告警
auto response = alertService->HandleBoardAlert(...);
if (response.success) {
    // 告警已记录，告警UUID: response.data
}
```

## 性能特性

- **无状态服务**：所有服务都是无状态的，可以安全地并发调用
- **只读vs写入**：MonitoringService是纯只读，性能极高
- **事务性**：每个方法都是一个完整的事务
- **异常安全**：所有方法都有异常处理，返回ResponseDTO

## 扩展点

1. **添加新的查询方法**：在MonitoringService中添加
2. **添加新的控制命令**：在StackControlService中添加
3. **添加新的告警类型**：在AlertService中添加处理方法
4. **自定义DTOs**：在dtos.h中添加新的数据结构

## 注意事项

### 1. 线程安全

- 所有服务都是线程安全的（通过仓储的线程安全机制）
- 可以并发调用服务方法

### 2. 性能考虑

- MonitoringService的GetSystemOverview()会读取所有机箱数据，适合定时调用
- GetTaskResource()是按需查询，适合用户触发

### 3. 错误处理

- 始终检查ResponseDTO.success
- 使用ResponseDTO.message获取错误描述
- 使用ResponseDTO.errorCode进行错误分类

## 代码统计

- **文件数量**：5个头文件
- **代码行数**：约1,100行
- **服务数量**：3个
- **DTOs数量**：15+个

## 命名空间

所有应用层代码都在 `zygl::application` 命名空间下。

## 下一步

应用层已完成，后续需要：
1. 实现Interfaces层（UDP/HTTP通信）
2. 完整的系统集成
3. 端到端测试

