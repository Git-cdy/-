#ifndef __DHT11_H
#define __DHT11_H

#include "stm32f10x.h"

// ================== DHT11 引脚配置 ==================
// 使用 PA4 作为 DHT11 数据线（单总线）
#define DHT11_GPIO_PORT    GPIOA
#define DHT11_GPIO_PIN     GPIO_Pin_4

// ================== DHT11 初始化与数据读取 ==================
void DHT11_Init(void);
uint8_t DHT11_Read_Data(uint8_t *temp, uint8_t *humi);

#endif
