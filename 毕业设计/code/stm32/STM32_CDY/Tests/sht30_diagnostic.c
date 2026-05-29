// ================== SHT30 诊断程序 ==================
// 用于快速定位 I2C 通信问题
// 烧录此文件替换 main.c，观察串口输出

#include "stm32f10x.h"
#include "uart.h"
#include "sht30.h"
#include <stdio.h>

int main(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    UART1_Init(115200);

    printf("\r\n========== SHT30 诊断程序 ==========\r\n");
    printf("此程序用于快速定位 I2C 通信问题\r\n");
    printf("观察以下输出，判断故障点：\r\n\r\n");

    // 初始化 SHT30
    SHT30_Init();

    // 延迟后尝试读取
    printf("\r\n[诊断] 等待 500ms 后尝试读取...\r\n");
    for (int i = 0; i < 500; i++)
    {
        for (int j = 0; j < 123; j++);  // 约 1ms
    }

    // 尝试读取 10 次
    for (int attempt = 0; attempt < 10; attempt++)
    {
        int8_t temp;
        uint8_t humi;

        printf("\r\n[诊断] 尝试 %d/10...\r\n", attempt + 1);

        if (SHT30_Read_Data(&temp, &humi))
        {
            printf("[诊断] ✓ 读取成功: T=%d°C, H=%d%%\r\n", temp, humi);
        }
        else
        {
            printf("[诊断] ✗ 读取失败\r\n");
        }

        // 延迟 2 秒
        for (int i = 0; i < 2000; i++)
        {
            for (int j = 0; j < 123; j++);
        }
    }

    printf("\r\n========== 诊断完成 ==========\r\n");
    printf("如果所有 10 次都失败，说明 I2C 通信完全不工作\r\n");
    printf("检查项：\r\n");
    printf("  1. PB6/PB7 是否有 4.7kΩ 上拉电阻\r\n");
    printf("  2. SHT30 VCC/GND 是否接对\r\n");
    printf("  3. 用万用表测 PB6/PB7 是否有 3.3V\r\n");
    printf("  4. I2C1 是否被 BH1750 占用（检查地址冲突）\r\n\r\n");

    while (1);
    return 0;
}
