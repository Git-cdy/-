#include "stm32f10x.h"
#include "sht30.h"
#include <stdio.h>

// ================== CRC-8 校验函数 ==================
// SHT30 使用 CRC-8 多项式：x^8 + x^5 + x^4 + 1（0x31）
static uint8_t SHT30_CRC8(uint8_t *data, uint8_t len)
{
    uint8_t crc = 0xFF;
    uint8_t i, j;

    for (i = 0; i < len; i++)
    {
        crc ^= data[i];
        for (j = 0; j < 8; j++)
        {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x31;
            else
                crc = crc << 1;
        }
    }

    return crc;
}

// ================== I2C1 初始化（PB6/PB7） ==================
static void I2C1_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    I2C_InitTypeDef I2C_InitStructure;

    // 启用时钟
    RCC_APB2PeriphClockCmd(SHT30_I2C_RCC_GPIO, ENABLE);
    RCC_APB1PeriphClockCmd(SHT30_I2C_RCC_I2C, ENABLE);

    // 配置 SCL 和 SDA 为开漏输出
    GPIO_InitStructure.GPIO_Pin = SHT30_I2C_SCL_PIN | SHT30_I2C_SDA_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;  // 开漏输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SHT30_I2C_PORT, &GPIO_InitStructure);

    printf("[SHT30] GPIO 配置完成（PB6/PB7 开漏输出）\r\n");

    // 配置 I2C
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = 0x00;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStructure.I2C_ClockSpeed = SHT30_I2C_SPEED;
    I2C_Init(SHT30_I2C, &I2C_InitStructure);

    I2C_Cmd(SHT30_I2C, ENABLE);

    printf("[SHT30] I2C1 初始化完成（100kHz）\r\n");
}

// ================== I2C 延时函数 ==================
static void I2C_Delay_ms(uint32_t ms)
{
    uint32_t i, j;
    for (i = 0; i < ms; i++)
        for (j = 0; j < 123; j++);  // 约 1ms（72MHz 时钟）
}

// ================== I2C 写入字节 ==================
static uint8_t I2C_Write_Byte(uint8_t data)
{
    uint32_t timeout = 0;

    // 等待 I2C 总线空闲
    while (I2C_GetFlagStatus(SHT30_I2C, I2C_FLAG_BUSY) && timeout < 10000)
        timeout++;
    if (timeout >= 10000) return 0;

    // 发送起始条件
    I2C_GenerateSTART(SHT30_I2C, ENABLE);
    timeout = 0;
    while (!I2C_CheckEvent(SHT30_I2C, I2C_EVENT_MASTER_MODE_SELECT) && timeout < 10000)
        timeout++;
    if (timeout >= 10000) return 0;

    // 发送地址 + 写入位
    I2C_Send7bitAddress(SHT30_I2C, SHT30_I2C_ADDR << 1, I2C_Direction_Transmitter);
    timeout = 0;
    while (!I2C_CheckEvent(SHT30_I2C, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) && timeout < 10000)
        timeout++;
    if (timeout >= 10000) return 0;

    // 发送数据字节
    I2C_SendData(SHT30_I2C, data);
    timeout = 0;
    while (!I2C_CheckEvent(SHT30_I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTED) && timeout < 10000)
        timeout++;
    if (timeout >= 10000) return 0;

    return 1;
}

// ================== I2C 读取字节 ==================
static uint8_t I2C_Read_Byte(uint8_t *data, uint8_t ack)
{
    uint32_t timeout = 0;

    if (ack)
        I2C_AcknowledgeConfig(SHT30_I2C, ENABLE);
    else
        I2C_AcknowledgeConfig(SHT30_I2C, DISABLE);

    timeout = 0;
    while (!I2C_CheckEvent(SHT30_I2C, I2C_EVENT_MASTER_BYTE_RECEIVED) && timeout < 10000)
        timeout++;
    if (timeout >= 10000) return 0;

    *data = I2C_ReceiveData(SHT30_I2C);
    return 1;
}

// ================== SHT30 软复位 ==================
static uint8_t SHT30_Soft_Reset(void)
{
    uint32_t timeout = 0;

    // 发送起始条件
    I2C_GenerateSTART(SHT30_I2C, ENABLE);
    timeout = 0;
    while (!I2C_CheckEvent(SHT30_I2C, I2C_EVENT_MASTER_MODE_SELECT) && timeout < 10000)
        timeout++;
    if (timeout >= 10000) return 0;

    // 发送地址 + 写入位
    I2C_Send7bitAddress(SHT30_I2C, SHT30_I2C_ADDR << 1, I2C_Direction_Transmitter);
    timeout = 0;
    while (!I2C_CheckEvent(SHT30_I2C, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) && timeout < 10000)
        timeout++;
    if (timeout >= 10000) return 0;

    // 发送复位命令（2 字节）
    I2C_SendData(SHT30_I2C, (SHT30_CMD_SOFT_RESET >> 8) & 0xFF);
    timeout = 0;
    while (!I2C_CheckEvent(SHT30_I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTED) && timeout < 10000)
        timeout++;
    if (timeout >= 10000) return 0;

    I2C_SendData(SHT30_I2C, SHT30_CMD_SOFT_RESET & 0xFF);
    timeout = 0;
    while (!I2C_CheckEvent(SHT30_I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTED) && timeout < 10000)
        timeout++;
    if (timeout >= 10000) return 0;

    // 发送停止条件
    I2C_GenerateSTOP(SHT30_I2C, ENABLE);
    I2C_Delay_ms(10);  // 等待复位完成

    return 1;
}

// ================== SHT30 初始化 ==================
void SHT30_Init(void)
{
    printf("\r\n[SHT30] ========== 初始化开始 ==========\r\n");

    I2C1_Init();

    // 尝试软复位
    printf("[SHT30] 尝试软复位...\r\n");
    if (SHT30_Soft_Reset())
    {
        printf("[SHT30] 软复位成功\r\n");
    }
    else
    {
        printf("[SHT30] 软复位失败 - I2C 通信可能有问题\r\n");
        printf("[SHT30] 检查项：\r\n");
        printf("  1. SHT30 是否正确接线（SCL=PB6, SDA=PB7）\r\n");
        printf("  2. 是否有 4.7kΩ 上拉电阻到 3.3V\r\n");
        printf("  3. SHT30 VCC 是否接 3.3V，GND 是否接地\r\n");
        printf("  4. 用万用表测 PB6/PB7 是否有 3.3V 电压\r\n");
    }

    printf("[SHT30] ========== 初始化完成 ==========\r\n\r\n");
}

// ================== SHT30 读取温湿度 ==================
uint8_t SHT30_Read_Data(int8_t *temp, uint8_t *humi)
{
    uint8_t data[6];
    uint32_t timeout = 0;
    uint16_t temp_raw, humi_raw;
    int16_t temp_val;
    uint8_t crc_temp, crc_humi;

    // 发送起始条件
    I2C_GenerateSTART(SHT30_I2C, ENABLE);
    timeout = 0;
    while (!I2C_CheckEvent(SHT30_I2C, I2C_EVENT_MASTER_MODE_SELECT) && timeout < 10000)
        timeout++;
    if (timeout >= 10000) return 0;

    // 发送地址 + 写入位
    I2C_Send7bitAddress(SHT30_I2C, SHT30_I2C_ADDR << 1, I2C_Direction_Transmitter);
    timeout = 0;
    while (!I2C_CheckEvent(SHT30_I2C, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) && timeout < 10000)
        timeout++;
    if (timeout >= 10000) return 0;

    // 发送测量命令（2 字节）
    I2C_SendData(SHT30_I2C, (SHT30_CMD_MEAS_SINGLE >> 8) & 0xFF);
    timeout = 0;
    while (!I2C_CheckEvent(SHT30_I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTED) && timeout < 10000)
        timeout++;
    if (timeout >= 10000) return 0;

    I2C_SendData(SHT30_I2C, SHT30_CMD_MEAS_SINGLE & 0xFF);
    timeout = 0;
    while (!I2C_CheckEvent(SHT30_I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTED) && timeout < 10000)
        timeout++;
    if (timeout >= 10000) return 0;

    // 发送停止条件
    I2C_GenerateSTOP(SHT30_I2C, ENABLE);
    I2C_Delay_ms(20);  // 等待测量完成

    // 读取数据
    I2C_GenerateSTART(SHT30_I2C, ENABLE);
    timeout = 0;
    while (!I2C_CheckEvent(SHT30_I2C, I2C_EVENT_MASTER_MODE_SELECT) && timeout < 10000)
        timeout++;
    if (timeout >= 10000) return 0;

    // 发送地址 + 读取位
    I2C_Send7bitAddress(SHT30_I2C, SHT30_I2C_ADDR << 1, I2C_Direction_Receiver);
    timeout = 0;
    while (!I2C_CheckEvent(SHT30_I2C, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED) && timeout < 10000)
        timeout++;
    if (timeout >= 10000) return 0;

    // 读取 6 字节数据（温度 2 字节 + CRC 1 字节 + 湿度 2 字节 + CRC 1 字节）
    for (int i = 0; i < 6; i++)
    {
        if (i < 5)
            I2C_AcknowledgeConfig(SHT30_I2C, ENABLE);
        else
            I2C_AcknowledgeConfig(SHT30_I2C, DISABLE);

        timeout = 0;
        while (!I2C_CheckEvent(SHT30_I2C, I2C_EVENT_MASTER_BYTE_RECEIVED) && timeout < 10000)
            timeout++;
        if (timeout >= 10000) return 0;

        data[i] = I2C_ReceiveData(SHT30_I2C);
    }

    // 发送停止条件
    I2C_GenerateSTOP(SHT30_I2C, ENABLE);

    // ========== CRC 校验 ==========
    // 校验温度数据（data[0] + data[1]）
    crc_temp = SHT30_CRC8(&data[0], 2);
    if (crc_temp != data[2])
    {
        printf("[SHT30] 温度 CRC 校验失败: 期望 0x%02X, 实际 0x%02X (原始数据: 0x%02X 0x%02X)\r\n",
               data[2], crc_temp, data[0], data[1]);
        return 0;
    }

    // 校验湿度数据（data[3] + data[4]）
    crc_humi = SHT30_CRC8(&data[3], 2);
    if (crc_humi != data[5])
    {
        printf("[SHT30] 湿度 CRC 校验失败: 期望 0x%02X, 实际 0x%02X (原始数据: 0x%02X 0x%02X)\r\n",
               data[5], crc_humi, data[3], data[4]);
        return 0;
    }

    // ========== 数据解析 ==========
    temp_raw = (data[0] << 8) | data[1];
    humi_raw = (data[3] << 8) | data[4];

    // 温度转换：T = -45 + 175 * (temp_raw / 65535)
    // 改用 int16_t 支持负数，然后转换为 int8_t
    temp_val = (175 * temp_raw) / 65535 - 45;
    *temp = (int8_t)temp_val;

    // 湿度转换：RH = 100 * (humi_raw / 65535)
    *humi = (uint8_t)((100 * humi_raw) / 65535);

    return 1;
}

