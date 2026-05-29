// ================== ADC 诊断程序 ==================
// 用于快速定位 ADC 读取问题

#include "stm32f10x.h"
#include "uart.h"
#include "soil_moisture.h"
#include <stdio.h>

int main(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    UART1_Init(115200);

    printf("\r\n========== ADC 诊断程序 ==========\r\n");
    printf("此程序用于快速定位 ADC 读取问题\r\n\r\n");

    // 初始化土壤湿度传感器
    Soil_Moisture_Init();

    printf("\r\n[诊断] 开始读取 ADC 值...\r\n");
    printf("[诊断] 请在干燥和泡水之间切换传感器，观察 ADC 值变化\r\n\r\n");

    // 连续读取 ADC 值
    for (int attempt = 0; attempt < 100; attempt++)
    {
        uint16_t adc_value;
        uint8_t moisture;

        if (Soil_Moisture_Read(&adc_value, &moisture))
        {
            printf("[诊断] #%3d: ADC=%4d, 湿度=%3d%%\r\n", attempt + 1, adc_value, moisture);
        }
        else
        {
            printf("[诊断] #%3d: 读取失败\r\n", attempt + 1);
        }

        // 延迟 500ms
        for (int i = 0; i < 500; i++)
        {
            for (int j = 0; j < 123; j++);
        }
    }

    printf("\r\n========== 诊断完成 ==========\r\n");
    printf("观察 ADC 值是否有变化：\r\n");
    printf("  - 如果都是 2000 左右，说明 ADC 没有真正读取 PA0\r\n");
    printf("  - 如果有变化，说明 ADC 工作正常，问题在标定参数\r\n");
    printf("  - 如果都是 0 或 4095，说明 ADC 配置有问题\r\n\r\n");

    while (1);
    return 0;
}
