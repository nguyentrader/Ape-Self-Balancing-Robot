#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "nvs_flash.h"

#include "vehicle_state.h"
#include "uart_stm32.h"
#include "led_control.h"
#include "wifi_ap.h"
#include "web_server.h"
#include "websocket.h"

static const char *TAG = "MAIN";

// Initialize NVS with recovery support.
static void nvs_init(void)
{
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {

        ESP_LOGW(TAG, "NVS partition requires recovery");

        ESP_ERROR_CHECK(nvs_flash_erase());

        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "NVS initialized");
}

// Initialize the network interface and event loop.
static void network_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_LOGI(TAG, "Network interface initialized");
}

// Main application entry point.
void app_main(void)
{
    ESP_LOGI(TAG, "Starting Gateway application");

    // Initialize persistent storage.
    nvs_init();

    // Initialize the network framework.
    network_init();

    // Initialize shared vehicle state.
    ESP_ERROR_CHECK(vehicle_state_init());

    ESP_LOGI(TAG, "Vehicle state initialized");

    // Initialize UART communication with STM32.
    ESP_ERROR_CHECK(uart_stm32_init());

    ESP_LOGI(TAG, "STM32 UART initialized");

    // Initialize the WS2812B LED strip.
    ESP_ERROR_CHECK(led_control_init());

    ESP_LOGI(TAG, "LED controller initialized");

    // Start the Wi-Fi SoftAP.
    ESP_ERROR_CHECK(wifi_ap_init());

    ESP_LOGI(TAG, "Wi-Fi SoftAP initialized");

    // Start the HTTP web server.
    ESP_ERROR_CHECK(web_server_init());

    ESP_LOGI(TAG, "HTTP server initialized");

    // Start the WebSocket server.
    ESP_ERROR_CHECK(websocket_server_init());

    ESP_LOGI(TAG, "WebSocket server initialized");

    ESP_LOGI(TAG, "Gateway initialization completed");
}
