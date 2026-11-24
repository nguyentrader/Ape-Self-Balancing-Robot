idf_component_register(
    SRCS
        "main.c"
        "vehicle_state.c"
        "uart_stm32.c"
        "led_control.c"
        "wifi_ap.c"
        "websocket.c"
        "web_server.c"

    INCLUDE_DIRS
        "."

    EMBED_TXTFILES
        "web_page.html"

    REQUIRES
        esp_wifi
        esp_event
        esp_netif
        nvs_flash
        esp_http_server
        esp_timer
        esp_driver_uart
        led_strip
        littlefs
)

littlefs_create_partition_image(
    littlefs
    ../littlefs_data
    FLASH_IN_PROJECT
)