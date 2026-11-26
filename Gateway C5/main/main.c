#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"

#include "vehicle_state.h"
#include "uart_stm32.h"
#include "led_control.h"
#include "wifi_ap.h"
#include "web_server.h"
#include "websocket.h"

static const char *TAG = "MAIN";

// Initialize the NVS storage.
static void nvs_init(void)
{
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {

        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);
}

// Initialize the ESP network interface.
static void network_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
}

// Main application entry point.
void app_main(void)
{
    ESP_LOGI(TAG, "Starting Gateway");

    // Initialize system storage.
    nvs_init();

    // Initialize network services.
    network_init();

    // Initialize vehicle state.
    ESP_ERROR_CHECK(vehicle_state_init());

    // Initialize STM32 UART communication.
    ESP_ERROR_CHECK(uart_stm32_init());

    // Initialize WS2812B LEDs.
    ESP_ERROR_CHECK(led_control_init());

    // Initialize Wi-Fi SoftAP.
    ESP_ERROR_CHECK(wifi_ap_init());

    // Start the HTTP server.
    ESP_ERROR_CHECK(web_server_init());

    // Start the WebSocket server.
    ESP_ERROR_CHECK(websocket_server_init());

    ESP_LOGI(TAG, "Gateway initialization completed");
}
