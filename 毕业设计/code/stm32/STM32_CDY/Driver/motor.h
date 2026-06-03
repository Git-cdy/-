#ifndef __MOTOR_H
#define __MOTOR_H

#include "stm32f10x.h"

// ================== 电机 PWM 控制接口 ==================
void Motor_Init(void);
void Motor_SetSpeed(uint8_t speed);

#endif
