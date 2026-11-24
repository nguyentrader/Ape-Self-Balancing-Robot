
#include "uart_stm32.h"

#include <stdio.h>
#include <string.h>

#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "vehicle_state.h"

#define STM32_UART          UART_NUM_2
#define STM32_TX_PIN        25
#define STM32_RX_PIN        26
#define STM32_BAUDRATE      115200

#define UART_RX_BUFFER_SIZE 1024
#define UART_LINE_SIZE      64
#define UART_TASK_STACK     3072
#define UART_TASK_PRIORITY  10

static SemaphoreHandle_t uart_tx_mutex = NULL;

static void uart_process_line(const char *line)
{
    char speed[VEHICLE_TELEMETRY_SIZE];
    char angle[VEHICLE_TELEMETRY_SIZE];

    if (sscanf(line, "D:%15[^,],%15s", speed, angle) == 2) {
        vehicle_state_set_telemetry(speed, angle);
    }
}

static void uart_receive_task(void *arg)
{
    uint8_t rx_data[128];

    char line[UART_LINE_SIZE];
    size_t line_length = 0;

    while (1) {
        int length = uart_read_bytes(
            STM32_UART,
            rx_data,
            sizeof(rx_data),
            pdMS_TO_TICKS(100)
        );

        if (length <= 0) {
            continue;
        }

        for (int i = 0; i < length; i++) {
            char ch = (char)rx_data[i];

            if (ch == '\n') {
                line[line_length] = '\0';

                if (line_length > 0) {
                    uart_process_line(line);
                }

                line_length = 0;
            }
            else if (ch != '\r') {
                if (line_length < sizeof(line) - 1) {
                    line[line_length++] = ch;
                }
                else {
                    line_length = 0;
                }
            }
        }
    }
}

esp_err_t uart_stm32_init(void)
{
    if (uart_tx_mutex == NULL) {
        uart_tx_mutex = xSemaphoreCreateMutex();

        if (uart_tx_mutex == NULL) {
            return ESP_ERR_NO_MEM;
        }
    }

    const uart_config_t uart_config = {
        .baud_rate = STM32_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT
    };

    esp_err_t ret = uart_param_config(STM32_UART, &uart_config);

    if (ret != ESP_OK) {
        return ret;
    }

    ret = uart_set_pin(
        STM32_UART,
        STM32_TX_PIN,
        STM32_RX_PIN,
        UART_PIN_NO_CHANGE,
        UART_PIN_NO_CHANGE
    );

    if (ret != ESP_OK) {
        return ret;
    }

    ret = uart_driver_install(
        STM32_UART,
        UART_RX_BUFFER_SIZE,
        0,
        0,
        NULL,
        0
    );

    if (ret != ESP_OK) {
        return ret;
    }

    BaseType_t task_result = xTaskCreate(
        uart_receive_task,
        "uart_rx_task",
        UART_TASK_STACK,
        NULL,
        UART_TASK_PRIORITY,
        NULL
    );

    if (task_result != pdPASS) {
        uart_driver_delete(STM32_UART);
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

void uart_stm32_send_command(const char *command)
{
    if (command == NULL || uart_tx_mutex == NULL) {
        return;
    }

    const char *wire_command = NULL;
    const char *direction = NULL;

    if (strcmp(command, "F") == 0) {
        wire_command = "F\n";
        direction = "F";
    }
    else if (strcmp(command, "B") == 0) {
        wire_command = "B\n";
        direction = "B";
    }
    else if (strcmp(command, "L") == 0) {
        wire_command = "S\nL\n";
        direction = "L";
    }
    else if (strcmp(command, "R") == 0) {
        wire_command = "S\nR\n";
        direction = "R";
    }
    else if (strcmp(command, "FL") == 0) {
        wire_command = "F\nL\n";
        direction = "FL";
    }
    else if (strcmp(command, "FR") == 0) {
        wire_command = "F\nR\n";
        direction = "FR";
    }
    else if (strcmp(command, "BL") == 0) {
        wire_command = "B\nL\n";
        direction = "BL";
    }
    else if (strcmp(command, "BR") == 0) {
        wire_command = "B\nR\n";
        direction = "BR";
    }
    else if (strcmp(command, "S") == 0) {
        wire_command = "S\n";
        direction = "S";
    }
    else {
        return;
    }

    xSemaphoreTake(uart_tx_mutex, portMAX_DELAY);

    int written = uart_write_bytes(
        STM32_UART,
        wire_command,
        strlen(wire_command)
    );

    if (written == (int)strlen(wire_command)) {
        uart_wait_tx_done(STM32_UART, pdMS_TO_TICKS(100));
        vehicle_state_set_direction(direction);
    }

    xSemaphoreGive(uart_tx_mutex);
}
