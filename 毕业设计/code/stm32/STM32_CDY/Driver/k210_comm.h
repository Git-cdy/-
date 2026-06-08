#ifndef __K210_COMM_H
#define __K210_COMM_H

#include "stm32f10x.h"
#include "uart_frame.h"
#include <stdint.h>

// ================== K210 通信管理模块 ==================
// 功能：
// 1. 接收 K210 的病害识别帧
// 2. 验证帧的完整性和有效性
// 3. 提供通信统计和诊断信息
// 4. 支持超时检测和错误恢复

// ================== 通信状态定义 ==================
typedef enum {
    K210_COMM_IDLE,        // 空闲，等待数据
    K210_COMM_RECEIVING,   // 正在接收
    K210_COMM_FRAME_READY, // 帧接收完成
    K210_COMM_TIMEOUT,     // 接收超时
    K210_COMM_ERROR        // 通信错误
} K210CommState_e;

// ================== K210 通信管理结构 ==================
typedef struct {
    FrameReceiver_t frame_rx;      // 帧接收器
    K210CommState_e state;         // 当前通信状态
    uint32_t last_rx_time;         // 最后接收时间（系统 tick）
    uint32_t timeout_ms;           // 超时时间（毫秒）
    uint16_t frame_count;          // 接收到的有效帧数
    uint16_t error_count;          // 错误帧数
    uint16_t timeout_count;        // 超时次数
    uint8_t disease_type;          // 当前病害类型
    uint8_t confidence;            // 当前置信度
} K210Comm_t;

// ================== 公开 API ==================

// 初始化 K210 通信管理
void K210_Comm_Init(K210Comm_t *comm, uint32_t timeout_ms);

// 处理接收到的一个字节（在 UART2 接收任务中调用）
void K210_Comm_Feed(K210Comm_t *comm, uint8_t byte, uint32_t current_tick);

// 获取当前通信状态
K210CommState_e K210_Comm_GetState(K210Comm_t *comm);

// 检查是否有新的有效帧
uint8_t K210_Comm_IsFrameReady(K210Comm_t *comm);

// 获取最新的识别结果
void K210_Comm_GetResult(K210Comm_t *comm, uint8_t *disease, uint8_t *confidence, uint16_t *timestamp);

// 清除帧接收标志
void K210_Comm_ClearFrame(K210Comm_t *comm);

// 获取通信统计信息
void K210_Comm_GetStats(K210Comm_t *comm, uint16_t *frames, uint16_t *errors, uint16_t *timeouts);

// 重置通信状态（用于错误恢复）
void K210_Comm_Reset(K210Comm_t *comm);

// 获取通信质量（0-100，100 表示完美）
uint8_t K210_Comm_GetQuality(K210Comm_t *comm);

#endif /* __K210_COMM_H */