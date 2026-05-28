#ifndef __ADC_H
#define __ADC_H

#include "stm32f10x.h"

// ================== ADC 引脚配置 ==================
// 我们使用 PA1 作为 ADC 模拟输入端
#define ADC1_GPIO_PORT    GPIOA
#define ADC1_GPIO_PIN     GPIO_Pin_1

// ================== 函数声明 ==================
void ADC1_Init(void);                        // ADC初始化
uint16_t ADC1_Get_Value(void);               // 单次获取ADC原始值 (0~4095)
uint16_t ADC1_Get_Average(uint8_t times);    // 获取多次采样的平均值（滤波）

#endif /* __ADC_H */
