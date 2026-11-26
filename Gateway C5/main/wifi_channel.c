#include "wifi_channel.h"

#include "nvs.h"
#include "nvs_flash.h"

#define WIFI_CHANNEL_NVS_NAMESPACE "wifi_cfg"
#define WIFI_CHANNEL_NVS_KEY       "channel"

static uint8_t current_channel = WIFI_CHANNEL_DEFAULT;

// Check whether the channel is allowed by this configuration.
bool wifi_channel_is_valid(uint8_t channel)
{
    switch (channel) {
        case 36:
        case 40:
        case 44:
        case 48:
        case 149:
        case 153:
        case 157:
        case 161:
        case 165:
            return true;

        default:
            return false;
    }
}

// Load the saved channel or create the default setting.
esp_err_t wifi_channel_config_init(void)
{
    nvs_handle_t handle;
    esp_err_t ret;
    uint8_t channel;

    ret = nvs_open(
        WIFI_CHANNEL_NVS_NAMESPACE,
        NVS_READWRITE,
        &handle
    );

    if (ret != ESP_OK) {
        return ret;
    }

    ret = nvs_get_u8(handle, WIFI_CHANNEL_NVS_KEY, &channel);

    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        channel = WIFI_CHANNEL_DEFAULT;

        ret = nvs_set_u8(
            handle,
            WIFI_CHANNEL_NVS_KEY,
            channel
        );

        if (ret == ESP_OK) {
            ret = nvs_commit(handle);
        }
    } else if (ret == ESP_OK && !wifi_channel_is_valid(channel)) {
        channel = WIFI_CHANNEL_DEFAULT;

        ret = nvs_set_u8(
            handle,
            WIFI_CHANNEL_NVS_KEY,
            channel
        );

        if (ret == ESP_OK) {
            ret = nvs_commit(handle);
        }
    }

    if (ret == ESP_OK) {
        current_channel = channel;
    }

    nvs_close(handle);

    return ret;
}

// Return the current channel.
uint8_t wifi_channel_config_get(void)
{
    return current_channel;
}

// Validate and save a new channel.
esp_err_t wifi_channel_config_set(uint8_t channel)
{
    nvs_handle_t handle;
    esp_err_t ret;

    if (!wifi_channel_is_valid(channel)) {
        return ESP_ERR_INVALID_ARG;
    }

    ret = nvs_open(
        WIFI_CHANNEL_NVS_NAMESPACE,
        NVS_READWRITE,
        &handle
    );

    if (ret != ESP_OK) {
        return ret;
    }

    ret = nvs_set_u8(
        handle,
        WIFI_CHANNEL_NVS_KEY,
        channel
    );

    if (ret == ESP_OK) {
        ret = nvs_commit(handle);
    }

    nvs_close(handle);

    if (ret == ESP_OK) {
        current_channel = channel;
    }

    return ret;
}
