/**
 * @file test_domain.cpp
 * @brief 领域层功能验证测试
 * 
 * 编译命令：
 *   g++ -std=c++17 -I./src test_domain.cpp -o test_domain
 * 
 * 运行：
 *   ./test_domain
 */

#include "src/domain/domain.h"
#include <iostream>
#include <cassert>

using namespace zygl::domain;

void TestValueObjects() {
    std::cout << "测试值对象..." << std::endl;
    
    // 测试TaskStatusInfo
    TaskStatusInfo task;
    task.SetTaskID("task-001");
    task.SetTaskStatus("running");
    task.SetServiceName("人脸识别");
    assert(std::string(task.taskID) == "task-001");
    
    // 测试LocationInfo
    LocationInfo location;
    location.SetBoardAddress("192.168.1.100");
    location.chassisNumber = 1;
    location.boardNumber = 3;
    assert(std::string(location.boardAddress) == "192.168.1.100");
    
    // 测试ResourceUsage
    ResourceUsage resources;
    resources.cpuUsage = 75.5f;
    resources.memoryUsage = 60.2f;
    assert(resources.cpuUsage > 0);
    
    std::cout << "  ✓ 值对象测试通过" << std::endl;
}

void TestBoard() {
    std::cout << "测试Board实体..." << std::endl;
    
    // 创建计算板卡
    Board board("192.168.1.100", 1, BoardType::Computing);
    assert(board.GetBoardNumber() == 1);
    assert(board.CanRunTasks());
    assert(board.GetStatus() == BoardOperationalStatus::Unknown);
    
    // 创建交换板卡
    Board switchBoard("192.168.1.106", 6, BoardType::Switch);
    assert(!switchBoard.CanRunTasks());
    
    // 测试更新状态
    std::vector<TaskStatusInfo> tasks;
    TaskStatusInfo task1, task2;
    task1.SetTaskID("task-001");
    task2.SetTaskID("task-002");
    tasks.push_back(task1);
    tasks.push_back(task2);
    
    board.UpdateFromApiData(0, tasks);  // 0表示正常
    assert(board.GetStatus() == BoardOperationalStatus::Normal);
    assert(board.GetTaskCount() == 2);
    assert(!board.IsAbnormal());
    assert(board.IsOnline());
    
    // 测试离线
    board.MarkAsOffline();
    assert(board.GetStatus() == BoardOperationalStatus::Offline);
    assert(board.GetTaskCount() == 0);
    assert(board.IsAbnormal());
    assert(!board.IsOnline());
    
    std::cout << "  ✓ Board实体测试通过" << std::endl;
}

void TestChassis() {
    std::cout << "测试Chassis聚合根..." << std::endl;
    
    // 创建机箱
    Chassis chassis(1, "机箱-01");
    assert(chassis.GetChassisNumber() == 1);
    assert(std::string(chassis.GetChassisName()) == "机箱-01");
    
    // 添加板卡
    for (int slot = 1; slot <= 14; ++slot) {
        BoardType type = BoardSlotHelper::GetBoardTypeBySlot(slot);
        char ip[16];
        snprintf(ip, sizeof(ip), "192.168.1.%d", 100 + slot);
        
        Board board(ip, slot, type);
        
        // 如果是计算板卡，添加一些任务
        if (board.CanRunTasks()) {
            std::vector<TaskStatusInfo> tasks;
            TaskStatusInfo task;
            task.SetTaskID("task-001");
            tasks.push_back(task);
            board.UpdateFromApiData(0, tasks);
        } else {
            board.UpdateFromApiData(0, std::vector<TaskStatusInfo>());
        }
        
        chassis.AddOrUpdateBoard(board);
    }
    
    // 测试查找板卡
    Board* board = chassis.GetBoardByNumber(3);
    assert(board != nullptr);
    assert(board->GetBoardNumber() == 3);
    assert(board->CanRunTasks());
    
    // 测试交换板卡
    Board* switchBoard = chassis.GetBoardByNumber(6);
    assert(switchBoard != nullptr);
    assert(!switchBoard->CanRunTasks());
    
    // 测试统计
    int normalCount = chassis.CountNormalBoards();
    int taskCount = chassis.CountTotalTasks();
    assert(normalCount == 14);  // 所有板卡都正常
    assert(taskCount == 10);    // 只有10块计算板卡有任务
    
    std::cout << "  ✓ Chassis聚合根测试通过" << std::endl;
}

void TestStack() {
    std::cout << "测试Stack聚合根..." << std::endl;
    
    // 创建业务链路
    Stack stack("stack-001", "视频分析链路");
    assert(stack.GetStackUUID() == "stack-001");
    assert(stack.GetStackName() == "视频分析链路");
    assert(!stack.IsDeployed());
    
    // 添加标签
    StackLabelInfo label;
    label.SetLabelName("生产环境");
    label.SetLabelUUID("label-prod");
    assert(stack.AddLabel(label));
    assert(stack.GetLabelCount() == 1);
    assert(stack.HasLabel("label-prod"));
    
    // 创建组件和任务
    Service service("service-001", "人脸识别");
    service.SetStatus(ServiceStatus::Running);
    
    Task task("task-001");
    task.SetTaskStatus("running");
    ResourceUsage resources;
    resources.cpuUsage = 50.0f;
    resources.memoryUsage = 40.0f;
    task.UpdateResources(resources);
    
    service.AddOrUpdateTask(task);
    stack.AddOrUpdateService(service);
    
    // 测试查询任务资源（方案B核心功能）
    auto taskRes = stack.GetTaskResources("task-001");
    assert(taskRes.has_value());
    assert(taskRes->cpuUsage == 50.0f);
    
    // 测试统计
    assert(stack.GetServiceCount() == 1);
    assert(stack.GetTotalTaskCount() == 1);
    
    // 测试状态重计算
    stack.SetDeployStatus(StackDeployStatus::Deployed);
    stack.RecalculateRunningStatus();
    assert(stack.IsDeployed());
    assert(stack.IsRunningNormally());
    
    std::cout << "  ✓ Stack聚合根测试通过" << std::endl;
}

void TestAlert() {
    std::cout << "测试Alert聚合根..." << std::endl;
    
    // 创建板卡告警
    LocationInfo location;
    location.SetBoardAddress("192.168.1.103");
    location.chassisNumber = 1;
    location.boardNumber = 3;
    
    std::vector<std::string> messages = {"板卡离线", "连接超时"};
    
    Alert boardAlert = Alert::CreateBoardAlert("alert-001", location, messages);
    assert(boardAlert.IsBoardAlert());
    assert(!boardAlert.IsComponentAlert());
    assert(boardAlert.GetMessageCount() == 2);
    assert(!boardAlert.IsAcknowledged());
    
    // 确认告警
    boardAlert.Acknowledge();
    assert(boardAlert.IsAcknowledged());
    
    // 创建组件告警
    Alert compAlert = Alert::CreateComponentAlert(
        "alert-002",
        "视频分析",
        "stack-001",
        "人脸识别",
        "service-001",
        "task-001",
        location,
        messages
    );
    assert(compAlert.IsComponentAlert());
    assert(std::string(compAlert.GetStackName()) == "视频分析");
    assert(std::string(compAlert.GetTaskID()) == "task-001");
    
    std::cout << "  ✓ Alert聚合根测试通过" << std::endl;
}

void TestSystemTopology() {
    std::cout << "测试系统拓扑常量..." << std::endl;
    
    assert(SystemTopology::TOTAL_CHASSIS == 9);
    assert(SystemTopology::BOARDS_PER_CHASSIS == 14);
    assert(SystemTopology::TOTAL_BOARDS == 126);
    assert(SystemTopology::COMPUTING_BOARDS_PER_CHASSIS == 10);
    assert(SystemTopology::TOTAL_COMPUTING_BOARDS == 90);
    
    std::cout << "  ✓ 系统拓扑常量测试通过" << std::endl;
}

void TestBoardSlotHelper() {
    std::cout << "测试BoardSlotHelper..." << std::endl;
    
    // 测试槽位类型判断
    assert(BoardSlotHelper::GetBoardTypeBySlot(1) == BoardType::Computing);
    assert(BoardSlotHelper::GetBoardTypeBySlot(6) == BoardType::Switch);
    assert(BoardSlotHelper::GetBoardTypeBySlot(7) == BoardType::Switch);
    assert(BoardSlotHelper::GetBoardTypeBySlot(13) == BoardType::Power);
    assert(BoardSlotHelper::GetBoardTypeBySlot(14) == BoardType::Power);
    
    // 测试槽位有效性
    assert(BoardSlotHelper::IsValidSlotNumber(1));
    assert(BoardSlotHelper::IsValidSlotNumber(14));
    assert(!BoardSlotHelper::IsValidSlotNumber(0));
    assert(!BoardSlotHelper::IsValidSlotNumber(15));
    
    // 测试计算槽位判断
    assert(BoardSlotHelper::IsComputingSlot(1));
    assert(!BoardSlotHelper::IsComputingSlot(6));
    assert(!BoardSlotHelper::IsComputingSlot(13));
    
    std::cout << "  ✓ BoardSlotHelper测试通过" << std::endl;
}

void PrintSystemInfo() {
    std::cout << "\n=== 系统拓扑信息 ===" << std::endl;
    std::cout << "机箱数量: " << SystemTopology::TOTAL_CHASSIS << std::endl;
    std::cout << "每机箱板卡数: " << SystemTopology::BOARDS_PER_CHASSIS << std::endl;
    std::cout << "总板卡数: " << SystemTopology::TOTAL_BOARDS << std::endl;
    std::cout << "计算板卡数: " << SystemTopology::TOTAL_COMPUTING_BOARDS << std::endl;
    std::cout << "交换板卡槽位: 6, 7" << std::endl;
    std::cout << "电源板卡槽位: 13, 14" << std::endl;
    std::cout << "每板卡最多任务数: " << MAX_TASKS_PER_BOARD << std::endl;
    std::cout << "每业务链路最多标签数: " << MAX_LABELS_PER_STACK << std::endl;
    std::cout << "每告警最多消息数: " << MAX_ALERT_MESSAGES << std::endl;
    std::cout << "领域层版本: " << DOMAIN_VERSION << std::endl;
    std::cout << "==================\n" << std::endl;
}

int main() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "    领域层功能验证测试" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    try {
        PrintSystemInfo();
        
        TestValueObjects();
        TestBoard();
        TestChassis();
        TestStack();
        TestAlert();
        TestSystemTopology();
        TestBoardSlotHelper();
        
        std::cout << "\n========================================" << std::endl;
        std::cout << "  ✅ 所有测试通过！领域层实现正确。" << std::endl;
        std::cout << "========================================\n" << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n❌ 测试失败: " << e.what() << std::endl;
        return 1;
    }
}

