#ifndef GPIO_H
#define GPIO_H

#include "stm32f10x.h"

void GPIO_Init_Custom(GPIO_TypeDef *GPIOx, uint16_t pin, GPIOMode_TypeDef mode, GPIOSpeed_TypeDef speed);

#endif
