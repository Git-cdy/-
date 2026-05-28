#include "soil.h"
#include "pin_config.h"

/* 两点标定值（需实际测量后修改） */
#define SOIL_DRY_ADC    4095    /* 空气中干燥时的ADC值（约0.5V → ~620） */
#define SOIL_WET_ADC    2048    /* 水中完全湿润时的ADC值（约2.5V → ~3100） */

/* 实际值需要标定：先用 Soil_Read(10) 分别在空气和水中测量，替换上面两个值 */

void Soil_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    ADC_InitTypeDef ADC_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_ADC1, ENABLE);
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);

    GPIO_InitStructure.GPIO_Pin = ADC_SOIL_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(ADC_SOIL_PORT, &GPIO_InitStructure);

    ADC_DeInit(ADC1);
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = 1;
    ADC_Init(ADC1, &ADC_InitStructure);

    ADC_Cmd(ADC1, ENABLE);

    ADC_ResetCalibration(ADC1);
    while (ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);
    while (ADC_GetCalibrationStatus(ADC1));
}

static uint16_t ADC_Read(void)
{
    ADC_RegularChannelConfig(ADC1, ADC_SOIL_CHANNEL, 1, ADC_SampleTime_239Cycles5);
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
    return ADC_GetConversionValue(ADC1);
}

/* 读取土壤湿度百分比（中值+均值滤波） */
float Soil_Read(uint8_t samples)
{
    uint16_t i, j, temp;
    uint16_t buf[11];
    uint32_t sum;

    if (samples > 10) samples = 10;
    if (samples < 3)  samples = 3;

    for (i = 0; i < samples; i++)
    {
        buf[i] = ADC_Read();
    }

    /* 冒泡排序 */
    for (i = 0; i < samples - 1; i++)
    {
        for (j = i + 1; j < samples; j++)
        {
            if (buf[i] > buf[j])
            {
                temp = buf[i];
                buf[i] = buf[j];
                buf[j] = temp;
            }
        }
    }

    /* 去掉最大最小值，取平均 */
    sum = 0;
    for (i = 1; i < samples - 1; i++)
    {
        sum += buf[i];
    }
    temp = sum / (samples - 2);

    /* 线性插值：土壤湿度 = (dry_adc - adc) / (dry_adc - wet_adc) * 100 */
    if (temp >= SOIL_DRY_ADC) return 0.0f;
    if (temp <= SOIL_WET_ADC) return 100.0f;
    return (float)(SOIL_DRY_ADC - temp) / (float)(SOIL_DRY_ADC - SOIL_WET_ADC) * 100.0f;
}
