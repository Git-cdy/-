#ifndef __SOIL_H
#define __SOIL_H

#include "stm32f10x.h"

void Soil_Init(void);
float Soil_Read(uint8_t samples);

#endif /* __SOIL_H */
