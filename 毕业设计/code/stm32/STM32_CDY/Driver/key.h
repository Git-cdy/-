#ifndef __KEY_H
#define __KEY_H

#include "stm32f10x.h"

// ================== 按键引脚宏定义（极简，只改这里两行） ==================
#define KEY1_GPIO_PORT      GPIOA
#define KEY1_GPIO_PIN       GPIO_Pin_0

// ================== 消抖配置 ==================
#define KEY_DEBOUNCE_TIME   15     // 消抖时间（单位：ms），推荐10~30ms

// ================== 对外暴露的变量 ==================
extern volatile uint8_t Key_Down;           // 按键按下事件标志
extern volatile uint8_t Key_Press_Count;    // 按键累计次数（1~9循环）

// ================== 消抖私有变量（移到这里，让 main.c 里的 KEY_Task 能访问） ==================
extern uint32_t  key_last_time;             // 上次状态变化时间戳
extern uint8_t   key_state;                 // 当前稳定状态（0=释放，1=按下）

// ================== 函数声明 ==================
void KEY_Init(void);                        // 按键初始化

#endif /* __KEY_H */
