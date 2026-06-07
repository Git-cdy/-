#ifndef __UART_FRAME_H
#define __UART_FRAME_H

#include "stm32f10x.h"
#include <stdint.h>

// ================== K210 帧格式 ==================
// 帧格式：[0xAA] [病害类型 1B] [置信度 1B] [校验和 1B]
// 总长度：4 字节
// 校验和 = (病害类型 + 置信度) & 0xFF

#define FRAME_SYNC_BYTE    0xAA     // 同步字节
#define FRAME_LEN          4        // 帧总长度

// 病害类型定义（与 K210 保持一致）
#define DISEASE_HEALTHY       0x04  // 健康
#define DISEASE_EARLY_BLIGHT  0x01  // 早疫病
#define DISEASE_LATE_BLIGHT   0x02  // 晚疫病
#define DISEASE_LEAF_SPOT     0x03  // 叶斑病

// ================== 帧接收状态机 ==================
typedef enum {
    FRAME_STATE_SYNC,       // 等待同步字节 0xAA
    FRAME_STATE_TYPE,       // 等待病害类型
    FRAME_STATE_CONFIDENCE, // 等待置信度
    FRAME_STATE_CHECKSUM    // 等待校验和
} FrameState_e;

// ================== K210 帧数据结构 ==================
typedef struct {
    uint8_t disease_type;   // 病害类型（0x01-0x04）
    uint8_t confidence;     // 置信度（0-100）
} K210FrameData_t;

// ================== 完整帧结构 ==================
typedef struct {
    uint8_t sync;           // 同步字节 0xAA
    K210FrameData_t data;   // 数据字段
    uint8_t checksum;       // 校验和 = (type + confidence) & 0xFF
} K210Frame_t;

// ================== 帧接收器状态 ==================
typedef struct {
    FrameState_e state;     // 当前状态
    K210Frame_t frame;      // 接收到的帧
    uint8_t frame_ready;    // 帧接收完成标志
    uint16_t error_count;   // 错误计数（校验失败）
    uint16_t success_count; // 成功计数
} FrameReceiver_t;

// ================== 公开 API ==================

// 初始化帧接收器
void Frame_Receiver_Init(FrameReceiver_t *rx);

// 喂入一个字节到状态机
// 返回 1 表示帧完成，0 表示继续等待
uint8_t Frame_Receiver_Feed(FrameReceiver_t *rx, uint8_t byte);

// 获取已接收的帧数据（当 frame_ready=1 时有效）
K210Frame_t* Frame_Receiver_GetFrame(FrameReceiver_t *rx);

// 清除帧接收完成标志
void Frame_Receiver_ClearFlag(FrameReceiver_t *rx);

// 获取通信统计信息
void Frame_Receiver_GetStats(FrameReceiver_t *rx, uint16_t *success, uint16_t *error);

#endif /* __UART_FRAME_H */