# 基础设施层 (Infrastructure Layer)

## 概述

基础设施层实现了领域层定义的仓储接口，以及与外部系统交互的组件（API客户端、数据采集器等）。

## 设计原则

### 1. 依赖倒置

基础设施层依赖并实现领域层的接口，符合DDD的依赖倒置原则。

### 2. 线程安全

- **InMemoryChassisRepository**：使用双缓冲机制实现无锁读取
- **InMemoryStackRepository**：使用mutex实现线程安全
- **InMemoryAlertRepository**：使用mutex实现线程安全

### 3. 性能优化

- 双缓冲无锁读取（适合读多写少场景）
- 批量操作支持
- 原子操作保证数据一致性

## 目录结构

```
infrastructure/
├── persistence/                          # 仓储实现
│   ├── in_memory_chassis_repository.h   # 机箱仓储（双缓冲）
│   ├── in_memory_stack_repository.h     # 业务链路仓储
│   └── in_memory_alert_repository.h     # 告警仓储
├── api_client/                           # API客户端
│   └── qyw_api_client.h                 # 后端API客户端
├── collectors/                           # 数据采集器
│   └── data_collector_service.h         # 定时数据采集服务
├── config/                               # 配置和工厂
│   └── chassis_factory.h                # 机箱工厂
└── infrastructure.h                      # 统一头文件
```

## 核心组件

### 1. InMemoryChassisRepository（双缓冲）

**设计亮点**：
- 使用双缓冲（Double Buffering）实现完全无锁的读取操作
- 写入者更新后台缓冲，完成后原子交换指针
- 适合频繁读取、定时更新的场景（如状态广播）

**关键方法**：
- `SaveAll()`：批量保存并原子交换缓冲（核心）
- `GetAll()`：无锁读取所有机箱
- `Initialize()`：系统启动时初始化拓扑

**线程模型**：
```
读取线程1 ──无锁──> ActiveBuffer ──┐
读取线程2 ──无锁──> ActiveBuffer   │
读取线程3 ──无锁──> ActiveBuffer   ├── 原子指针
                                  │
写入线程   ──独占──> BackBuffer ────┘
```

### 2. InMemoryStackRepository

**功能**：
- 管理业务链路聚合根
- 支持按标签查找（批量deploy/undeploy）
- 支持按任务ID查找资源（方案B核心）

**关键方法**：
- `FindByLabel()`：根据标签查找业务链路
- `FindTaskResources()`：按任务ID查找资源（方案B）
- `SaveAll()`：批量保存

**线程安全**：使用mutex保护

### 3. InMemoryAlertRepository

**功能**：
- 管理活动告警
- 支持多维度查询（类型、实体、板卡、业务链路）
- 支持告警确认和过期清理

**关键方法**：
- `GetAllActive()`：获取所有活动告警（用于UDP广播）
- `GetUnacknowledged()`：获取未确认告警
- `RemoveExpired()`：清理过期已确认告警
- `AcknowledgeMultiple()`：批量确认

**线程安全**：使用mutex保护

### 4. ChassisFactory（配置工厂）

**职责**：
- 在系统启动时创建9×14完整拓扑
- 根据配置分配IP地址
- 自动设置板卡类型（计算/交换/电源）

**使用示例**：
```cpp
ChassisFactory factory;

// 使用默认配置
auto topology = factory.CreateFullTopology();

// 使用自定义配置
ChassisFactory::ChassisConfig config;
config.chassisNumber = 1;
config.chassisName = "机箱-01";
config.ipBaseAddress = "192.168.1";
config.ipStartOffset = 100;
auto chassis = factory.CreateChassis(config);
```

### 5. QywApiClient（API客户端）

**职责**：
- 封装对后端API的HTTP调用
- 处理JSON解析和错误
- 防腐层（Anti-Corruption Layer）

**接口**：
- `GetBoardInfo()`：获取板卡信息
- `GetStackInfo()`：获取业务链路详情
- `Deploy()`：批量启用业务链路
- `Undeploy()`：批量停用业务链路

**注意**：
- 需要集成cpp-httplib库
- 需要集成JSON解析库（如nlohmann/json）
- 当前实现提供接口定义，实际HTTP调用需要补充

### 6. DataCollectorService（数据采集器）

**职责**：
- 定时调用后端API
- 将API数据转换为领域对象
- 更新内存仓储

**工作流程**：
```
1. 定时触发（如每10秒）
2. 调用 GET /boardinfo
3. 更新Chassis聚合（非活动缓冲）
4. 调用 GET /stackinfo
5. 更新Stack聚合
6. 原子交换Chassis缓冲指针
```

**线程模型**：
- 运行在独立后台线程
- 可以安全启动/停止
- 支持手动触发采集（用于测试）

**使用示例**：
```cpp
auto collector = std::make_shared<DataCollectorService>(
    apiClient, 
    chassisRepo, 
    stackRepo,
    10  // 10秒间隔
);

collector->Start();  // 启动后台采集
// ...
collector->Stop();   // 停止采集
```

## 依赖关系

```
Infrastructure Layer
    ↓ 依赖
Domain Layer（接口）
```

```
外部依赖：
- C++17标准库
- <atomic>（原子操作）
- <mutex>（线程同步）
- <thread>（后台线程）
- cpp-httplib（待集成，用于HTTP通信）
- nlohmann/json（待集成，用于JSON解析）
```

## 工厂类

### RepositoryFactory

用于创建所有仓储实例：

```cpp
// 创建单个仓储
auto chassisRepo = RepositoryFactory::CreateChassisRepository();
auto stackRepo = RepositoryFactory::CreateStackRepository();
auto alertRepo = RepositoryFactory::CreateAlertRepository();

// 一次创建所有仓储
auto repos = RepositoryFactory::CreateAll();
```

### ServiceFactory

用于创建服务实例：

```cpp
// 创建API客户端
auto apiClient = ServiceFactory::CreateApiClient("http://192.168.1.100:8080");

// 创建数据采集器
auto collector = ServiceFactory::CreateDataCollector(
    apiClient, chassisRepo, stackRepo, 10
);
```

### SystemInitializer

用于系统初始化：

```cpp
// 初始化系统拓扑（默认配置）
SystemInitializer::InitializeTopology(chassisRepo);

// 初始化系统拓扑（自定义配置）
std::array<ChassisFactory::ChassisConfig, 9> configs = /* ... */;
SystemInitializer::InitializeTopology(chassisRepo, configs);
```

## 使用示例

### 完整的系统启动流程

```cpp
#include "infrastructure/infrastructure.h"

using namespace zygl::infrastructure;

int main() {
    // 1. 创建所有仓储
    auto repos = RepositoryFactory::CreateAll();
    
    // 2. 初始化系统拓扑（9×14固定拓扑）
    SystemInitializer::InitializeTopology(repos.chassisRepo);
    
    // 3. 创建API客户端
    auto apiClient = ServiceFactory::CreateApiClient("http://192.168.1.100:8080");
    
    // 4. 创建并启动数据采集服务
    auto collector = ServiceFactory::CreateDataCollector(
        apiClient, 
        repos.chassisRepo, 
        repos.stackRepo,
        10  // 10秒采集间隔
    );
    collector->Start();
    
    // 5. 系统运行...
    
    // 6. 停止采集服务
    collector->Stop();
    
    return 0;
}
```

### 双缓冲机制说明

```cpp
// 写入线程（DataCollector）
auto allChassis = chassisRepo->GetAll();  // 读取旧数据
// ... 更新allChassis中的所有机箱状态 ...
chassisRepo->SaveAll(allChassis);  // 原子交换缓冲

// 读取线程（StateBroadcaster）
auto chassisData = chassisRepo->GetAll();  // 无锁读取
// 读取到的数据是完整一致的（要么全是旧数据，要么全是新数据）
```

## 性能特性

### 双缓冲性能

| 操作 | 复杂度 | 锁 | 说明 |
|------|--------|---|------|
| GetAll() | O(1) | 无 | 无锁读取，极高性能 |
| SaveAll() | O(n) | 无 | 独占访问后台缓冲，原子交换 |

### 内存使用

- **双缓冲开销**：2倍机箱数据（126块板卡 × 2）
- **优势**：无锁读取，适合高频广播场景

## 扩展点

### 1. API客户端实现

当前QywApiClient提供接口定义，需要：
1. 集成cpp-httplib库
2. 实现HTTP GET/POST方法
3. 实现JSON解析逻辑

### 2. 配置加载

当前ChassisFactory使用默认配置，可以扩展：
1. 从配置文件加载（YAML/JSON）
2. 从环境变量读取
3. 从数据库加载

### 3. 错误处理

可以增强：
1. API调用失败重试
2. 数据采集失败告警
3. 详细的错误日志

## 注意事项

### 1. 线程安全

- **Chassis仓储**：写入者应该是单线程（DataCollector）
- **Stack/Alert仓储**：支持多线程读写，但写入仍应谨慎

### 2. 内存管理

- 所有数据存储在内存中
- 服务重启后数据丢失（设计权衡）
- 注意内存增长，定期清理过期告警

### 3. 性能调优

- 采集间隔不宜过短（建议≥5秒）
- UDP广播频率应与采集间隔匹配
- 考虑API服务器负载

## 代码统计

- **文件数量**：7个头文件
- **代码行数**：约1,678行
- **仓储实现**：3个
- **服务组件**：2个
- **工厂类**：3个

## 命名空间

所有基础设施层代码都在 `zygl::infrastructure` 命名空间下。

## 下一步

基础设施层已基本完成，后续需要：
1. 实现Application层（应用服务）
2. 实现Interfaces层（UDP/HTTP通信）
3. 集成cpp-httplib和JSON库
4. 完整的系统集成测试

