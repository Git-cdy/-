#include "stm32f10x.h"
#include "soil_moisture.h"
#include <stdio.h>

// ================== ADC1 初始化（PA0） ==================
void Soil_Moisture_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    ADC_InitTypeDef ADC_InitStructure;

    printf("\r\n[Soil_Moisture] ========== 初始化开始 ==========\r\n");

    // 启用 GPIOA 和 ADC1 时钟
    RCC_APB2PeriphClockCmd(SOIL_ADC_RCC_GPIO, ENABLE);
    RCC_APB2PeriphClockCmd(SOIL_ADC_RCC_ADC, ENABLE);

    // 配置 PA0 为模拟输入
    GPIO_InitStructure.GPIO_Pin = SOIL_ADC_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;  // 模拟输入
    GPIO_Init(SOIL_ADC_PORT, &GPIO_InitStructure);

    printf("[Soil_Moisture] GPIO 配置完成（PA0 模拟输入）\r\n");

    // 配置 ADC1
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;  // 单通道模式
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;  // 单次转换
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;  // 右对齐
    ADC_InitStructure.ADC_NbrOfChannel = 1;
    ADC_Init(SOIL_ADC, &ADC_InitStructure);

    // 配置 ADC 通道
    ADC_RegularChannelConfig(SOIL_ADC, SOIL_ADC_CHANNEL, 1, ADC_SampleTime_55Cycles5);

    // 启用 ADC
    ADC_Cmd(SOIL_ADC, ENABLE);

    // ADC 校准
    ADC_ResetCalibration(SOIL_ADC);
    while (ADC_GetResetCalibrationStatus(SOIL_ADC));
    ADC_StartCalibration(SOIL_ADC);
    while (ADC_GetCalibrationStatus(SOIL_ADC));

    printf("[Soil_Moisture] ADC1 初始化完成（12 位分辨率，0~4095）\r\n");
    printf("[Soil_Moisture] 标定参数：干燥=%d, 饱和=%d\r\n", SOIL_DRY_ADC_VALUE, SOIL_WET_ADC_VALUE);
    printf("[Soil_Moisture] ========== 初始化完成 ==========\r\n\r\n");
}

// ================== 读取土壤湿度 ==================
uint8_t Soil_Moisture_Read(uint16_t *adc_value, uint8_t *moisture)
{
    int16_t raw_moisture;

    // 启动 ADC 转换
    ADC_SoftwareStartConvCmd(SOIL_ADC, ENABLE);

    // 等待转换完成（超时保护）
    uint32_t timeout = 0;
    while (!ADC_GetFlagStatus(SOIL_ADC, ADC_FLAG_EOC) && timeout < 100000)
        timeout++;

    if (timeout >= 100000)
    {
        printf("[Soil_Moisture] 读取失败：ADC 转换超时\r\n");
        return 0;
    }

    // 读取 ADC 值
    *adc_value = ADC_GetConversionValue(SOIL_ADC);

    // 转换为湿度百分比
    // 公式：moisture = (adc - dry) / (wet - dry) * 100%
    // 整数计算：moisture = (adc - dry) * 100 / (wet - dry)
    if (*adc_value <= SOIL_DRY_ADC_VALUE)
    {
        *moisture = 0;
    }
    else if (*adc_value >= SOIL_WET_ADC_VALUE)
    {
        *moisture = 100;
    }
    else
    {
        raw_moisture = (int16_t)((*adc_value - SOIL_DRY_ADC_VALUE) * 100 / (SOIL_WET_ADC_VALUE - SOIL_DRY_ADC_VALUE));
        *moisture = (uint8_t)raw_moisture;
    }

    return 1;
}
