#include "web_server.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "esp_littlefs.h"

#define HTTP_SERVER_PORT 80
#define HTTP_SERVER_CTRL_PORT 32768

#define LITTLEFS_BASE_PATH "/littlefs"
#define LITTLEFS_PARTITION "littlefs"

#define FILE_BUFFER_SIZE 1024

static const char *TAG = "WEB_SERVER";

static httpd_handle_t http_server = NULL;

static bool littlefs_ready = false;

/*
 * HTML duoc nhung truc tiep vao firmware
 * thong qua EMBED_TXTFILES trong CMakeLists.txt.
 */
extern const uint8_t web_page_html_start[] asm("_binary_web_page_html_start");

/* ---------------------------------------------------------
 * Khoi tao LittleFS
 * --------------------------------------------------------- */

static esp_err_t littlefs_init(void)
{
    const esp_vfs_littlefs_conf_t config = {
        .base_path = LITTLEFS_BASE_PATH,
        .partition_label = LITTLEFS_PARTITION,
        .format_if_mount_failed = false,
        .read_only = true,
        .dont_mount = false,
        .grow_on_mount = false
    };

    esp_err_t ret = esp_vfs_littlefs_register(&config);

    if (ret != ESP_OK)
    {
        ESP_LOGW(TAG, "LittleFS unavailable: %s",
                 esp_err_to_name(ret));

        littlefs_ready = false;

        return ESP_OK;
    }

    littlefs_ready = true;

    ESP_LOGI(TAG, "LittleFS mounted successfully");

    size_t total_bytes = 0;
    size_t used_bytes = 0;

    ret = esp_littlefs_info(
        LITTLEFS_PARTITION,
        &total_bytes,
        &used_bytes
    );

    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG,
                 "LittleFS: total=%u, used=%u bytes",
                 (unsigned int)total_bytes,
                 (unsigned int)used_bytes);
    }
    else
    {
        ESP_LOGW(TAG, "Cannot get LittleFS information: %s",
                 esp_err_to_name(ret));
    }

    return ESP_OK;
}

/* ---------------------------------------------------------
 * Xu ly GET /
 *
 * Tra ve giao dien HTML.
 * --------------------------------------------------------- */

static esp_err_t root_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html; charset=utf-8");

    httpd_resp_set_hdr(
        req,
        "Cache-Control",
        "no-cache");

    esp_err_t ret = httpd_resp_send(
        req,
        (const char *)web_page_html_start,
        HTTPD_RESP_USE_STRLEN);

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Cannot send HTML page");
    }

    return ret;
}

/* ---------------------------------------------------------
 * Xu ly GET /background.jpg
 *
 * Doc anh tu LittleFS va gui theo tung chunk.
 * --------------------------------------------------------- */

static esp_err_t background_handler(httpd_req_t *req)
{
    if (!littlefs_ready)
    {
        ESP_LOGW(TAG, "LittleFS is not available");

        httpd_resp_send_err(
            req,
            HTTPD_404_NOT_FOUND,
            "Background image not available"
        );

        return ESP_OK;
    }

    const char *file_path = LITTLEFS_BASE_PATH "/background.jpg";

    FILE *file = fopen(file_path, "rb");

    if (file == NULL)
    {
        ESP_LOGW(TAG, "Cannot open %s", file_path);

        httpd_resp_send_err(
            req,
            HTTPD_404_NOT_FOUND,
            "background.jpg not found"
        );

        return ESP_OK;
    }

    httpd_resp_set_type(req, "image/jpeg");

    httpd_resp_set_hdr(
        req,
        "Cache-Control",
        "public, max-age=86400"
    );

    char buffer[FILE_BUFFER_SIZE];

    size_t bytes_read;

    while ((bytes_read = fread(
                buffer,
                1,
                sizeof(buffer),
                file)) > 0)
    {
        esp_err_t ret = httpd_resp_send_chunk(
            req,
            buffer,
            bytes_read
        );

        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Error sending background image");
            fclose(file);
            return ret;
        }
    }

    fclose(file);

    return httpd_resp_send_chunk(req, NULL, 0);
}

/* ---------------------------------------------------------
 * Khoi tao HTTP server
 * --------------------------------------------------------- */

esp_err_t web_server_init(void)
{
    esp_err_t ret = littlefs_init();

    if (ret != ESP_OK)
    {
        return ret;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    config.server_port = HTTP_SERVER_PORT;
    config.ctrl_port = HTTP_SERVER_CTRL_PORT;

    config.max_open_sockets = 7;
    config.max_uri_handlers = 4;

    config.stack_size = 6144;

    config.lru_purge_enable = true;

    ret = httpd_start(&http_server, &config);

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Cannot start HTTP server: %s",
                 esp_err_to_name(ret));
        return ret;
    }

    /*
     * URI /
     */
    const httpd_uri_t root_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = root_handler,
        .user_ctx = NULL};

    ret = httpd_register_uri_handler(
        http_server,
        &root_uri);

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Cannot register root URI");
        httpd_stop(http_server);
        http_server = NULL;
        return ret;
    }

    /*
     * URI /background.jpg
     */
    const httpd_uri_t background_uri = {
        .uri = "/background.jpg",
        .method = HTTP_GET,
        .handler = background_handler,
        .user_ctx = NULL};

    ret = httpd_register_uri_handler(
        http_server,
        &background_uri);

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Cannot register background URI");
        httpd_stop(http_server);
        http_server = NULL;
        return ret;
    }

    ESP_LOGI(TAG, "HTTP server started on port %d",
             HTTP_SERVER_PORT);

    return ESP_OK;
}