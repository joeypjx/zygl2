# 领域层 (Domain Layer)

## 概述

本目录包含系统的核心业务逻辑和领域模型，采用领域驱动设计（DDD）原则。领域层不依赖任何外部框架、数据库或网络库，只使用C++标准库。

## 设计原则

### 1. 聚合（Aggregates）和聚合根（Aggregate Roots）

- **Chassis（机箱）** - 聚合根
  - 包含14块Board实体
  - 系统中固定有9个机箱
  - 负责维护板卡状态的一致性

- **Stack（业务链路）** - 聚合根
  - 包含多个Service实体
  - Service包含多个Task实体
  - 负责计算整体运行状态

- **Alert（告警）** - 聚合根
  - 独立的告警实体
  - 记录系统异常事件

### 2. 固定大小数据结构（UDP通信优化）

为了支持"原数据对象"UDP通信，关键数据结构采用固定大小：

- 使用 `std::array` 代替 `std::vector`
- 使用 `char[N]` 代替 `std::string`（对于需要UDP传输的字段）
- 使用 `#pragma pack(1)` 确保内存对齐
- 需要进行网络字节序转换（htons/htonl）

### 3. 业务规则

#### 机箱和板卡
- 9个机箱，每个机箱14块板卡（槽位1-14）
- 槽位6、7：交换板卡（不运行任务）
- 槽位13、14：电源板卡（不运行任务）
- 计算板卡最多运行8个任务

#### 状态管理
- 板卡状态：Unknown（初始）、Normal（正常）、Abnormal（异常）、Offline（离线）
- 业务链路部署状态：Undeployed（未部署）、Deployed（已部署）
- 业务链路运行状态：Normal（正常）、Abnormal（异常）

## 文件结构

```
domain/
├── README.md                      # 本文件
├── value_objects.h                # 值对象和枚举定义
├── board.h                        # Board实体
├── chassis.h                      # Chassis聚合根
├── task.h                         # Task实体
├── service.h                      # Service实体
├── stack.h                        # Stack聚合根
├── alert.h                        # Alert聚合根
├── i_chassis_repository.h         # 机箱仓储接口
├── i_stack_repository.h           # 业务链路仓储接口
└── i_alert_repository.h           # 告警仓储接口
```

## 核心组件说明

### 值对象 (Value Objects)

定义在 `value_objects.h` 中：

- **枚举类型**：BoardType、BoardOperationalStatus、StackDeployStatus、StackRunningStatus、ServiceStatus、ServiceType、AlertType
- **固定大小结构**：TaskStatusInfo、LocationInfo、StackLabelInfo、ResourceUsage、AlertMessage

所有固定大小结构都使用 `#pragma pack(1)` 确保跨平台兼容性。

### 实体和聚合根

#### Board（板卡实体）
- 维护板卡配置（地址、槽位号、类型）
- 维护动态状态（运行状态、任务列表）
- 核心方法：
  - `CanRunTasks()`：检查板卡类型是否允许运行任务
  - `UpdateFromApiData()`：从API数据更新状态
  - `MarkAsOffline()`：标记为离线

#### Chassis（机箱聚合根）
- 包含14块板卡（固定槽位）
- 提供板卡查找和统计功能
- 核心方法：
  - `GetBoardByAddress()`：根据IP查找板卡
  - `GetBoardByNumber()`：根据槽位号查找板卡
  - `CountNormalBoards()`、`CountAbnormalBoards()`等统计方法

#### Task（任务实体）
- 包含资源使用情况（CPU、内存、网络、GPU）
- 包含运行位置信息
- 核心方法：
  - `UpdateResources()`：更新资源使用
  - `IsResourceOverloaded()`：检查资源是否过载

#### Service（算法组件实体）
- 包含多个Task实例
- 维护组件状态和类型
- 核心方法：
  - `AddOrUpdateTask()`：管理任务
  - `CalculateTotalResources()`：聚合所有任务的资源
  - `RecalculateStatus()`：根据任务状态重新计算组件状态

#### Stack（业务链路聚合根）
- 包含多个Service实例
- 包含标签信息（用于批量操作）
- 核心方法：
  - `GetTaskResources()`：查找任务资源（方案B：按需查询）
  - `RecalculateRunningStatus()`：计算整体运行状态
  - `HasLabel()`：检查是否包含指定标签

#### Alert（告警聚合根）
- 支持两种类型：板卡异常、组件异常
- 包含告警消息列表（最多16条）
- 核心方法：
  - `CreateBoardAlert()`：创建板卡告警
  - `CreateComponentAlert()`：创建组件告警
  - `Acknowledge()`：确认告警

### 仓储接口 (Repository Interfaces)

#### IChassisRepository
- 管理9个固定机箱的存储
- 支持双缓冲机制（SaveAll、GetAll无锁读取）
- 提供统计功能

#### IStackRepository
- 管理动态数量的业务链路
- 支持按标签查找（用于deploy/undeploy命令）
- 支持按任务ID查找资源（方案B）

#### IAlertRepository
- 管理活动告警
- 支持按类型、实体、板卡、业务链路查找
- 支持告警确认和过期清理

## 使用示例

### 创建和更新板卡

```cpp
using namespace zygl::domain;

// 创建一个计算板卡
Board board("192.168.1.100", 1, BoardType::Computing);

// 从API数据更新板卡状态
std::vector<TaskStatusInfo> tasks = /* ... 从API获取 ... */;
board.UpdateFromApiData(0, tasks);  // 0表示正常

// 检查板卡状态
if (board.IsAbnormal()) {
    // 处理异常
}
```

### 创建和管理机箱

```cpp
// 创建机箱
Chassis chassis(1, "Chassis-01");

// 添加板卡
for (int i = 1; i <= 14; ++i) {
    BoardType type = BoardType::Computing;
    if (i == 6 || i == 7) type = BoardType::Switch;
    if (i == 13 || i == 14) type = BoardType::Power;
    
    Board board(ip, i, type);
    chassis.AddOrUpdateBoard(board);
}

// 查找板卡
Board* board = chassis.GetBoardByNumber(3);
if (board && board->CanRunTasks()) {
    // 操作计算板卡
}
```

### 创建告警

```cpp
// 创建板卡异常告警
LocationInfo location;
location.SetBoardAddress("192.168.1.100");
location.chassisNumber = 1;
location.boardNumber = 3;

std::vector<std::string> messages = {"板卡离线", "连接超时"};

Alert alert = Alert::CreateBoardAlert(
    "alert-uuid-001",
    location,
    messages
);

// 确认告警
alert.Acknowledge();
```

### 查询任务资源

```cpp
// 从业务链路中查询任务资源
Stack stack("stack-uuid", "业务链路A");
// ... 添加Service和Task ...

auto resources = stack.GetTaskResources("task-123");
if (resources.has_value()) {
    float cpuUsage = resources->cpuUsage;
    float memUsage = resources->memoryUsage;
    // 使用资源数据
}
```

## 常量定义

```cpp
TOTAL_CHASSIS_COUNT = 9              // 系统中的机箱数量
BOARDS_PER_CHASSIS = 14              // 每个机箱的板卡数量
MAX_TASKS_PER_BOARD = 8              // 每块板卡最多任务数
MAX_LABELS_PER_STACK = 8             // 每个业务链路最多标签数
MAX_ALERT_MESSAGES = 16              // 每个告警最多消息数
```

## 依赖关系

```
领域层（Domain Layer）
    ↓ 被依赖
应用层（Application Layer）
    ↓ 被依赖
基础设施层（Infrastructure Layer）
接口层（Interfaces Layer）
```

领域层是整个系统的核心，不依赖任何其他层，只使用C++标准库。

## 线程安全

领域层的类本身**不保证线程安全**。线程安全由基础设施层的仓储实现（InMemoryRepository）负责：

- **Chassis仓储**：使用双缓冲机制实现无锁读取
- **Stack仓储**：使用std::shared_mutex保护
- **Alert仓储**：使用std::shared_mutex保护

## 命名空间

所有领域层代码都在 `zygl::domain` 命名空间下。

