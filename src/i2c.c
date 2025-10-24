#include "i2c.h"

void I2C_Init_Custom(I2C_TypeDef *I2Cx) {
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
    
    I2C_InitTypeDef I2C_InitStructure;
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = 0x00;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_ClockSpeed = 100000;
    I2C_Init(I2Cx, &I2C_InitStructure);
    
    I2C_Cmd(I2Cx, ENABLE);
}

void I2C_StartTransmission(I2C_TypeDef *I2Cx, uint8_t address, uint8_t direction) {
    // Your code here for I2C start and address setup
}

void I2C_WriteData(I2C_TypeDef *I2Cx, uint8_t data) {
    // Your code here for I2C write
}
