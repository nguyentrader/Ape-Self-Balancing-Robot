#include "websocket.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
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

#define WS_SERVER_PORT             81
#define WS_SERVER_CTRL_PORT        32769
#define WS_MAX_CLIENTS             2
#define WS_FAILSAFE_TIMEOUT_US     900000
#define WS_TELEMETRY_INTERVAL_MS   250
#define WS_TASK_INTERVAL_MS        50
#define WS_RX_BUFFER_SIZE          64

static const char *TAG = "WEBSOCKET";

static httpd_handle_t ws_server = NULL;


/* ---------------------------------------------------------
 * Dem so client WebSocket dang ket noi
 * --------------------------------------------------------- */

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


/* ---------------------------------------------------------
 * Gui mot frame Text cho client
 *
 * Ham nay duoc goi trong context cua HTTP server task.
 * --------------------------------------------------------- */

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
        ESP_LOGW(TAG, "Cannot send WS frame to fd=%d: %s",
                 fd, esp_err_to_name(ret));
    }
}


/* ---------------------------------------------------------
 * Gui telemetry hien tai cho mot client
 *
 * Duoc dua vao httpd_queue_work de dam bao frame chi duoc
 * gui sau khi handshake WebSocket hoan tat.
 * --------------------------------------------------------- */

static void websocket_send_initial_work(void *arg)
{
    int fd = (int)(intptr_t)arg;

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

    snprintf(message, sizeof(message), "T:%s,%s", speed, angle);

    websocket_send_text(fd, message);

    ESP_LOGI(TAG, "Initial telemetry sent to fd=%d", fd);
}


/* ---------------------------------------------------------
 * Broadcast telemetry cho tat ca client
 * --------------------------------------------------------- */

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

    snprintf(message, sizeof(message), "T:%s,%s", speed, angle);

    for (size_t i = 0; i < client_count; i++) {
        int fd = client_fds[i];

        if (httpd_ws_get_fd_info(
                ws_server,
                fd) == HTTPD_WS_CLIENT_WEBSOCKET) {

            websocket_send_text(fd, message);
        }
    }
}


/* ---------------------------------------------------------
 * Xu ly khi client WebSocket ngat ket noi
 * --------------------------------------------------------- */

static void websocket_disconnect_work(void *arg)
{
    size_t client_count = websocket_get_client_count();

    ESP_LOGI(TAG, "Remaining WebSocket clients: %u",
             (unsigned int)client_count);

    if (client_count == 0 && vehicle_state_is_moving()) {

        ESP_LOGW(TAG, "Last client disconnected. Stopping vehicle.");

        uart_stm32_send_command("S");
        led_control_set_direction("S");

        vehicle_state_set_last_active(esp_timer_get_time());
    }
}


static void websocket_event_handler(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data)
{
    if (event_id != HTTP_SERVER_EVENT_DISCONNECTED) {
        return;
    }

    if (ws_server != NULL) {
        esp_err_t ret = httpd_queue_work(
            ws_server,
            websocket_disconnect_work,
            NULL
        );

        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Cannot queue disconnect work");
        }
    }
}


/* ---------------------------------------------------------
 * WebSocket URI handler
 * --------------------------------------------------------- */

static esp_err_t websocket_handler(httpd_req_t *req)
{
    int fd = httpd_req_to_sockfd(req);

    /*
     * Lan dau handler duoc goi la request GET de handshake.
     */
    if (req->method == HTTP_GET) {

        size_t current_clients = websocket_get_client_count();

        if (current_clients >= WS_MAX_CLIENTS) {
            ESP_LOGW(TAG, "Maximum WebSocket clients reached");
            return ESP_FAIL;
        }

        vehicle_state_set_last_active(esp_timer_get_time());

        ESP_LOGI(TAG, "WebSocket handshake requested: fd=%d", fd);

        /*
         * Gui telemetry sau khi handshake hoan tat.
         */
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


    /*
     * Lay kich thuoc frame truoc.
     */
    httpd_ws_frame_t frame = {0};

    esp_err_t ret = httpd_ws_recv_frame(
        req,
        &frame,
        0
    );

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Cannot get WebSocket frame size");
        return ret;
    }

    /*
     * Chi nhan Text frame.
     */
    if (frame.type != HTTPD_WS_TYPE_TEXT) {
        return ESP_OK;
    }

    /*
     * Kiem tra kich thuoc du lieu.
     */
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
        ESP_LOGE(TAG, "Cannot receive WebSocket payload");
        return ret;
    }

    message[frame.len] = '\0';

    /*
     * Cap nhat thoi diem hoat dong.
     */
    vehicle_state_set_last_active(esp_timer_get_time());

    /*
     * K la keepalive tu trinh duyet.
     * Khong gui K xuong STM32.
     */
    if (strcmp(message, "K") == 0) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Command received: %s", message);

    /*
     * Gui lenh xuong STM32.
     */
    uart_stm32_send_command(message);

    /*
     * Cap nhat LED theo huong di chuyen.
     */
    led_control_set_direction(message);

    return ESP_OK;
}


/* ---------------------------------------------------------
 * Task broadcast telemetry va failsafe
 * --------------------------------------------------------- */

static void websocket_task(void *arg)
{
    int64_t last_broadcast_time = 0;

    while (1) {

        int64_t now = esp_timer_get_time();

        /*
         * Failsafe:
         * Neu xe dang di chuyen va khong nhan duoc message
         * tu client trong hon 900 ms thi dung xe.
         */
        if (vehicle_state_is_moving()) {

            int64_t last_active = vehicle_state_get_last_active();

            if (now - last_active > WS_FAILSAFE_TIMEOUT_US) {

                ESP_LOGW(TAG, "Failsafe timeout. Stopping vehicle.");

                uart_stm32_send_command("S");
                led_control_set_direction("S");

                vehicle_state_set_last_active(now);
            }
        }

        /*
         * Broadcast telemetry moi 250 ms.
         */
        if (now - last_broadcast_time >=
            (WS_TELEMETRY_INTERVAL_MS * 1000)) {

            if (websocket_get_client_count() > 0) {

                if (ws_server != NULL) {
                    esp_err_t ret = httpd_queue_work(
                        ws_server,
                        websocket_broadcast_work,
                        NULL
                    );

                    if (ret != ESP_OK) {
                        ESP_LOGW(TAG, "Cannot queue telemetry broadcast");
                    }
                }
            }

            last_broadcast_time = now;
        }

        vTaskDelay(pdMS_TO_TICKS(WS_TASK_INTERVAL_MS));
    }
}


/* ---------------------------------------------------------
 * Khoi tao WebSocket server
 * --------------------------------------------------------- */

esp_err_t websocket_server_init(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    config.server_port = WS_SERVER_PORT;

    /*
     * Moi HTTP server instance can mot UDP control port rieng.
     * HTTP server cong 80 se dung port mac dinh 32768.
     */
    config.ctrl_port = WS_SERVER_CTRL_PORT;

    /*
     * HTTPD tinh ca socket noi bo trong max_open_sockets.
     */
    config.max_open_sockets = 5;

    config.max_uri_handlers = 4;

    config.stack_size = 6144;

    config.lru_purge_enable = true;

    /*
     * TCP keepalive de phat hien ket noi TCP chet.
     */
    config.keep_alive_enable = true;
    config.keep_alive_idle = 5;
    config.keep_alive_interval = 5;
    config.keep_alive_count = 3;


    esp_err_t ret = httpd_start(&ws_server, &config);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Cannot start WebSocket server: %s",
                 esp_err_to_name(ret));
        return ret;
    }


    /*
     * Dang ky WebSocket URI.
     */
    httpd_uri_t ws_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = websocket_handler,
        .user_ctx = NULL,
        .is_websocket = true
    };

    ret = httpd_register_uri_handler(ws_server, &ws_uri);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Cannot register WebSocket URI");
        httpd_stop(ws_server);
        ws_server = NULL;
        return ret;
    }


    /*
     * Theo doi su kien disconnect cua HTTP server.
     */
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


    /*
     * Task broadcast telemetry va failsafe.
     */
    BaseType_t task_ret = xTaskCreate(
        websocket_task,
        "websocket_task",
        4096,
        NULL,
        5,
        NULL
    );

    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Cannot create WebSocket task");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "WebSocket server started on port %d",
             WS_SERVER_PORT);

    return ESP_OK;
}