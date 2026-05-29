#ifndef __SOIL_MOISTURE_H
#define __SOIL_MOISTURE_H

#include "stm32f10x.h"

// ================== 土壤湿度传感器 ADC 配置 ==================
// 使用 ADC1 的 IN0 通道（PA0）
#define SOIL_ADC                ADC1
#define SOIL_ADC_PORT            GPIOA
#define SOIL_ADC_PIN             GPIO_Pin_0
#define SOIL_ADC_CHANNEL         ADC_Channel_0
#define SOIL_ADC_RCC_GPIO        RCC_APB2Periph_GPIOA
#define SOIL_ADC_RCC_ADC         RCC_APB2Periph_ADC1

// ================== 土壤湿度转换参数 ==================
// 传感器输出范围：0~3.3V（对应 ADC 值 0~4095）
// 土壤湿度范围：0~100%（需要根据实际传感器标定）
//
// 标定方法：
// 1. 干燥（0%）：测得 ADC 值 = DRY_ADC_VALUE
// 2. 饱和（100%）：测得 ADC 值 = WET_ADC_VALUE
// 3. 湿度 = (ADC - DRY) / (WET - DRY) * 100%
//
// 默认值（需根据实际传感器调整）：
#define SOIL_DRY_ADC_VALUE       800    // 干燥时 ADC 值（0%）
#define SOIL_WET_ADC_VALUE       3200   // 饱和时 ADC 值（100%）

// ================== 土壤湿度初始化与读取 ==================
// moisture: 土壤湿度（0~100%）
void Soil_Moisture_Init(void);
uint8_t Soil_Moisture_Read(uint16_t *adc_value, uint8_t *moisture);

#endif
