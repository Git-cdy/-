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
    // 增加采样时间到 239.5 周期，确保模拟信号充分采样
    ADC_RegularChannelConfig(SOIL_ADC, SOIL_ADC_CHANNEL, 1, ADC_SampleTime_239Cycles5);

    // 启用 ADC
    ADC_Cmd(SOIL_ADC, ENABLE);

    printf("[Soil_Moisture] ADC1 初始化完成（12 位分辨率，0~4095）\r\n");
    printf("[Soil_Moisture] 标定参数：干燥=%d, 饱和=%d\r\n", SOIL_DRY_ADC_VALUE, SOIL_WET_ADC_VALUE);
    printf("[Soil_Moisture] ========== 初始化完成 ==========\r\n\r\n");
}

// ================== 读取土壤湿度 ==================
uint8_t Soil_Moisture_Read(uint16_t *adc_value, uint8_t *moisture)
{
    int16_t raw_moisture;
    uint32_t adc_sum = 0;
    uint8_t i;

    // 多次采样求平均（减少噪声）
    // 采样 20 次，每次间隔 5ms，确保数据稳定
    for (i = 0; i < 20; i++)
    {
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

        adc_sum += ADC_GetConversionValue(SOIL_ADC);

        // 采样间隔 10ms（增加稳定性）
        if (i < 19)
        {
            for (int j = 0; j < 10 * 123; j++);  // 约 10ms
        }
    }

    // 计算平均值
    *adc_value = adc_sum / 20;

    // 临时打印原始 ADC 值用于标定
    printf("[Soil_Moisture] 原始 ADC 值: %d\r\n", *adc_value);

    // 转换为湿度百分比
    // 公式：moisture = (DRY - adc) / (DRY - WET) * 100%
    // 因为干燥时 ADC 大（3000），泡水时 ADC 小（1000）
    // 整数计算：moisture = (DRY - adc) * 100 / (DRY - WET)
    if (*adc_value >= SOIL_DRY_ADC_VALUE)
    {
        *moisture = 0;
    }
    else if (*adc_value <= SOIL_WET_ADC_VALUE)
    {
        *moisture = 100;
    }
    else
    {
        raw_moisture = (int16_t)((SOIL_DRY_ADC_VALUE - *adc_value) * 100 / (SOIL_DRY_ADC_VALUE - SOIL_WET_ADC_VALUE));
        *moisture = (uint8_t)raw_moisture;
    }

    return 1;
}
