#ifndef __RELAY_H
#define __RELAY_H

#include "stm32f10x.h"
#include "pin_config.h"

#define RELAY_ON    0
#define RELAY_OFF   1

void Relay_Init(void);
void Relay_Fan(uint8_t state);
void Relay_Water(uint8_t state);
void Relay_Light(uint8_t state);

#endif /* __RELAY_H */
