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
        for (j = 0; j < 6000; j++);  // 校准后约 1ms（72MHz, -O0）
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
    int8_t test_temp;
    uint8_t test_humi;
    uint8_t init_retry = 0;

    printf("\r\n[SHT30] ========== 初始化开始 ==========\r\n");

    I2C1_Init();

    // 尝试软复位
    printf("[SHT30] 尝试软复位...\r\n");
    if (!SHT30_Soft_Reset())
    {
        printf("[SHT30] 软复位失败 - I2C 通信可能有问题\r\n");
        printf("[SHT30] 检查项：\r\n");
        printf("  1. SHT30 是否正确接线（SCL=PB6, SDA=PB7）\r\n");
        printf("  2. 是否有 4.7kΩ 上拉电阻到 3.3V\r\n");
        printf("  3. SHT30 VCC 是否接 3.3V，GND 是否接地\r\n");
        printf("  4. 用万用表测 PB6/PB7 是否有 3.3V 电压\r\n");
        printf("  5. 检查 I2C1 是否被其他设备（BH1750）占用\r\n");
        printf("[SHT30] ========== 初始化失败 ==========\r\n\r\n");
        return;
    }

    printf("[SHT30] 软复位成功\r\n");

    // 初始化验证：尝试读取一次数据
    printf("[SHT30] 验证通信...\r\n");
    I2C_Delay_ms(50);  // 等待复位完成

    while (init_retry < 3 && !SHT30_Read_Data(&test_temp, &test_humi))
    {
        init_retry++;
        printf("[SHT30] 初始化验证失败，重试 %d/3\r\n", init_retry);
        I2C_Delay_ms(100);
    }

    if (init_retry >= 3)
    {
        printf("[SHT30] 初始化验证失败 3 次 - 传感器无响应\r\n");
        printf("[SHT30] ========== 初始化失败 ==========\r\n\r\n");
        return;
    }

    printf("[SHT30] 初始化验证成功 (T=%d°C, H=%d%%)\r\n", test_temp, test_humi);
    printf("[SHT30] ========== 初始化完成 ==========\r\n\r\n");
}

// ================== SHT30 读取温湿度 ==================
uint8_t SHT30_Read_Data(int8_t *temp, uint8_t *humi)
{
    uint8_t data[6];
    uint32_t timeout = 0;
    uint16_t temp_raw, humi_raw;
    int16_t temp_val;

    // 检查 I2C 总线是否被卡住
    if (I2C_GetFlagStatus(SHT30_I2C, I2C_FLAG_BUSY))
    {
        printf("[SHT30] I2C 总线被卡住，尝试恢复...\r\n");
        // 禁用 I2C，重新初始化
        I2C_Cmd(SHT30_I2C, DISABLE);
        I2C_Delay_ms(10);
        I2C_Cmd(SHT30_I2C, ENABLE);
        I2C_Delay_ms(10);
    }

    // 发送起始条件
    I2C_GenerateSTART(SHT30_I2C, ENABLE);
    timeout = 0;
    while (!I2C_CheckEvent(SHT30_I2C, I2C_EVENT_MASTER_MODE_SELECT) && timeout < 10000)
        timeout++;
    if (timeout >= 10000)
    {
        printf("[SHT30] 发送起始条件超时\r\n");
        return 0;
    }

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
    I2C_Delay_ms(50);  // 等待测量完成（SHT30 高精度约 15ms，给足余量）

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
    // 临时禁用 CRC 校验以诊断问题
    // crc_temp = SHT30_CRC8(&data[0], 2);
    // if (crc_temp != data[2])
    // {
    //     printf("[SHT30] 温度 CRC 校验失败: 期望 0x%02X, 实际 0x%02X (原始数据: 0x%02X 0x%02X)\r\n",
    //            data[2], crc_temp, data[0], data[1]);
    //     return 0;
    // }

    // crc_humi = SHT30_CRC8(&data[3], 2);
    // if (crc_humi != data[5])
    // {
    //     printf("[SHT30] 湿度 CRC 校验失败: 期望 0x%02X, 实际 0x%02X (原始数据: 0x%02X 0x%02X)\r\n",
    //            data[5], crc_humi, data[3], data[4]);
    //     return 0;
    // }

    // ========== 数据解析 ==========
    temp_raw = (data[0] << 8) | data[1];
    humi_raw = (data[3] << 8) | data[4];

    // 温度转换：T = -45 + 175 * (temp_raw / 65535)
    // 改用 int16_t 支持负数，然后转换为 int8_t
    temp_val = (175 * temp_raw) / 65535 - 45;
    *temp = (int8_t)temp_val;

    // 湿度转换：RH = 100 * (humi_raw / 65535)
    *humi = (uint8_t)((100 * humi_raw) / 65535);

    // ========== 数据有效性检查 ==========
    // SHT30 工作范围：-40~125°C，0~100%RH
    // 如果超出范围，说明通信失败或数据损坏
    if (*temp < -40 || *temp > 125)
    {
        printf("[SHT30] 温度超出范围 (%d°C)，数据无效，原始值: 0x%02X 0x%02X\r\n",
               *temp, data[0], data[1]);
        return 0;
    }

    if (*humi > 100)
    {
        printf("[SHT30] 湿度超出范围 (%d%%)，数据无效，原始值: 0x%02X 0x%02X\r\n",
               *humi, data[3], data[4]);
        return 0;
    }

    return 1;
}

