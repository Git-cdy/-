#include "uart_frame.h"
#include <stdio.h>
#include <string.h>

// ================== 校验和计算 ==================
// 简单的和校验
static uint8_t Frame_CalculateChecksum(uint8_t disease_type, uint8_t confidence)
{
    return (disease_type + confidence) & 0xFF;
}

// ================== 初始化帧接收器 ==================
void Frame_Receiver_Init(FrameReceiver_t *rx)
{
    rx->state = FRAME_STATE_SYNC;
    rx->frame_ready = 0;
    memset(&rx->frame, 0, sizeof(rx->frame));
}

// ================== 状态机：喂入一个字节 ==================
uint8_t Frame_Receiver_Feed(FrameReceiver_t *rx, uint8_t byte)
{
    uint8_t expected_checksum;

    switch (rx->state)
    {
        case FRAME_STATE_SYNC:
            // 等待同步字 0xAA
            if (byte == FRAME_SYNC_BYTE)
            {
                rx->state = FRAME_STATE_DISEASE;
            }
            break;

        case FRAME_STATE_DISEASE:
            // 接收病害类别
            rx->frame.disease_type = byte;
            rx->state = FRAME_STATE_CONFIDENCE;
            break;

        case FRAME_STATE_CONFIDENCE:
            // 接收置信度
            rx->frame.confidence = byte;
            rx->state = FRAME_STATE_CHECKSUM;
            break;

        case FRAME_STATE_CHECKSUM:
            // 接收校验和，并验证
            rx->frame.checksum = byte;

            expected_checksum = Frame_CalculateChecksum(
                rx->frame.disease_type,
                rx->frame.confidence
            );

            if (rx->frame.checksum == expected_checksum)
            {
                // 校验通过，标记帧就绪
                rx->frame_ready = 1;
                printf("[FrameRx] 帧有效: 类别=0x%02X, 置信度=%d%%, 校验=0x%02X\r\n",
                       rx->frame.disease_type,
                       rx->frame.confidence,
                       rx->frame.checksum);
            }
            else
            {
                // 校验失败，弹出调试信息
                printf("[FrameRx] 校验失败: 期望=0x%02X, 实际=0x%02X\r\n",
                       expected_checksum,
                       rx->frame.checksum);
            }

            // 无论校验成功或失败，都回到等待同步字状态
            rx->state = FRAME_STATE_SYNC;
            break;

        default:
            // 不应该到达这里
            rx->state = FRAME_STATE_SYNC;
            break;
    }

    return rx->frame_ready;
}

// ================== 获取已接收的帧数据 ==================
K210Frame_t* Frame_Receiver_GetFrame(FrameReceiver_t *rx)
{
    if (rx->frame_ready)
    {
        return &rx->frame;
    }
    return NULL;
}

// ================== 清除帧就绪标志 ==================
void Frame_Receiver_ClearFlag(FrameReceiver_t *rx)
{
    rx->frame_ready = 0;
}
