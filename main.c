#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_usart.h"
#include <stdio.h>
#include <string.h>

#define LED_PIN GPIO_Pin_13
#define LED_PORT GPIOC
#define DHT11_PORT GPIOA
#define DHT11_PIN GPIO_Pin_1

SemaphoreHandle_t xUARTSemaphore;
SemaphoreHandle_t xLEDSemaphore;
QueueHandle_t xQueueTemp;
QueueHandle_t xQueueLEDTemp;

// --- GPIO Init ---
void GPIO_InitConfig() {
    GPIO_InitTypeDef GPIO_InitStruct;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    GPIO_InitStruct.GPIO_Pin = LED_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(LED_PORT, &GPIO_InitStruct);
    GPIO_ResetBits(LED_PORT, LED_PIN);
}

// --- UART Init ---
void UART_InitConfig() {
    GPIO_InitTypeDef GPIO_InitStruct;
    USART_InitTypeDef USART_InitStruct;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);

    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_9; // TX
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_10; // RX
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    USART_InitStruct.USART_BaudRate = 9600;
    USART_InitStruct.USART_WordLength = USART_WordLength_8b;
    USART_InitStruct.USART_StopBits = USART_StopBits_1;
    USART_InitStruct.USART_Parity = USART_Parity_No;
    USART_InitStruct.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_Init(USART1, &USART_InitStruct);
    USART_Cmd(USART1, ENABLE);
}

// --- Delay us (simple blocking) ---
void Delay_us(uint32_t us) {
    us *= 8; // Roughly calibrated for 72 MHz
    while (us--) {
        __asm("nop");
    }
}

// --- DHT11 Pin Control ---
void DHT11_SetOutput(void) {
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_Pin = DHT11_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DHT11_PORT, &GPIO_InitStruct);
}

void DHT11_SetInput(void) {
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_Pin = DHT11_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(DHT11_PORT, &GPIO_InitStruct);
}

// --- DHT11 Read ---
uint8_t DHT11_ReadByte(void) {
    uint8_t i, result = 0;
    for (i = 0; i < 8; i++) {
        while (!GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN));
        Delay_us(40);
        if (GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN)) result |= (1 << (7 - i));
        while (GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN));
    }
    return result;
}

uint8_t DHT11_ReadTemperature(void) {
    uint8_t data[5] = {0};
    uint32_t timeout = 0;

    DHT11_SetOutput();
    GPIO_ResetBits(DHT11_PORT, DHT11_PIN);
    vTaskDelay(pdMS_TO_TICKS(20));  // >18ms
    GPIO_SetBits(DHT11_PORT, DHT11_PIN);
    Delay_us(30);
    DHT11_SetInput();

    timeout = 10000;
    while (GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN)) {
        if (--timeout == 0) return 0;
    }
    timeout = 10000;
    while (!GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN)) {
        if (--timeout == 0) return 0;
    }
    timeout = 10000;
    while (GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN)) {
        if (--timeout == 0) return 0;
    }

    for (int i = 0; i < 5; i++) data[i] = DHT11_ReadByte();
    if ((data[0] + data[1] + data[2] + data[3]) == data[4])
        return data[2];  // Temperature
    return 0;
}

// --- UART Send ---
void UART_SendString(const char *str) {
    xSemaphoreTake(xUARTSemaphore, portMAX_DELAY);
    while (*str) {
        USART_SendData(USART1, *str++);
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    }
    xSemaphoreGive(xUARTSemaphore);
}

// --- Task Read DHT11 ---
void Task_ReadDHT11(void *pvParameters) {
    uint8_t temp;
    while (1) {
        temp = DHT11_ReadTemperature();
        xQueueSend(xQueueTemp, &temp, portMAX_DELAY);
        xQueueSend(xQueueLEDTemp, &temp, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

// --- Task UART Handler ---
void Task_UARTHandler(void *pvParameters) {
    uint8_t temp;
    char buffer[64];
    while (1) {
        if (xQueueReceive(xQueueTemp, &temp, portMAX_DELAY)) {
            snprintf(buffer, sizeof(buffer), "Nhiet do: %d*C\r\n", temp);
            UART_SendString(buffer);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// --- Task LED ---
void Task_LEDControl(void *pvParameters) {
    uint8_t temp;
    while (1) {
        if (xQueueReceive(xQueueLEDTemp, &temp, portMAX_DELAY)) {
            xSemaphoreTake(xLEDSemaphore, portMAX_DELAY);
            if (temp > 35) {
                GPIO_ResetBits(LED_PORT, LED_PIN);  // Sáng LED
            } else {
                GPIO_SetBits(LED_PORT, LED_PIN);  // T?t LED
            }
            xSemaphoreGive(xLEDSemaphore);
        }
    }
}

// --- Main ---
int main(void) {
    xUARTSemaphore = xSemaphoreCreateMutex();
    xLEDSemaphore = xSemaphoreCreateMutex();
    xQueueTemp = xQueueCreate(5, sizeof(uint8_t));
    xQueueLEDTemp = xQueueCreate(5, sizeof(uint8_t));

    GPIO_InitConfig();
    UART_InitConfig();

    xTaskCreate(Task_ReadDHT11, "ReadDHT11", 128, NULL, 1, NULL);
    xTaskCreate(Task_UARTHandler, "UARTHandler", 128, NULL, 1, NULL);
    xTaskCreate(Task_LEDControl, "LEDControl", 128, NULL, 1, NULL);

    vTaskStartScheduler();
    while (1);
}
