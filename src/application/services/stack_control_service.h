#pragma once

#include "../../domain/i_stack_repository.h"
#include "../../infrastructure/api_client/qyw_api_client.h"
#include "../dtos/dtos.h"
#include <memory>
#include <vector>
#include <string>

namespace zygl::application {

/**
 * @brief StackControlService - 业务链路控制服务
 * 
 * 职责：
 * 1. 处理业务链路的启用（Deploy）命令
 * 2. 处理业务链路的停用（Undeploy）命令
 * 3. 调用后端API执行实际操作
 * 4. 返回操作结果
 * 
 * 这是一个写服务，会调用外部API修改系统状态。
 */
class StackControlService {
public:
    /**
     * @brief 构造函数
     */
    StackControlService(
        std::shared_ptr<domain::IStackRepository> stackRepo,
        std::shared_ptr<infrastructure::QywApiClient> apiClient)
        : m_stackRepo(stackRepo),
          m_apiClient(apiClient) {
    }

    /**
     * @brief 根据标签批量启用业务链路
     * 
     * 工作流程：
     * 1. 从仓储中查找包含指定标签的业务链路
     * 2. 调用后端API执行Deploy操作
     * 3. 返回操作结果（成功和失败的业务链路列表）
     * 
     * @param command 包含标签UUID列表的命令
     * @return 部署结果DTO
     */
    ResponseDTO<DeployResultDTO> DeployByLabels(const DeployCommandDTO& command) const {
        try {
            // 验证输入
            if (command.stackLabels.empty()) {
                return ResponseDTO<DeployResultDTO>::Failure("标签列表不能为空");
            }
            
            // 调用后端API执行Deploy
            auto deployResponseOpt = m_apiClient->Deploy(command.stackLabels);
            if (!deployResponseOpt.has_value()) {
                return ResponseDTO<DeployResultDTO>::Failure("调用后端API失败");
            }
            
            const auto& apiResponse = deployResponseOpt.value();
            
            // 转换为DTO
            DeployResultDTO result;
            
            // 成功的业务链路
            for (const auto& success : apiResponse.successStackInfos) {
                DeployResultDTO::StackResult stackResult;
                stackResult.stackName = success.stackName;
                stackResult.stackUUID = success.stackUUID;
                stackResult.message = success.message;
                result.successStacks.push_back(stackResult);
            }
            
            // 失败的业务链路
            for (const auto& failure : apiResponse.failureStackInfos) {
                DeployResultDTO::StackResult stackResult;
                stackResult.stackName = failure.stackName;
                stackResult.stackUUID = failure.stackUUID;
                stackResult.message = failure.message;
                result.failureStacks.push_back(stackResult);
            }
            
            // 统计
            result.totalCount = static_cast<int32_t>(result.successStacks.size() + result.failureStacks.size());
            result.successCount = static_cast<int32_t>(result.successStacks.size());
            result.failureCount = static_cast<int32_t>(result.failureStacks.size());
            
            return ResponseDTO<DeployResultDTO>::Success(result, "Deploy命令执行完成");
        } catch (const std::exception& e) {
            return ResponseDTO<DeployResultDTO>::Failure(
                std::string("执行Deploy命令失败: ") + e.what()
            );
        }
    }

    /**
     * @brief 根据标签批量停用业务链路
     * 
     * 工作流程：
     * 1. 从仓储中查找包含指定标签的业务链路
     * 2. 调用后端API执行Undeploy操作
     * 3. 返回操作结果（成功和失败的业务链路列表）
     * 
     * @param command 包含标签UUID列表的命令
     * @return 部署结果DTO
     */
    ResponseDTO<DeployResultDTO> UndeployByLabels(const DeployCommandDTO& command) const {
        try {
            // 验证输入
            if (command.stackLabels.empty()) {
                return ResponseDTO<DeployResultDTO>::Failure("标签列表不能为空");
            }
            
            // 调用后端API执行Undeploy
            auto undeployResponseOpt = m_apiClient->Undeploy(command.stackLabels);
            if (!undeployResponseOpt.has_value()) {
                return ResponseDTO<DeployResultDTO>::Failure("调用后端API失败");
            }
            
            const auto& apiResponse = undeployResponseOpt.value();
            
            // 转换为DTO
            DeployResultDTO result;
            
            // 成功的业务链路
            for (const auto& success : apiResponse.successStackInfos) {
                DeployResultDTO::StackResult stackResult;
                stackResult.stackName = success.stackName;
                stackResult.stackUUID = success.stackUUID;
                stackResult.message = success.message;
                result.successStacks.push_back(stackResult);
            }
            
            // 失败的业务链路
            for (const auto& failure : apiResponse.failureStackInfos) {
                DeployResultDTO::StackResult stackResult;
                stackResult.stackName = failure.stackName;
                stackResult.stackUUID = failure.stackUUID;
                stackResult.message = failure.message;
                result.failureStacks.push_back(stackResult);
            }
            
            // 统计
            result.totalCount = static_cast<int32_t>(result.successStacks.size() + result.failureStacks.size());
            result.successCount = static_cast<int32_t>(result.successStacks.size());
            result.failureCount = static_cast<int32_t>(result.failureStacks.size());
            
            return ResponseDTO<DeployResultDTO>::Success(result, "Undeploy命令执行完成");
        } catch (const std::exception& e) {
            return ResponseDTO<DeployResultDTO>::Failure(
                std::string("执行Undeploy命令失败: ") + e.what()
            );
        }
    }

    /**
     * @brief 根据单个标签启用业务链路（便捷方法）
     * 
     * @param labelUUID 标签UUID
     * @return 部署结果DTO
     */
    ResponseDTO<DeployResultDTO> DeployByLabel(const std::string& labelUUID) const {
        DeployCommandDTO command;
        command.stackLabels.push_back(labelUUID);
        return DeployByLabels(command);
    }

    /**
     * @brief 根据单个标签停用业务链路（便捷方法）
     * 
     * @param labelUUID 标签UUID
     * @return 部署结果DTO
     */
    ResponseDTO<DeployResultDTO> UndeployByLabel(const std::string& labelUUID) const {
        DeployCommandDTO command;
        command.stackLabels.push_back(labelUUID);
        return UndeployByLabels(command);
    }

    /**
     * @brief 查询包含指定标签的业务链路（不执行操作，仅查询）
     * 
     * 用于在执行Deploy/Undeploy前预览将要操作的业务链路。
     * 
     * @param labelUUID 标签UUID
     * @return 业务链路UUID列表
     */
    ResponseDTO<std::vector<std::string>> PreviewStacksByLabel(const std::string& labelUUID) const {
        try {
            auto stacks = m_stackRepo->FindByLabel(labelUUID);
            
            std::vector<std::string> stackUUIDs;
            for (const auto& stack : stacks) {
                stackUUIDs.push_back(stack.GetStackUUID());
            }
            
            return ResponseDTO<std::vector<std::string>>::Success(
                stackUUIDs, 
                "找到 " + std::to_string(stackUUIDs.size()) + " 个业务链路"
            );
        } catch (const std::exception& e) {
            return ResponseDTO<std::vector<std::string>>::Failure(
                std::string("查询业务链路失败: ") + e.what()
            );
        }
    }

private:
    std::shared_ptr<domain::IStackRepository> m_stackRepo;
    std::shared_ptr<infrastructure::QywApiClient> m_apiClient;
};

} // namespace zygl::application

