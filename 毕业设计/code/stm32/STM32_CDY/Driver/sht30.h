#ifndef __SHT30_H
#define __SHT30_H

#include "stm32f10x.h"

// ================== SHT30 I2C 引脚配置 ==================
// 使用 I2C1（PB6/PB7）硬件 I2C
#define SHT30_I2C              I2C1
#define SHT30_I2C_PORT         GPIOB
#define SHT30_I2C_SCL_PIN      GPIO_Pin_6
#define SHT30_I2C_SDA_PIN      GPIO_Pin_7
#define SHT30_I2C_RCC_GPIO     RCC_APB2Periph_GPIOB
#define SHT30_I2C_RCC_I2C      RCC_APB1Periph_I2C1
#define SHT30_I2C_SPEED        100000  // 100kHz

// ================== SHT30 I2C 地址 ==================
#define SHT30_I2C_ADDR         0x44    // 7 位地址

// ================== SHT30 命令定义 ==================
#define SHT30_CMD_SOFT_RESET   0x30A2  // 软复位
#define SHT30_CMD_MEAS_SINGLE  0x2400  // 单次测量（高精度）

// ================== SHT30 初始化与数据读取 ==================
void SHT30_Init(void);
uint8_t SHT30_Read_Data(uint8_t *temp, uint8_t *humi);

#endif

