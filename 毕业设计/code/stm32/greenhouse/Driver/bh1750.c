#include "bh1750.h"
#include "pin_config.h"
#include "delay.h"

void BH1750_Init(void)
{
    /* I2C1 已在 SHT30_Init 中初始化，此处发送上电命令 */
    I2C_GenerateSTART(I2C1, ENABLE);
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));
    I2C_Send7bitAddress(I2C1, BH1750_ADDR << 1, I2C_Direction_Transmitter);
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
    I2C_SendData(I2C1, 0x01);
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
    I2C_GenerateSTOP(I2C1, ENABLE);
}

float BH1750_Read(void)
{
    uint8_t buf[2];
    uint16_t raw;
    uint32_t timeout;

    /* 连续高分辨率模式 */
    I2C_GenerateSTART(I2C1, ENABLE);
    timeout = 100000;
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT)) {
        if (--timeout == 0) goto error;
    }
    I2C_Send7bitAddress(I2C1, BH1750_ADDR << 1, I2C_Direction_Transmitter);
    timeout = 100000;
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) {
        if (--timeout == 0) goto error;
    }
    I2C_SendData(I2C1, 0x10);
    timeout = 100000;
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
        if (--timeout == 0) goto error;
    }
    I2C_GenerateSTOP(I2C1, ENABLE);

    Delay_ms(180);

    /* 读取2字节 */
    I2C_GenerateSTART(I2C1, ENABLE);
    timeout = 100000;
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT)) {
        if (--timeout == 0) goto error;
    }
    I2C_Send7bitAddress(I2C1, BH1750_ADDR << 1, I2C_Direction_Receiver);
    timeout = 100000;
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)) {
        if (--timeout == 0) goto error;
    }

    I2C_AcknowledgeConfig(I2C1, ENABLE);
    timeout = 100000;
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED)) {
        if (--timeout == 0) goto error;
    }
    buf[0] = I2C_ReceiveData(I2C1);

    I2C_AcknowledgeConfig(I2C1, DISABLE);
    timeout = 100000;
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED)) {
        if (--timeout == 0) goto error;
    }
    buf[1] = I2C_ReceiveData(I2C1);
    I2C_GenerateSTOP(I2C1, ENABLE);

    raw = ((uint16_t)buf[0] << 8) | buf[1];
    return (float)raw / 1.2f;

error:
    I2C_GenerateSTOP(I2C1, ENABLE);
    return -1.0f;
}
