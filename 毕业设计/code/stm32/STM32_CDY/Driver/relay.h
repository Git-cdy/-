#ifndef __RELAY_H
#define __RELAY_H

#include "stm32f10x.h"

// ================== 继电器引脚配置 ==================
// 使用 PA4/PA5/PA6 作为继电器控制引脚
#define RELAY_PORT               GPIOA
#define RELAY_RCC_GPIO           RCC_APB2Periph_GPIOA

// 继电器 1：风机（PA4）
#define RELAY1_PIN               GPIO_Pin_4
#define RELAY1_PORT              GPIOA

// 继电器 2：水阀（PA5）
#define RELAY2_PIN               GPIO_Pin_5
#define RELAY2_PORT              GPIOA

// 继电器 3：补光灯（PA6）
#define RELAY3_PIN               GPIO_Pin_6
#define RELAY3_PORT              GPIOA

// ================== 继电器控制宏 ==================
// 高电平 ON，低电平 OFF
#define RELAY1_ON()              GPIO_SetBits(RELAY1_PORT, RELAY1_PIN)
#define RELAY1_OFF()             GPIO_ResetBits(RELAY1_PORT, RELAY1_PIN)

#define RELAY2_ON()              GPIO_SetBits(RELAY2_PORT, RELAY2_PIN)
#define RELAY2_OFF()             GPIO_ResetBits(RELAY2_PORT, RELAY2_PIN)

#define RELAY3_ON()              GPIO_SetBits(RELAY3_PORT, RELAY3_PIN)
#define RELAY3_OFF()             GPIO_ResetBits(RELAY3_PORT, RELAY3_PIN)

// ================== 继电器初始化与控制 ==================
void Relay_Init(void);
uint8_t Relay_SetState(uint8_t relay_id, uint8_t state);  // relay_id: 1-3, state: 0=OFF, 1=ON

#endif
