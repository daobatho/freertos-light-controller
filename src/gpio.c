#include "gpio.h"

void GPIO_Init_Custom(GPIO_TypeDef *GPIOx, uint16_t pin, GPIOMode_TypeDef mode, GPIOSpeed_TypeDef speed) {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = pin;
    GPIO_InitStructure.GPIO_Mode = mode;
    GPIO_InitStructure.GPIO_Speed = speed;
    GPIO_Init(GPIOx, &GPIO_InitStructure);
}
