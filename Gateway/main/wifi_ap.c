#include "wifi_ap.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"

#define WIFI_AP_SSID        "Xe Tàu Loại 3"
#define WIFI_AP_PASSWORD    "12345678"

#define WIFI_AP_MAX_CLIENTS 2
#define WIFI_AP_TX_POWER    78

#define WIFI_SCAN_MAX_AP    30

static const char *TAG = "WIFI_AP";

static esp_netif_t *ap_netif = NULL;

static int find_best_channel(void)
{
    const uint8_t candidates[3] = {1, 6, 11};
    int32_t score[3] = {0, 0, 0};

    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = true,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time.active.min = 100,
        .scan_time.active.max = 300
    };

    esp_err_t ret = esp_wifi_set_mode(WIFI_MODE_STA);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Cannot set STA mode: %s", esp_err_to_name(ret));
        return 11;
    }

    ret = esp_wifi_start();

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Cannot start WiFi for scanning: %s",
                 esp_err_to_name(ret));
        return 11;
    }

    ret = esp_wifi_scan_start(&scan_config, true);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi scan failed: %s", esp_err_to_name(ret));
        esp_wifi_stop();
        return 11;
    }

    uint16_t ap_count = 0;

    ret = esp_wifi_scan_get_ap_num(&ap_count);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Cannot get AP count: %s", esp_err_to_name(ret));
        esp_wifi_stop();
        return 11;
    }

    wifi_ap_record_t records[WIFI_SCAN_MAX_AP];

    uint16_t record_count = ap_count;

    if (record_count > WIFI_SCAN_MAX_AP) {
        record_count = WIFI_SCAN_MAX_AP;
    }

    if (record_count > 0) {
        ret = esp_wifi_scan_get_ap_records(&record_count, records);

        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Cannot get AP records: %s",
                     esp_err_to_name(ret));
            esp_wifi_stop();
            return 11;
        }

        for (int i = 0; i < record_count; i++) {
            int channel = records[i].primary;
            int rssi = records[i].rssi;

            int weight = 100 + rssi;

            if (weight < 1) {
                weight = 1;
            }

            for (int k = 0; k < 3; k++) {
                int distance = abs(channel - candidates[k]);

                if (distance == 0) {
                    score[k] += weight * 3;
                }
                else if (distance <= 2) {
                    score[k] += weight * 2;
                }
                else if (distance <= 4) {
                    score[k] += weight;
                }
            }
        }
    }

    int best_index = 2;

    if (score[1] < score[best_index]) {
        best_index = 1;
    }

    if (score[0] < score[best_index]) {
        best_index = 0;
    }

    int best_channel = candidates[best_index];

    ESP_LOGI(
        TAG,
        "Scan: %u networks | CH1:%ld CH6:%ld CH11:%ld | Selected CH%d",
        ap_count,
        (long)score[0],
        (long)score[1],
        (long)score[2],
        best_channel
    );

    ret = esp_wifi_stop();

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Cannot stop WiFi after scan: %s",
                 esp_err_to_name(ret));
        return 11;
    }

    return best_channel;
}

esp_err_t wifi_ap_init(void)
{
    esp_err_t ret;

    ap_netif = esp_netif_create_default_wifi_ap();

    if (ap_netif == NULL) {
        ESP_LOGE(TAG, "Cannot create default AP network interface");
        return ESP_FAIL;
    }

    wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();

    ret = esp_wifi_init(&init_config);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    int best_channel = find_best_channel();

    wifi_config_t ap_config = {
        .ap = {
            .ssid_len = strlen(WIFI_AP_SSID),
            .channel = best_channel,
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
        strlen(WIFI_AP_SSID)
    );

    ret = esp_wifi_set_mode(WIFI_MODE_AP);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Cannot set AP mode: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_wifi_set_config(WIFI_IF_AP, &ap_config);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Cannot set AP config: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW20);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Cannot set HT20 bandwidth: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_wifi_set_max_tx_power(WIFI_AP_TX_POWER);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Cannot set TX power: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_wifi_set_ps(WIFI_PS_NONE);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Cannot disable WiFi power save: %s",
                 esp_err_to_name(ret));
        return ret;
    }

    ret = esp_wifi_start();

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Cannot start SoftAP: %s", esp_err_to_name(ret));
        return ret;
    }

    esp_netif_ip_info_t ip_info;

    ret = esp_netif_get_ip_info(ap_netif, &ip_info);

    if (ret == ESP_OK) {
        ESP_LOGI(
            TAG,
            "SoftAP started | SSID: %s | Channel: %d | IP: " IPSTR,
            WIFI_AP_SSID,
            best_channel,
            IP2STR(&ip_info.ip)
        );
    }

    return ESP_OK;
}