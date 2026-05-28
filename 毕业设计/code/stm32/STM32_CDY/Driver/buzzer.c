#include "buzzer.h"

// ================== 蜂鸣器初始化 ==================
void Buzzer_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 开启 GPIOB 时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    
    // 配置 PB12 为推挽输出
    GPIO_InitStructure.GPIO_Pin = BUZZER_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(BUZZER_GPIO_PORT, &GPIO_InitStructure);

    // 关键修正：上电默认输出高电平（关闭低电平触发的蜂鸣器）
    GPIO_SetBits(BUZZER_GPIO_PORT, BUZZER_GPIO_PIN);
}

// ================== 蜂鸣器控制逻辑 (低电平触发) ==================

// 开启报警（输出低电平，让蜂鸣器响）
void Buzzer_On(void)
{
    GPIO_ResetBits(BUZZER_GPIO_PORT, BUZZER_GPIO_PIN);
}

// 关闭报警（输出高电平，让蜂鸣器停）
void Buzzer_Off(void)
{
    GPIO_SetBits(BUZZER_GPIO_PORT, BUZZER_GPIO_PIN);
}
