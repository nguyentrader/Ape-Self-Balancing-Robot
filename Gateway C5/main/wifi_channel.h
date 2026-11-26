#ifndef WIFI_CHANNEL_H
#define WIFI_CHANNEL_H

#include <stdint.h>
#include <stdbool.h>

#include "esp_err.h"

// Default Wi-Fi channel.
#define WIFI_CHANNEL_DEFAULT 36

// Initialize channel configuration from NVS.
esp_err_t wifi_channel_config_init(void);

// Get the currently configured channel.
uint8_t wifi_channel_config_get(void);

// Save a new channel to NVS.
esp_err_t wifi_channel_config_set(uint8_t channel);

// Check whether a channel is supported.
bool wifi_channel_is_valid(uint8_t channel);

#endif
