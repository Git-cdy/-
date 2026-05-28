#include "stm32f10x.h"

// ================== 电机 PWM 配置 ==================
// PA11 (TIM1_CH4) 产生 PWM 波控制风扇电机
// PWM 频率：1kHz，占空比：0~100%

// ================== 电机初始化 ==================
void Motor_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;

    // -------- 启用时钟 --------
    // 启用 GPIOA 时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    // 启用 TIM1 时钟（高级定时器在 APB2 上）
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);

    // 启用 AFIO 时钟（复用功能必须）
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

    // -------- 配置 GPIO --------
    // 配置 PA11 为 TIM1_CH4 复用推挽输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // -------- 配置 TIM1 基础参数 --------
    // 72MHz / 72 = 1MHz，1MHz / 1000 = 1kHz
    TIM_TimeBaseInitStructure.TIM_Period = 1000 - 1;           // 自动重装载值（ARR）
    TIM_TimeBaseInitStructure.TIM_Prescaler = 72 - 1;          // 预分频系数（PSC）
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;       // 高级定时器必须设置
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseInitStructure);

    // -------- 配置 TIM1_CH4 PWM 输出 --------
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Disable;  // 不使用互补输出
    TIM_OCInitStructure.TIM_Pulse = 0;                         // 初始占空比 0%
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_High;
    TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;
    TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCNIdleState_Reset;
    TIM_OC4Init(TIM1, &TIM_OCInitStructure);

    // -------- 预装载配置 --------
    // 启用输出比较预装载
    TIM_OC4PreloadConfig(TIM1, TIM_OCPreload_Enable);

    // 启用自动重装载寄存器预装载
    TIM_ARRPreloadConfig(TIM1, ENABLE);

    // -------- 启动定时器 --------
    // 启用 TIM1 主输出（高级定时器必须）
    TIM_CtrlPWMOutputs(TIM1, ENABLE);

    // 启动 TIM1 计数器
    TIM_Cmd(TIM1, ENABLE);
}

// ================== 电机速度控制 ==================
// speed：0~100，表示占空比百分比
void Motor_SetSpeed(uint8_t speed)
{
    uint16_t pulse;

    // 限制 speed 在 0~100 范围内
    if (speed > 100) speed = 100;

    // 计算 PWM 脉冲宽度
    // 自动重装载值为 1000，占空比 = pulse / 1000
    // 例如：speed = 50 时，pulse = 500，占空比 = 50%
    pulse = (uint16_t)(1000 * speed / 100);

    // 更新 TIM1_CH4 的比较值（CCR4）
    TIM_SetCompare4(TIM1, pulse);
}
