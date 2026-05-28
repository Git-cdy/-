#include "adc.h"

// ================== ADC1 初始化 ==================
void ADC1_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    ADC_InitTypeDef  ADC_InitStructure;

    // 1. 开启时钟：GPIOA 和 ADC1
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_ADC1, ENABLE);
    
    // 2. 设置 ADC 时钟分频 (72MHz / 6 = 12MHz) 
    // STM32规定ADC时钟不能超过14MHz
    RCC_ADCCLKConfig(RCC_PCLK2_Div6); 

    // 3. 配置引脚 PA1 为模拟输入模式
    GPIO_InitStructure.GPIO_Pin  = ADC1_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;  // 模拟输入
    GPIO_Init(ADC1_GPIO_PORT, &GPIO_InitStructure);

    // 4. ADC 基础配置
    ADC_DeInit(ADC1);
    ADC_InitStructure.ADC_Mode               = ADC_Mode_Independent;       // 独立模式
    ADC_InitStructure.ADC_ScanConvMode       = DISABLE;                    // 非扫描模式(单通道)
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;                    // 单次转换模式(软件触发)
    ADC_InitStructure.ADC_ExternalTrigConv   = ADC_ExternalTrigConv_None;  // 软件触发，不使用外部触发
    ADC_InitStructure.ADC_DataAlign          = ADC_DataAlign_Right;        // 数据右对齐 (12位，0~4095)
    ADC_InitStructure.ADC_NbrOfChannel       = 1;                          // 顺序进行规则转换的ADC通道数目
    ADC_Init(ADC1, &ADC_InitStructure);

    // 5. 使能 ADC1
    ADC_Cmd(ADC1, ENABLE);

    // 6. 开启 ADC 校准（官方推荐，保证采样精度）
    ADC_ResetCalibration(ADC1);                     // 复位校准寄存器
    while(ADC_GetResetCalibrationStatus(ADC1));     // 等待复位完成
    ADC_StartCalibration(ADC1);                     // 开始校准
    while(ADC_GetCalibrationStatus(ADC1));          // 等待校准完成
}

// ================== 获取 ADC 采样值 ==================
uint16_t ADC1_Get_Value(void)
{
    // 配置转换通道(通道1)、序号(1)、采样时间(239.5个周期，时间越长越稳定)
    ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 1, ADC_SampleTime_239Cycles5);
    
    // 软件触发启动转换
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    
    // 等待转换结束 (EOC标志位变为SET)
    while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
    
    // 返回转换结果
    return ADC_GetConversionValue(ADC1);
}

// ================== 获取多次采样的平均值 (软件滤波) ==================
uint16_t ADC1_Get_Average(uint8_t times)
{
    uint32_t temp_val = 0;
    uint8_t t;
    
    for(t = 0; t < times; t++)
    {
        temp_val += ADC1_Get_Value();
    }
    
    return temp_val / times;
}
