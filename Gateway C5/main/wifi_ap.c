#include "wifi_ap.h"

#include <stdint.h>
#include <string.h>

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_netif.h"

#define WIFI_AP_SSID        "Xe Tàu Loại 3"
#define WIFI_AP_PASSWORD    "12345678"

#define WIFI_AP_MAX_CLIENTS 2
#define WIFI_AP_TX_POWER    78

#define WIFI_CHANNEL_5G     36
#define WIFI_CHANNEL_2G     1

static const char *TAG = "WIFI_AP";

static esp_netif_t *ap_netif = NULL;

static esp_err_t set_ap_channel(uint8_t channel)
{
    wifi_config_t ap_config = {
        .ap = {
            .ssid_len = sizeof(WIFI_AP_SSID) - 1,
            .channel = channel,
            .password = WIFI_AP_PASSWORD,
            .max_connection = WIFI_AP_MAX_CLIENTS,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .ssid_hidden = 0,
            .beacon_interval = 100
        }
    };

    memcpy(
        ap_config.ap.ssid,
        WIFI_AP_SSID,
        sizeof(WIFI_AP_SSID) - 1
    );

    return esp_wifi_set_config(WIFI_IF_AP, &ap_config);
}

esp_err_t wifi_ap_init(void)
{
    esp_err_t ret;

    ap_netif = esp_netif_create_default_wifi_ap();

    if (ap_netif == NULL) {
        ESP_LOGE(TAG, "Cannot create AP network interface");
        return ESP_FAIL;
    }

    wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();

    ret = esp_wifi_init(&init_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi initialization failed: %s",
                 esp_err_to_name(ret));
        return ret;
    }

    ret = esp_wifi_set_mode(WIFI_MODE_AP);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Cannot set AP mode: %s",
                 esp_err_to_name(ret));
        return ret;
    }

    // Configure 5 GHz channel before starting WiFi.
    ret = set_ap_channel(WIFI_CHANNEL_5G);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Cannot configure 5 GHz AP: %s",
                 esp_err_to_name(ret));
        return ret;
    }

    ret = esp_wifi_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Cannot start WiFi: %s",
                 esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "WiFi driver started");

    // Select 5 GHz after WiFi has started.
    ret = esp_wifi_set_band_mode(WIFI_BAND_MODE_5G_ONLY);

    if (ret == ESP_OK) {
        ret = esp_wifi_set_channel(
            WIFI_CHANNEL_5G,
            WIFI_SECOND_CHAN_NONE
        );
    }

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "5 GHz SoftAP selected, channel %d",
                 WIFI_CHANNEL_5G);
    } else {
        ESP_LOGW(TAG, "5 GHz unavailable: %s",
                 esp_err_to_name(ret));

        // Switch to 2.4 GHz.
        ret = esp_wifi_set_band_mode(WIFI_BAND_MODE_2G_ONLY);

        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Cannot select 2.4 GHz: %s",
                     esp_err_to_name(ret));
            esp_wifi_stop();
            return ret;
        }

        ret = set_ap_channel(WIFI_CHANNEL_2G);

        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Cannot configure 2.4 GHz AP: %s",
                     esp_err_to_name(ret));
            esp_wifi_stop();
            return ret;
        }

        ret = esp_wifi_set_channel(
            WIFI_CHANNEL_2G,
            WIFI_SECOND_CHAN_NONE
        );

        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Cannot set 2.4 GHz channel: %s",
                     esp_err_to_name(ret));
            esp_wifi_stop();
            return ret;
        }

        ESP_LOGI(TAG, "2.4 GHz SoftAP selected, channel %d",
                 WIFI_CHANNEL_2G);
    }

    ret = esp_wifi_set_max_tx_power(WIFI_AP_TX_POWER);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Cannot set TX power: %s",
                 esp_err_to_name(ret));
        esp_wifi_stop();
        return ret;
    }

    ret = esp_wifi_set_ps(WIFI_PS_NONE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Cannot disable WiFi power save: %s",
                 esp_err_to_name(ret));
        esp_wifi_stop();
        return ret;
    }

    esp_netif_ip_info_t ip_info;

    ret = esp_netif_get_ip_info(ap_netif, &ip_info);

    if (ret == ESP_OK) {
        ESP_LOGI(
            TAG,
            "SoftAP ready | SSID: %s | IP: " IPSTR,
            WIFI_AP_SSID,
            IP2STR(&ip_info.ip)
        );
    }

    return ESP_OK;
}
