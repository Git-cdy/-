#include "stm32f10x.h"
#include "sht30.h"
#include <stdio.h>

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

    // 配置 I2C
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = 0x00;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStructure.I2C_ClockSpeed = SHT30_I2C_SPEED;
    I2C_Init(SHT30_I2C, &I2C_InitStructure);

    I2C_Cmd(SHT30_I2C, ENABLE);
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
    I2C1_Init();
    SHT30_Soft_Reset();
    printf("[SHT30] 初始化完成\r\n");
}

// ================== SHT30 读取温湿度 ==================
uint8_t SHT30_Read_Data(uint8_t *temp, uint8_t *humi)
{
    uint8_t data[6];
    uint32_t timeout = 0;
    uint16_t temp_raw, humi_raw;
    int temp_val;

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

    // 解析温度和湿度
    temp_raw = (data[0] << 8) | data[1];
    humi_raw = (data[3] << 8) | data[4];

    // 温度转换：T = -45 + 175 * (temp_raw / 65535)
    // 简化为整数：T = (175 * temp_raw) / 65535 - 45
    temp_val = (175 * temp_raw) / 65535 - 45;
    *temp = (uint8_t)(temp_val & 0xFF);

    // 湿度转换：RH = 100 * (humi_raw / 65535)
    // 简化为整数：RH = (100 * humi_raw) / 65535
    *humi = (uint8_t)((100 * humi_raw) / 65535);

    return 1;
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
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) && timeout < 10000)
        timeout++;
    if (timeout >= 10000) return 0;

    // 发送起始条件
    I2C_GenerateSTART(I2C1, ENABLE);
    timeout = 0;
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT) && timeout < 10000)
        timeout++;
    if (timeout >= 10000) return 0;

    // 发送地址 + 写入位
    I2C_Send7bitAddress(I2C1, SHT30_I2C_ADDR << 1, I2C_Direction_Transmitter);
    timeout = 0;
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) && timeout < 10000)
        timeout++;
    if (timeout >= 10000) return 0;

    // 发送数据字节
    I2C_SendData(I2C1, data);
    timeout = 0;
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED) && timeout < 10000)
        timeout++;
    if (timeout >= 10000) return 0;

    return 1;
}

// ================== I2C 读取字节 ==================
static uint8_t I2C_Read_Byte(uint8_t *data, uint8_t ack)
{
    uint32_t timeout = 0;

    if (ack)
        I2C_AcknowledgeConfig(I2C1, ENABLE);
    else
        I2C_AcknowledgeConfig(I2C1, DISABLE);

    timeout = 0;
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED) && timeout < 10000)
        timeout++;
    if (timeout >= 10000) return 0;

    *data = I2C_ReceiveData(I2C1);
    return 1;
}

// ================== SHT30 软复位 ==================
static uint8_t SHT30_Soft_Reset(void)
{
    uint32_t timeout = 0;

    // 发送起始条件
    I2C_GenerateSTART(I2C1, ENABLE);
    timeout = 0;
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT) && timeout < 10000)
        timeout++;
    if (timeout >= 10000) return 0;

    // 发送地址 + 写入位
    I2C_Send7bitAddress(I2C1, SHT30_I2C_ADDR << 1, I2C_Direction_Transmitter);
    timeout = 0;
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) && timeout < 10000)
        timeout++;
    if (timeout >= 10000) return 0;

    // 发送复位命令（2 字节）
    I2C_SendData(I2C1, (SHT30_CMD_SOFT_RESET >> 8) & 0xFF);
    timeout = 0;
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED) && timeout < 10000)
        timeout++;
    if (timeout >= 10000) return 0;

    I2C_SendData(I2C1, SHT30_CMD_SOFT_RESET & 0xFF);
    timeout = 0;
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED) && timeout < 10000)
        timeout++;
    if (timeout >= 10000) return 0;

    // 发送停止条件
    I2C_GenerateSTOP(I2C1, ENABLE);
    I2C_Delay_ms(10);  // 等待复位完成

    return 1;
}

// ================== SHT30 初始化 ==================
void SHT30_Init(void)
{
    I2C1_Init();
    SHT30_Soft_Reset();
    printf("[SHT30] 初始化完成\r\n");
}

// ================== SHT30 读取温湿度 ==================
uint8_t SHT30_Read_Data(uint8_t *temp, uint8_t *humi)
{
    uint8_t data[6];
    uint32_t timeout = 0;
    uint16_t temp_raw, humi_raw;
    int temp_val;

    // 发送起始条件
    I2C_GenerateSTART(I2C1, ENABLE);
    timeout = 0;
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT) && timeout < 10000)
        timeout++;
    if (timeout >= 10000) return 0;

    // 发送地址 + 写入位
    I2C_Send7bitAddress(I2C1, SHT30_I2C_ADDR << 1, I2C_Direction_Transmitter);
    timeout = 0;
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) && timeout < 10000)
        timeout++;
    if (timeout >= 10000) return 0;

    // 发送测量命令（2 字节）
    I2C_SendData(I2C1, (SHT30_CMD_MEAS_SINGLE >> 8) & 0xFF);
    timeout = 0;
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED) && timeout < 10000)
        timeout++;
    if (timeout >= 10000) return 0;

    I2C_SendData(I2C1, SHT30_CMD_MEAS_SINGLE & 0xFF);
    timeout = 0;
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED) && timeout < 10000)
        timeout++;
    if (timeout >= 10000) return 0;

    // 发送停止条件
    I2C_GenerateSTOP(I2C1, ENABLE);
    I2C_Delay_ms(20);  // 等待测量完成

    // 读取数据
    I2C_GenerateSTART(I2C1, ENABLE);
    timeout = 0;
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT) && timeout < 10000)
        timeout++;
    if (timeout >= 10000) return 0;

    // 发送地址 + 读取位
    I2C_Send7bitAddress(I2C1, SHT30_I2C_ADDR << 1, I2C_Direction_Receiver);
    timeout = 0;
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED) && timeout < 10000)
        timeout++;
    if (timeout >= 10000) return 0;

    // 读取 6 字节数据（温度 2 字节 + CRC 1 字节 + 湿度 2 字节 + CRC 1 字节）
    for (int i = 0; i < 6; i++)
    {
        if (i < 5)
            I2C_AcknowledgeConfig(I2C1, ENABLE);
        else
            I2C_AcknowledgeConfig(I2C1, DISABLE);

        timeout = 0;
        while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED) && timeout < 10000)
            timeout++;
        if (timeout >= 10000) return 0;

        data[i] = I2C_ReceiveData(I2C1);
    }

    // 发送停止条件
    I2C_GenerateSTOP(I2C1, ENABLE);

    // 解析温度和湿度
    temp_raw = (data[0] << 8) | data[1];
    humi_raw = (data[3] << 8) | data[4];

    // 温度转换：T = -45 + 175 * (temp_raw / 65535)
    // 简化为整数：T = (175 * temp_raw) / 65535 - 45
    temp_val = (175 * temp_raw) / 65535 - 45;
    *temp = (uint8_t)(temp_val & 0xFF);

    // 湿度转换：RH = 100 * (humi_raw / 65535)
    // 简化为整数：RH = (100 * humi_raw) / 65535
    *humi = (uint8_t)((100 * humi_raw) / 65535);

    return 1;
}
