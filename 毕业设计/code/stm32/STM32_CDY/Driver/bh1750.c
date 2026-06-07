#include "stm32f10x.h"
#include "bh1750.h"
#include <stdio.h>

// ================== I2C 延时函数（复用 SHT30 的延时） ==================
static void I2C_Delay_ms(uint32_t ms)
{
    uint32_t i, j;
        for (j = 0; j < 6000; j++);  // 校准至约 1ms（72MHz, -O0）
        for (j = 0; j < 123; j++);  // 约 1ms（72MHz 时钟）
}

// ================== BH1750 初始化 ==================
void BH1750_Init(void)
{
    printf("\r\n[BH1750] ========== 初始化开始 ==========\r\n");

    // I2C1 已由 SHT30 初始化，无需重复初始化
    printf("[BH1750] I2C1 已由 SHT30 初始化，复用中...\r\n");

    // 发送电源开启命令
    uint32_t timeout = 0;

    // 发送起始条件
    I2C_GenerateSTART(BH1750_I2C, ENABLE);
    timeout = 0;
    while (!I2C_CheckEvent(BH1750_I2C, I2C_EVENT_MASTER_MODE_SELECT) && timeout < 10000)
        timeout++;
    if (timeout >= 10000)
    {
        printf("[BH1750] 初始化失败：发送起始条件超时\r\n");
        return;
    }

    // 发送地址 + 写入位
    I2C_Send7bitAddress(BH1750_I2C, BH1750_I2C_ADDR << 1, I2C_Direction_Transmitter);
    timeout = 0;
    while (!I2C_CheckEvent(BH1750_I2C, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) && timeout < 10000)
        timeout++;
    if (timeout >= 10000)
    {
        printf("[BH1750] 初始化失败：地址发送超时\r\n");
        return;
    }

    // 发送电源开启命令
    I2C_SendData(BH1750_I2C, BH1750_CMD_POWER_ON);
    timeout = 0;
    while (!I2C_CheckEvent(BH1750_I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTED) && timeout < 10000)
        timeout++;
    if (timeout >= 10000)
    {
        printf("[BH1750] 初始化失败：命令发送超时\r\n");
        return;
    }

    // 发送停止条件
    I2C_GenerateSTOP(BH1750_I2C, ENABLE);
    I2C_Delay_ms(10);

    // 发送重置命令
    I2C_GenerateSTART(BH1750_I2C, ENABLE);
    timeout = 0;
    while (!I2C_CheckEvent(BH1750_I2C, I2C_EVENT_MASTER_MODE_SELECT) && timeout < 10000)
        timeout++;

    I2C_Send7bitAddress(BH1750_I2C, BH1750_I2C_ADDR << 1, I2C_Direction_Transmitter);
    timeout = 0;
    while (!I2C_CheckEvent(BH1750_I2C, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) && timeout < 10000)
        timeout++;

    I2C_SendData(BH1750_I2C, BH1750_CMD_RESET);
    timeout = 0;
    while (!I2C_CheckEvent(BH1750_I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTED) && timeout < 10000)
        timeout++;

    I2C_GenerateSTOP(BH1750_I2C, ENABLE);
    I2C_Delay_ms(10);

    printf("[BH1750] 初始化成功（地址 0x23，连续高分辨率模式）\r\n");
    printf("[BH1750] ========== 初始化完成 ==========\r\n\r\n");
}

// ================== BH1750 读取光照强度 ==================
uint8_t BH1750_Read_Lux(uint16_t *lux)
{
    uint8_t data[2];
    uint32_t timeout = 0;

    // 检查 I2C 总线是否被卡住
    if (I2C_GetFlagStatus(BH1750_I2C, I2C_FLAG_BUSY))
    {
        printf("[BH1750] I2C BUSY - GPIO recovery...\r\n");
        I2C_Cmd(BH1750_I2C, DISABLE);
        {
            GPIO_InitTypeDef g;
            g.GPIO_Pin   = GPIO_Pin_6 | GPIO_Pin_7;
            g.GPIO_Mode  = GPIO_Mode_Out_OD;
            g.GPIO_Speed = GPIO_Speed_50MHz;
            GPIO_Init(GPIOB, &g);
            GPIO_SetBits(GPIOB, GPIO_Pin_7);
            for (int k = 0; k < 16; k++)
            {
                GPIO_ResetBits(GPIOB, GPIO_Pin_6); I2C_Delay_ms(1);
                GPIO_SetBits(GPIOB, GPIO_Pin_6);   I2C_Delay_ms(1);
                if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_7)) break;
            }
            GPIO_ResetBits(GPIOB, GPIO_Pin_6); I2C_Delay_ms(1);
            GPIO_ResetBits(GPIOB, GPIO_Pin_7); I2C_Delay_ms(1);
            GPIO_SetBits(GPIOB, GPIO_Pin_6);   I2C_Delay_ms(1);
            GPIO_SetBits(GPIOB, GPIO_Pin_7);   I2C_Delay_ms(1);
            g.GPIO_Mode = GPIO_Mode_AF_OD;
            GPIO_Init(GPIOB, &g);
        }
        I2C_Cmd(BH1750_I2C, ENABLE);
        I2C_Delay_ms(10);
        printf("[BH1750] I2C recovery done\r\n");
    }

    // 发送测量命令（连续高分辨率模式）
    I2C_GenerateSTART(BH1750_I2C, ENABLE);
    timeout = 0;
    while (!I2C_CheckEvent(BH1750_I2C, I2C_EVENT_MASTER_MODE_SELECT) && timeout < 10000)
        timeout++;
    if (timeout >= 10000)
    {
        printf("[BH1750] 读取失败：发送起始条件超时\r\n");
        return 0;
    }

    // 发送地址 + 写入位
    I2C_Send7bitAddress(BH1750_I2C, BH1750_I2C_ADDR << 1, I2C_Direction_Transmitter);
    timeout = 0;
    while (!I2C_CheckEvent(BH1750_I2C, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) && timeout < 10000)
        timeout++;
    if (timeout >= 10000)
    {
        printf("[BH1750] 读取失败：地址发送超时\r\n");
        return 0;
    }

    // 发送测量命令
    I2C_SendData(BH1750_I2C, BH1750_CMD_CONT_H);
    timeout = 0;
    while (!I2C_CheckEvent(BH1750_I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTED) && timeout < 10000)
        timeout++;
    if (timeout >= 10000)
    {
        printf("[BH1750] 读取失败：命令发送超时\r\n");
        return 0;
    }

    // 发送停止条件，等待测量完成（120ms）
    I2C_GenerateSTOP(BH1750_I2C, ENABLE);
    I2C_Delay_ms(120);

    // 读取数据
    I2C_GenerateSTART(BH1750_I2C, ENABLE);
    timeout = 0;
    while (!I2C_CheckEvent(BH1750_I2C, I2C_EVENT_MASTER_MODE_SELECT) && timeout < 10000)
        timeout++;
    if (timeout >= 10000)
    {
        printf("[BH1750] 读取失败：读取起始条件超时\r\n");
        return 0;
    }

    // 发送地址 + 读取位
    I2C_Send7bitAddress(BH1750_I2C, BH1750_I2C_ADDR << 1, I2C_Direction_Receiver);
    timeout = 0;
    while (!I2C_CheckEvent(BH1750_I2C, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED) && timeout < 10000)
        timeout++;
    if (timeout >= 10000)
    {
        printf("[BH1750] 读取失败：读取地址发送超时\r\n");
        return 0;
    }

    // 读取 2 字节数据（高字节 + 低字节）
    // 第一字节：ACK
    I2C_AcknowledgeConfig(BH1750_I2C, ENABLE);
    timeout = 0;
    while (!I2C_CheckEvent(BH1750_I2C, I2C_EVENT_MASTER_BYTE_RECEIVED) && timeout < 10000)
        timeout++;
    if (timeout >= 10000)
    {
        printf("[BH1750] 读取失败：第一字节接收超时\r\n");
        return 0;
    }
    data[0] = I2C_ReceiveData(BH1750_I2C);

    // 第二字节：NACK
    I2C_AcknowledgeConfig(BH1750_I2C, DISABLE);
    timeout = 0;
    while (!I2C_CheckEvent(BH1750_I2C, I2C_EVENT_MASTER_BYTE_RECEIVED) && timeout < 10000)
        timeout++;
    if (timeout >= 10000)
    {
        printf("[BH1750] 读取失败：第二字节接收超时\r\n");
        return 0;
    }
    data[1] = I2C_ReceiveData(BH1750_I2C);

    // 发送停止条件
    I2C_GenerateSTOP(BH1750_I2C, ENABLE);

    // 解析光照强度
    // BH1750 返回的是 16 位数据（高字节在前）
    // 光照强度 = (data[0] << 8 | data[1]) / 1.2
    // 简化为整数：lux = (data[0] * 256 + data[1]) / 1.2
    *lux = (uint16_t)((data[0] << 8 | data[1]) * 10 / 12);  // 乘以 10 再除以 12 避免浮点

    return 1;
}
