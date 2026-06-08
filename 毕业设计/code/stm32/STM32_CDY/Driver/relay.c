#include "stm32f10x.h"
#include "relay.h"
#include <stdio.h>

// ================== 继电器初始化 ==================
void Relay_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    printf("\r\n[Relay] ========== 初始化开始 ==========\r\n");

    // 启用 GPIOA 时钟
    RCC_APB2PeriphClockCmd(RELAY_RCC_GPIO, ENABLE);

    // 配置 PA4/PA5/PA6 为推挽输出
    GPIO_InitStructure.GPIO_Pin = RELAY1_PIN | RELAY2_PIN | RELAY3_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  // 推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(RELAY_PORT, &GPIO_InitStructure);

    // 初始化：所有继电器关闭（输出高电平，模块低电平触发）
    RELAY1_OFF();
    RELAY2_OFF();
    RELAY3_OFF();

    printf("[Relay] GPIO 配置完成（PA4/PA5/PA6 推挽输出）\r\n");
    printf("[Relay] 继电器 1（风机）: PA4\r\n");
    printf("[Relay] 继电器 2（水阀）: PA5\r\n");
    printf("[Relay] 继电器 3（补光灯）: PA6\r\n");
    printf("[Relay] ========== 初始化完成 ==========\r\n\r\n");
}

// ================== 继电器控制 ==================
uint8_t Relay_SetState(uint8_t relay_id, uint8_t state)
{
    const char *relay_name[] = {"", "风机", "水阀", "补光灯"};

    if (relay_id < 1 || relay_id > 3)
    {
        printf("[Relay] 错误：继电器 ID 无效 (%d)\r\n", relay_id);
        return 0;
    }

    if (state)
    {
        switch (relay_id)
        {
            case 1: RELAY1_ON(); break;
            case 2: RELAY2_ON(); break;
            case 3: RELAY3_ON(); break;
        }
        printf("[Relay] %s 已开启\r\n", relay_name[relay_id]);
    }
    else
    {
        switch (relay_id)
        {
            case 1: RELAY1_OFF(); break;
            case 2: RELAY2_OFF(); break;
            case 3: RELAY3_OFF(); break;
        }
        printf("[Relay] %s 已关闭\r\n", relay_name[relay_id]);
    }

    return 1;
}
