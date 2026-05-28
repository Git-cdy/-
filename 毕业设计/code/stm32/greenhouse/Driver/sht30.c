#include "sht30.h"
#include "pin_config.h"
#include "delay.h"

static uint8_t SHT30_CRC8(const uint8_t *data, uint8_t len)
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
                crc <<= 1;
        }
    }
    return crc;
}

static void I2C1_Init_HW(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    I2C_InitTypeDef I2C_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

    GPIO_InitStructure.GPIO_Pin = I2C1_SCL_PIN | I2C1_SDA_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(I2C1_SCL_PORT, &GPIO_InitStructure);

    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = 0x00;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStructure.I2C_ClockSpeed = 100000;
    I2C_Init(I2C1, &I2C_InitStructure);
    I2C_Cmd(I2C1, ENABLE);
}

void SHT30_Init(void)
{
    I2C1_Init_HW();
}

uint8_t SHT30_Read(float *temperature, float *humidity)
{
    uint8_t buf[6];
    uint16_t raw_temp, raw_humi;

    /* 发送测量命令：高重复性，时钟拉伸使能 */
    I2C_GenerateSTART(I2C1, ENABLE);
    if (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT)) goto error;
    I2C_Send7bitAddress(I2C1, SHT30_ADDR << 1, I2C_Direction_Transmitter);
    if (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) goto error;
    I2C_SendData(I2C1, 0x2C);
    if (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) goto error;
    I2C_SendData(I2C1, 0x06);
    if (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) goto error;
    I2C_GenerateSTOP(I2C1, ENABLE);

    Delay_ms(20);

    /* 读取6字节：Temp_MSB, Temp_LSB, Temp_CRC, Humi_MSB, Humi_LSB, Humi_CRC */
    I2C_GenerateSTART(I2C1, ENABLE);
    if (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT)) goto error;
    I2C_Send7bitAddress(I2C1, SHT30_ADDR << 1, I2C_Direction_Receiver);
    if (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)) goto error;

    I2C_AcknowledgeConfig(I2C1, ENABLE);
    if (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED)) goto error;
    buf[0] = I2C_ReceiveData(I2C1);

    if (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED)) goto error;
    buf[1] = I2C_ReceiveData(I2C1);

    if (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED)) goto error;
    buf[2] = I2C_ReceiveData(I2C1);

    if (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED)) goto error;
    buf[3] = I2C_ReceiveData(I2C1);

    if (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED)) goto error;
    buf[4] = I2C_ReceiveData(I2C1);

    I2C_AcknowledgeConfig(I2C1, DISABLE);
    if (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED)) goto error;
    buf[5] = I2C_ReceiveData(I2C1);
    I2C_GenerateSTOP(I2C1, ENABLE);

    if (SHT30_CRC8(buf, 2) != buf[2]) goto error;
    if (SHT30_CRC8(buf + 3, 2) != buf[5]) goto error;

    raw_temp = ((uint16_t)buf[0] << 8) | buf[1];
    raw_humi = ((uint16_t)buf[3] << 8) | buf[4];

    *temperature = -45.0f + 175.0f * (float)raw_temp / 65535.0f;
    *humidity = 100.0f * (float)raw_humi / 65535.0f;

    return 1;

error:
    I2C_GenerateSTOP(I2C1, ENABLE);
    return 0;
}
