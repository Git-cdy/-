#ifndef __UART_FRAME_H
#define __UART_FRAME_H

#include "stm32f10x.h"
#include <stdint.h>

// ================== K210 帧结构 ==================
// 帧格式：[0xAA] [病害类别 1B] [置信度 1B] [校验和 1B]
#define FRAME_SYNC_BYTE    0xAA     // 同步字
#define FRAME_LEN          4        // 帧长度（包括同步字）

// 病害类别定义（与 K210 保持一致）
#define DISEASE_EARLY_BLIGHT  0x01  // 早疫病
#define DISEASE_LATE_BLIGHT   0x02  // 晚疫病
#define DISEASE_LEAF_SPOT     0x03  // 叶斑病
#define DISEASE_HEALTHY       0x04  // 健康

// ================== 帧接收状态机 ==================
typedef enum {
    FRAME_STATE_SYNC,       // 等待同步字
    FRAME_STATE_DISEASE,    // 等待病害类别
    FRAME_STATE_CONFIDENCE, // 等待置信度
    FRAME_STATE_CHECKSUM    // 等待校验和
} FrameState_e;

// ================== K210 帧数据结构 ==================
typedef struct {
    uint8_t disease_type;   // 病害类别（0x01-0x04）
    uint8_t confidence;     // 置信度（0-100）
    uint8_t checksum;       // 校验和
} K210Frame_t;

// ================== 帧接收器对象 ==================
typedef struct {
    FrameState_e state;     // 当前状态
    K210Frame_t frame;      // 完整帧数据
    uint8_t frame_ready;    // 帧就绪标志（1=收到完整有效帧）
} FrameReceiver_t;

// ================== 公共 API ==================

// 初始化帧接收器
void Frame_Receiver_Init(FrameReceiver_t *rx);

// 喂入一个字节到状态机
// 返回 1 如果帧完成（且校验正确），否则返回 0
uint8_t Frame_Receiver_Feed(FrameReceiver_t *rx, uint8_t byte);

// 获取已接收的帧数据（仅在 frame_ready=1 时有效）
K210Frame_t* Frame_Receiver_GetFrame(FrameReceiver_t *rx);

// 清除帧就绪标志
void Frame_Receiver_ClearFlag(FrameReceiver_t *rx);

#endif /* __UART_FRAME_H */
