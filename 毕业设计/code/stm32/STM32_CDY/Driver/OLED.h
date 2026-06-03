#ifndef __OLED_H
#define __OLED_H

#include "stm32f10x.h"

// ================== OLED 引脚配置 ==================
// 使用 PB8 (SCL) 和 PB9 (SDA) 进行软件 I2C 通信
#define OLED_W_SCL(x)		GPIO_WriteBit(GPIOB, GPIO_Pin_13, (BitAction)(x))
#define OLED_W_SDA(x)		GPIO_WriteBit(GPIOB, GPIO_Pin_12, (BitAction)(x))

// ================== 核心初始化与清屏 ==================
void OLED_Init(void);
void OLED_Clear(void);

// ================== 字符与字符串显示 ==================
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char);
void OLED_ShowString(uint8_t Line, uint8_t Column, const char *String);

// ================== 数字显示（十进制、十六进制、二进制） ==================
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length);
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);

// ================== 汉字显示 ==================
void OLED_ShowChinese(uint8_t Line, uint8_t Column, const char *Chinese);

#endif
