#ifndef __SHT30_H
#define __SHT30_H

#include "stm32f10x.h"

void SHT30_Init(void);
uint8_t SHT30_Read(float *temperature, float *humidity);

#endif /* __SHT30_H */
