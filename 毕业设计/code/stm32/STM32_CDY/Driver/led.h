#ifndef __LED_H
#define __LED_H

#include "stm32f10x.h"

// ================== LED 引脚宏定义（只改这里即可调整引脚） ==================
// 高电平点亮（按你的要求）

#define LED1_GPIO_PORT    GPIOA
#define LED1_GPIO_PIN     GPIO_Pin_3

#define LED2_GPIO_PORT    GPIOA
#define LED2_GPIO_PIN     GPIO_Pin_5

#define LED3_GPIO_PORT    GPIOA
#define LED3_GPIO_PIN     GPIO_Pin_7

// 如果后续要加更多LED，只需在这里继续添加宏定义即可

// ================== 函数声明 ==================
void LED_Init(void);

void LED1_On(void);
void LED1_Off(void);
void LED1_Toggle(void);

void LED2_On(void);
void LED2_Off(void);
void LED2_Toggle(void);

void LED3_On(void);
void LED3_Off(void);
void LED3_Toggle(void);

void LED_AllOn(void);      // 新增：全部点亮
void LED_AllOff(void);     // 全部熄灭
void LED_AllToggle(void);  // 全部翻转（可选）

#endif
