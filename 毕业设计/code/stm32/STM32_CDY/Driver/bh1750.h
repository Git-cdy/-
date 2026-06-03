#ifndef __BH1750_H
#define __BH1750_H

#include "stm32f10x.h"

// ================== BH1750 I2C 引脚配置 ==================
// 使用 I2C1（PB6/PB7）硬件 I2C（与 SHT30 共享）
#define BH1750_I2C              I2C1
#define BH1750_I2C_PORT         GPIOB
#define BH1750_I2C_SCL_PIN      GPIO_Pin_6
#define BH1750_I2C_SDA_PIN      GPIO_Pin_7
#define BH1750_I2C_RCC_GPIO     RCC_APB2Periph_GPIOB
#define BH1750_I2C_RCC_I2C      RCC_APB1Periph_I2C1
#define BH1750_I2C_SPEED        100000  // 100kHz

// ================== BH1750 I2C 地址 ==================
// ADDR 引脚接 GND：0x23，接 VCC：0x5C
#define BH1750_I2C_ADDR         0x23    // 7 位地址（ADDR 接 GND）

// ================== BH1750 命令定义 ==================
#define BH1750_CMD_POWER_DOWN   0x00    // 关闭电源
#define BH1750_CMD_POWER_ON     0x01    // 打开电源
#define BH1750_CMD_RESET        0x07    // 重置内部寄存器
#define BH1750_CMD_CONT_H       0x10    // 连续高分辨率模式（1 lux 分辨率，120ms）
#define BH1750_CMD_CONT_H2      0x11    // 连续高分辨率模式 2（0.5 lux 分辨率，120ms）
#define BH1750_CMD_CONT_L       0x13    // 连续低分辨率模式（4 lux 分辨率，16ms）
#define BH1750_CMD_ONCE_H       0x20    // 单次高分辨率模式
#define BH1750_CMD_ONCE_H2      0x21    // 单次高分辨率模式 2
#define BH1750_CMD_ONCE_L       0x23    // 单次低分辨率模式

// ================== BH1750 初始化与数据读取 ==================
// lux: 光照强度（0~65535 lux）
void BH1750_Init(void);
uint8_t BH1750_Read_Lux(uint16_t *lux);

#endif
