#include "k210_comm.h"
#include <stdio.h>
#include <string.h>

// ================== 初始化 K210 通信管理 ==================
void K210_Comm_Init(K210Comm_t *comm, uint32_t timeout_ms)
{
    memset(comm, 0, sizeof(K210Comm_t));
    Frame_Receiver_Init(&comm->frame_rx);
    comm->state = K210_COMM_IDLE;
    comm->timeout_ms = timeout_ms;
    comm->last_rx_time = 0;
    printf("[K210通信] 初始化完成，超时时间=%dms\r\n", timeout_ms);
}

// ================== 处理接收到的一个字节 ==================
void K210_Comm_Feed(K210Comm_t *comm, uint8_t byte, uint32_t current_tick)
{
    // 喂入字节到帧接收器
    if (Frame_Receiver_Feed(&comm->frame_rx, byte))
    {
        // 帧接收完成
        K210Frame_t *frame = Frame_Receiver_GetFrame(&comm->frame_rx);
        if (frame)
        {
            // 有效帧
            comm->state = K210_COMM_FRAME_READY;
            comm->disease_type = frame->data.disease_type;
            comm->confidence = frame->data.confidence;
            comm->last_rx_time = current_tick;
            comm->frame_count++;

            printf("[K210通信] 新帧: 病害=0x%02X, 置信度=%d%%\r\n",
                   comm->disease_type, comm->confidence);
        }
        else
        {
            // 帧校验失败
            comm->state = K210_COMM_ERROR;
            comm->error_count++;
            printf("[K210通信] 帧校验失败\r\n");
        }

        Frame_Receiver_ClearFlag(&comm->frame_rx);
    }
    else
    {
        // 继续接收
        if (comm->state == K210_COMM_IDLE)
            comm->state = K210_COMM_RECEIVING;
    }

    // 检查超时
    if (current_tick - comm->last_rx_time > comm->timeout_ms && comm->last_rx_time > 0)
    {
        if (comm->state != K210_COMM_TIMEOUT)
        {
            comm->state = K210_COMM_TIMEOUT;
            comm->timeout_count++;
            printf("[K210通信] 接收超时 (>%dms)\r\n", comm->timeout_ms);
        }
    }
}

// ================== 获取当前通信状态 ==================
K210CommState_e K210_Comm_GetState(K210Comm_t *comm)
{
    return comm->state;
}

// ================== 检查是否有新的有效帧 ==================
uint8_t K210_Comm_IsFrameReady(K210Comm_t *comm)
{
    return (comm->state == K210_COMM_FRAME_READY);
}

// ================== 获取最新的识别结果 ==================
void K210_Comm_GetResult(K210Comm_t *comm, uint8_t *disease, uint8_t *confidence, uint16_t *timestamp)
{
    if (disease) *disease = comm->disease_type;
    if (confidence) *confidence = comm->confidence;
    if (timestamp) *timestamp = 0;
}

// ================== 清除帧接收标志 ==================
void K210_Comm_ClearFrame(K210Comm_t *comm)
{
    if (comm->state == K210_COMM_FRAME_READY)
    {
        comm->state = K210_COMM_IDLE;
    }
}

// ================== 获取通信统计信息 ==================
void K210_Comm_GetStats(K210Comm_t *comm, uint16_t *frames, uint16_t *errors, uint16_t *timeouts)
{
    if (frames) *frames = comm->frame_count;
    if (errors) *errors = comm->error_count;
    if (timeouts) *timeouts = comm->timeout_count;
}

// ================== 重置通信状态（用于错误恢复）==================
void K210_Comm_Reset(K210Comm_t *comm)
{
    Frame_Receiver_Init(&comm->frame_rx);
    comm->state = K210_COMM_IDLE;
    comm->last_rx_time = 0;
    printf("[K210通信] 通信已重置\r\n");
}

// ================== 获取通信质量（0-100）==================
uint8_t K210_Comm_GetQuality(K210Comm_t *comm)
{
    uint16_t total = comm->frame_count + comm->error_count;

    if (total == 0)
        return 100;  // 没有数据时认为质量完美

    // 质量 = (有效帧数 / 总帧数) * 100
    uint8_t quality = (comm->frame_count * 100) / total;

    // 如果有超时，进一步降低质量
    if (comm->timeout_count > 0)
    {
        quality = (quality * 80) / 100;  // 降低 20%
    }

    return quality;
}