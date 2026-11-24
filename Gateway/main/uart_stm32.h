#ifndef UART_STM32_H
#define UART_STM32_H

#include "esp_err.h"

esp_err_t uart_stm32_init(void);

void uart_stm32_send_command(const char *command);

#endif