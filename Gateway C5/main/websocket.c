#include "websocket.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_timer.h"
#include "esp_http_server.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "vehicle_state.h"
#include "uart_stm32.h"
#include "led_control.h"

// WebSocket configuration.
#define WS_SERVER_PORT             81
#define WS_SERVER_CTRL_PORT        32769
#define WS_MAX_CLIENTS             2

#define WS_FAILSAFE_TIMEOUT_US     900000
#define WS_TELEMETRY_INTERVAL_MS   250
#define WS_TASK_INTERVAL_MS        50
#define WS_RX_BUFFER_SIZE          64

#define WS_TASK_STACK              4096
#define WS_TASK_PRIORITY           5

static const char *TAG = "WEBSOCKET";

static httpd_handle_t ws_server = NULL;

// Count connected WebSocket clients.
static size_t websocket_get_client_count(void)
{
    if (ws_server == NULL) {
        return 0;
    }

    int client_fds[WS_MAX_CLIENTS + 3];
    size_t client_count = WS_MAX_CLIENTS + 3;

    if (httpd_get_client_list(
            ws_server,
            &client_count,
            client_fds) != ESP_OK) {
        return 0;
    }

    size_t ws_count = 0;

    for (size_t i = 0; i < client_count; i++) {
        if (httpd_ws_get_fd_info(
                ws_server,
                client_fds[i]) == HTTPD_WS_CLIENT_WEBSOCKET) {
            ws_count++;
        }
    }

    return ws_count;
}

// Check supported vehicle commands.
static bool websocket_is_valid_command(const char *command)
{
    static const char *valid_commands[] = {
        "F", "B",
        "L", "R",
        "FL", "FR",
        "BL", "BR",
        "S"
    };

    if (command == NULL) {
        return false;
    }

    for (size_t i = 0;
         i < sizeof(valid_commands) / sizeof(valid_commands[0]);
         i++) {

        if (strcmp(command, valid_commands[i]) == 0) {
            return true;
        }
    }

    return false;
}

// Send a text frame to one client.
static void websocket_send_text(int fd, const char *message)
{
    if (ws_server == NULL || message == NULL) {
        return;
    }

    httpd_ws_frame_t frame = {
        .type = HTTPD_WS_TYPE_TEXT,
        .payload = (uint8_t *)message,
        .len = strlen(message)
    };

    esp_err_t ret = httpd_ws_send_frame_async(
        ws_server,
        fd,
        &frame
    );

    if (ret != ESP_OK) {
        ESP_LOGW(
            TAG,
            "Cannot send WS frame to fd=%d: %s",
            fd,
            esp_err_to_name(ret)
        );
    }
}

// Send initial telemetry after the handshake.
static void websocket_send_initial_work(void *arg)
{
    int fd = (int)(intptr_t)arg;

    if (ws_server == NULL) {
        return;
    }

    if (httpd_ws_get_fd_info(
            ws_server,
            fd) != HTTPD_WS_CLIENT_WEBSOCKET) {
        return;
    }

    char speed[VEHICLE_TELEMETRY_SIZE];
    char angle[VEHICLE_TELEMETRY_SIZE];
    char message[64];

    vehicle_state_get_telemetry(
        speed,
        sizeof(speed),
        angle,
        sizeof(angle)
    );

    snprintf(
        message,
        sizeof(message),
        "T:%s,%s",
        speed,
        angle
    );

    websocket_send_text(fd, message);

    ESP_LOGI(TAG, "Initial telemetry sent to fd=%d", fd);
}

// Broadcast telemetry to all connected clients.
static void websocket_broadcast_work(void *arg)
{
    if (ws_server == NULL) {
        return;
    }

    int client_fds[WS_MAX_CLIENTS + 3];
    size_t client_count = WS_MAX_CLIENTS + 3;

    if (httpd_get_client_list(
            ws_server,
            &client_count,
            client_fds) != ESP_OK) {
        return;
    }

    char speed[VEHICLE_TELEMETRY_SIZE];
    char angle[VEHICLE_TELEMETRY_SIZE];
    char message[64];

    vehicle_state_get_telemetry(
        speed,
        sizeof(speed),
        angle,
        sizeof(angle)
    );

    snprintf(
        message,
        sizeof(message),
        "T:%s,%s",
        speed,
        angle
    );

    for (size_t i = 0; i < client_count; i++) {
        int fd = client_fds[i];

        if (httpd_ws_get_fd_info(
                ws_server,
                fd) == HTTPD_WS_CLIENT_WEBSOCKET) {

            websocket_send_text(fd, message);
        }
    }
}

// Handle client disconnection.
static void websocket_disconnect_work(void *arg)
{
    size_t client_count = websocket_get_client_count();

    ESP_LOGI(
        TAG,
        "Remaining WebSocket clients: %u",
        (unsigned int)client_count
    );

    if (client_count == 0 && vehicle_state_is_moving()) {

        ESP_LOGW(
            TAG,
            "Last client disconnected. Stopping vehicle."
        );

        uart_stm32_send_command("S");
        led_control_set_direction("S");

        vehicle_state_set_last_active(esp_timer_get_time());
    }
}

// Handle HTTP server disconnect events.
static void websocket_event_handler(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data
)
{
    if (event_id != HTTP_SERVER_EVENT_DISCONNECTED) {
        return;
    }

    if (ws_server == NULL) {
        return;
    }

    esp_err_t ret = httpd_queue_work(
        ws_server,
        websocket_disconnect_work,
        NULL
    );

    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Cannot queue disconnect work");
    }
}

// Handle WebSocket requests.
static esp_err_t websocket_handler(httpd_req_t *req)
{
    int fd = httpd_req_to_sockfd(req);

    // Handle the initial handshake.
    if (req->method == HTTP_GET) {

        size_t current_clients = websocket_get_client_count();

        if (current_clients >= WS_MAX_CLIENTS) {
            ESP_LOGW(TAG, "Maximum WebSocket clients reached");
            return ESP_FAIL;
        }

        vehicle_state_set_last_active(esp_timer_get_time());

        ESP_LOGI(
            TAG,
            "WebSocket handshake requested: fd=%d",
            fd
        );

        // Queue initial telemetry after the handshake.
        esp_err_t ret = httpd_queue_work(
            req->handle,
            websocket_send_initial_work,
            (void *)(intptr_t)fd
        );

        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Cannot queue initial telemetry");
            return ESP_FAIL;
        }

        return ESP_OK;
    }

    // Read the incoming frame header.
    httpd_ws_frame_t frame = {0};

    esp_err_t ret = httpd_ws_recv_frame(
        req,
        &frame,
        0
    );

    if (ret != ESP_OK) {
        ESP_LOGE(
            TAG,
            "Cannot get WebSocket frame size: %s",
            esp_err_to_name(ret)
        );
        return ret;
    }

    // Accept text frames only.
    if (frame.type != HTTPD_WS_TYPE_TEXT) {
        return ESP_OK;
    }

    // Check message size.
    if (frame.len >= WS_RX_BUFFER_SIZE) {
        ESP_LOGW(TAG, "WebSocket message too long");
        return ESP_ERR_INVALID_SIZE;
    }

    char message[WS_RX_BUFFER_SIZE];

    frame.payload = (uint8_t *)message;

    ret = httpd_ws_recv_frame(
        req,
        &frame,
        frame.len
    );

    if (ret != ESP_OK) {
        ESP_LOGE(
            TAG,
            "Cannot receive WebSocket payload: %s",
            esp_err_to_name(ret)
        );
        return ret;
    }

    message[frame.len] = '\0';

    // Handle browser keepalive messages.
    if (strcmp(message, "K") == 0) {
        vehicle_state_set_last_active(esp_timer_get_time());
        return ESP_OK;
    }

    // Reject unsupported commands.
    if (!websocket_is_valid_command(message)) {
        ESP_LOGW(TAG, "Invalid command received");
        return ESP_OK;
    }

    vehicle_state_set_last_active(esp_timer_get_time());

    ESP_LOGI(TAG, "Command received: %s", message);

    // Forward the command to STM32.
    uart_stm32_send_command(message);

    // Update the LED display.
    led_control_set_direction(message);

    return ESP_OK;
}

// Run telemetry broadcast and vehicle failsafe.
static void websocket_task(void *arg)
{
    int64_t last_broadcast_time = 0;

    while (1) {
        int64_t now = esp_timer_get_time();

        // Stop the vehicle if communication is lost.
        if (vehicle_state_is_moving()) {

            int64_t last_active =
                vehicle_state_get_last_active();

            if (now - last_active > WS_FAILSAFE_TIMEOUT_US) {

                ESP_LOGW(
                    TAG,
                    "Failsafe timeout. Stopping vehicle."
                );

                uart_stm32_send_command("S");
                led_control_set_direction("S");

                vehicle_state_set_last_active(now);
            }
        }

        // Broadcast telemetry every 250 ms.
        if (now - last_broadcast_time >=
            (WS_TELEMETRY_INTERVAL_MS * 1000LL)) {

            if (ws_server != NULL &&
                websocket_get_client_count() > 0) {

                esp_err_t ret = httpd_queue_work(
                    ws_server,
                    websocket_broadcast_work,
                    NULL
                );

                if (ret != ESP_OK) {
                    ESP_LOGW(
                        TAG,
                        "Cannot queue telemetry broadcast"
                    );
                }
            }

            last_broadcast_time = now;
        }

        vTaskDelay(pdMS_TO_TICKS(WS_TASK_INTERVAL_MS));
    }
}

// Initialize the WebSocket server.
esp_err_t websocket_server_init(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    config.server_port = WS_SERVER_PORT;

    // Use a separate control port from the HTTP server.
    config.ctrl_port = WS_SERVER_CTRL_PORT;

    // Allow two clients and internal server sockets.
    config.max_open_sockets = 5;

    config.max_uri_handlers = 4;
    config.stack_size = 6144;
    config.lru_purge_enable = true;

    // Enable TCP keepalive.
    config.keep_alive_enable = true;
    config.keep_alive_idle = 5;
    config.keep_alive_interval = 5;
    config.keep_alive_count = 3;

    esp_err_t ret = httpd_start(&ws_server, &config);

    if (ret != ESP_OK) {
        ESP_LOGE(
            TAG,
            "Cannot start WebSocket server: %s",
            esp_err_to_name(ret)
        );
        return ret;
    }

    // Register the WebSocket endpoint.
    httpd_uri_t ws_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = websocket_handler,
        .user_ctx = NULL,
        .is_websocket = true
    };

    ret = httpd_register_uri_handler(
        ws_server,
        &ws_uri
    );

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Cannot register WebSocket URI");

        httpd_stop(ws_server);
        ws_server = NULL;

        return ret;
    }

    // Monitor client disconnections.
    ret = esp_event_handler_register(
        ESP_HTTP_SERVER_EVENT,
        HTTP_SERVER_EVENT_DISCONNECTED,
        websocket_event_handler,
        NULL
    );

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Cannot register HTTP server event handler");

        httpd_stop(ws_server);
        ws_server = NULL;

        return ret;
    }

    // Start the telemetry and failsafe task.
    BaseType_t task_ret = xTaskCreate(
        websocket_task,
        "websocket_task",
        WS_TASK_STACK,
        NULL,
        WS_TASK_PRIORITY,
        NULL
    );

    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Cannot create WebSocket task");

        esp_event_handler_unregister(
            ESP_HTTP_SERVER_EVENT,
            HTTP_SERVER_EVENT_DISCONNECTED,
            websocket_event_handler
        );

        httpd_stop(ws_server);
        ws_server = NULL;

        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(
        TAG,
        "WebSocket server started on port %d",
        WS_SERVER_PORT
    );

    return ESP_OK;
}
