#ifndef UART_H
#define UART_H

#include "stm32f10x.h"

void USART_Init_Custom(uint32_t baudrate);
void USART_SendChar(char c);
void USART_SendString(char *str);

#endif
