#ifndef __BUZZER_H
#define __BUZZER_H

#include "stm32f10x.h"

// ================== 蜂鸣器引脚宏定义 ==================
#define BUZZER_GPIO_PORT    GPIOB
#define BUZZER_GPIO_PIN     GPIO_Pin_11

// ================== 蜂鸣器控制接口 ==================
void Buzzer_Init(void);
void Buzzer_On(void);
void Buzzer_Off(void);

#endif
