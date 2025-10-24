#ifndef I2C_H
#define I2C_H

#include "stm32f10x.h"

void I2C_Init_Custom(I2C_TypeDef *I2Cx);
void I2C_StartTransmission(I2C_TypeDef *I2Cx, uint8_t address, uint8_t direction);
void I2C_WriteData(I2C_TypeDef *I2Cx, uint8_t data);

#endif
