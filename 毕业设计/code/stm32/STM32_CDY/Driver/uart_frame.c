#include "uart_frame.h"
#include <stdio.h>
#include <string.h>

// ================== 初始化帧接收器 ==================
void Frame_Receiver_Init(FrameReceiver_t *rx)
{
    rx->state = FRAME_STATE_SYNC;
    rx->frame_ready = 0;
    rx->error_count = 0;
    rx->success_count = 0;
    memset(&rx->frame, 0, sizeof(rx->frame));
}

// ================== 状态机：喂入一个字节 ==================
uint8_t Frame_Receiver_Feed(FrameReceiver_t *rx, uint8_t byte)
{
    uint8_t expected_checksum;

    switch (rx->state)
    {
        case FRAME_STATE_SYNC:
            // 等待同步字节 0xAA
            if (byte == FRAME_SYNC_BYTE)
            {
                rx->frame.sync = byte;
                rx->state = FRAME_STATE_TYPE;
            }
            break;

        case FRAME_STATE_TYPE:
            // 接收病害类型（0x01-0x04）
            if (byte >= 0x01 && byte <= 0x04)
            {
                rx->frame.data.disease_type = byte;
                rx->state = FRAME_STATE_CONFIDENCE;
            }
            else
            {
                // 无效类型，回到同步状态
        // printf("[帧接收] 无效病害类型: 0x%02X\r\n", byte);
                rx->state = FRAME_STATE_SYNC;
            }
            break;

        case FRAME_STATE_CONFIDENCE:
            // 接收置信度（0-100）
            rx->frame.data.confidence = byte;
            rx->state = FRAME_STATE_CHECKSUM;
            break;

        case FRAME_STATE_CHECKSUM:
            // 验证校验和 = (type + confidence) & 0xFF
            rx->frame.checksum = byte;
            expected_checksum = (rx->frame.data.disease_type + rx->frame.data.confidence) & 0xFF;

            if (rx->frame.checksum == expected_checksum)
            {
                // 校验通过
                rx->frame_ready = 1;
                rx->success_count++;
/*
    printf("[帧接收] OK: 病害=0x%02X, 置信度=%d%%\r\n",
    rx->frame.data.disease_type,
    rx->frame.data.confidence);
    */
            }
            else
            {
                // 校验失败
                rx->error_count++;
/*
    printf("[帧接收] 校验失败: 期望=0x%02X, 实际=0x%02X\r\n",
    expected_checksum,
    rx->frame.checksum);
    */
            }

            // 回到等待同步状态
            rx->state = FRAME_STATE_SYNC;
            break;

        default:
            // 异常状态，重置
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

// ================== 清除帧接收完成标志 ==================
void Frame_Receiver_ClearFlag(FrameReceiver_t *rx)
{
    rx->frame_ready = 0;
}

// ================== 获取通信统计信息 ==================
void Frame_Receiver_GetStats(FrameReceiver_t *rx, uint16_t *success, uint16_t *error)
{
    if (success) *success = rx->success_count;
    if (error) *error = rx->error_count;
}
