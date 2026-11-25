#ifndef UART_STM32_H
#define UART_STM32_H

#include "esp_err.h"

// Initialize UART communication with STM32.
esp_err_t uart_stm32_init(void);

// Send a vehicle command to STM32.
void uart_stm32_send_command(const char *command);

#endif