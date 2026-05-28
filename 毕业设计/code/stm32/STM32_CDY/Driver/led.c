#include "led.h"

// ================== LED 初始化 ==================
void LED_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 使能所有用到的GPIO时钟（目前都在GPIOA，后续跨端口时在这里扩展）
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    // LED1 初始化
    GPIO_InitStructure.GPIO_Pin = LED1_GPIO_PIN;
    GPIO_Init(LED1_GPIO_PORT, &GPIO_InitStructure);

    // LED2 初始化
    GPIO_InitStructure.GPIO_Pin = LED2_GPIO_PIN;
    GPIO_Init(LED2_GPIO_PORT, &GPIO_InitStructure);

    // LED3 初始化
    GPIO_InitStructure.GPIO_Pin = LED3_GPIO_PIN;
    GPIO_Init(LED3_GPIO_PORT, &GPIO_InitStructure);

    LED_AllOff();   // 上电默认全灭
}

// ================== 单个LED控制函数 ==================
void LED1_On(void)     { GPIO_SetBits(LED1_GPIO_PORT, LED1_GPIO_PIN); }   // 高电平点亮
void LED1_Off(void)    { GPIO_ResetBits(LED1_GPIO_PORT, LED1_GPIO_PIN); }
void LED1_Toggle(void) { 
    if (GPIO_ReadOutputDataBit(LED1_GPIO_PORT, LED1_GPIO_PIN) == Bit_RESET)
        LED1_On();
    else
        LED1_Off();
}

void LED2_On(void)     { GPIO_SetBits(LED2_GPIO_PORT, LED2_GPIO_PIN); }
void LED2_Off(void)    { GPIO_ResetBits(LED2_GPIO_PORT, LED2_GPIO_PIN); }
void LED2_Toggle(void) { 
    if (GPIO_ReadOutputDataBit(LED2_GPIO_PORT, LED2_GPIO_PIN) == Bit_RESET)
        LED2_On();
    else
        LED2_Off();
}

void LED3_On(void)     { GPIO_SetBits(LED3_GPIO_PORT, LED3_GPIO_PIN); }
void LED3_Off(void)    { GPIO_ResetBits(LED3_GPIO_PORT, LED3_GPIO_PIN); }
void LED3_Toggle(void) { 
    if (GPIO_ReadOutputDataBit(LED3_GPIO_PORT, LED3_GPIO_PIN) == Bit_RESET)
        LED3_On();
    else
        LED3_Off();
}

// ================== 全部LED控制函数 ==================
void LED_AllOn(void)
{
    LED1_On();
    LED2_On();
    LED3_On();
}

void LED_AllOff(void)
{
    LED1_Off();
    LED2_Off();
    LED3_Off();
}

void LED_AllToggle(void)
{
    LED1_Toggle();
    LED2_Toggle();
    LED3_Toggle();
}
