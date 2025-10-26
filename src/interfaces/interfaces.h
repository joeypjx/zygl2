#pragma once

/**
 * @file interfaces.h
 * @brief Interfaces层统一头文件
 * 
 * Interfaces层（接口层）负责处理所有外部通信：
 * - UDP多播：状态广播和命令接收
 * - HTTP：Webhook接收
 * 
 * 主要组件：
 * 1. UDP通信
 *    - udp_protocol.h: UDP通信协议定义（数据包格式）
 *    - state_broadcaster.h: 状态广播器（服务端->前端）
 *    - command_listener.h: 命令监听器（前端->服务端）
 * 
 * 2. HTTP通信
 *    - webhook_listener.h: Webhook监听器（后端API->服务端）
 */

// UDP通信
#include "udp/udp_protocol.h"
#include "udp/state_broadcaster.h"
#include "udp/command_listener.h"

// HTTP通信
#include "http/webhook_listener.h"

