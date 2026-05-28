#include "relay.h"

void Relay_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin = RELAY_FAN_PIN | RELAY_WATER_PIN | RELAY_LIGHT_PIN;
    GPIO_Init(RELAY_FAN_PORT, &GPIO_InitStructure);

    /* 初始全部关闭（高电平=OFF，光耦隔离） */
    GPIO_SetBits(RELAY_FAN_PORT, RELAY_FAN_PIN | RELAY_WATER_PIN | RELAY_LIGHT_PIN);
}

void Relay_Fan(uint8_t state)
{
    if (state == RELAY_ON)
        GPIO_ResetBits(RELAY_FAN_PORT, RELAY_FAN_PIN);
    else
        GPIO_SetBits(RELAY_FAN_PORT, RELAY_FAN_PIN);
}

void Relay_Water(uint8_t state)
{
    if (state == RELAY_ON)
        GPIO_ResetBits(RELAY_WATER_PORT, RELAY_WATER_PIN);
    else
        GPIO_SetBits(RELAY_WATER_PORT, RELAY_WATER_PIN);
}

void Relay_Light(uint8_t state)
{
    if (state == RELAY_ON)
        GPIO_ResetBits(RELAY_LIGHT_PORT, RELAY_LIGHT_PIN);
    else
        GPIO_SetBits(RELAY_LIGHT_PORT, RELAY_LIGHT_PIN);
}
