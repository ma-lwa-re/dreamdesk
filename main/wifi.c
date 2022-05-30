/* MIT License
*
* Copyright (c) 2022 ma-lwa-re
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/
#include <string.h>
#include "esp_event.h"
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "nvs_flash.h"
#include "wifi.h"

static const char *WIFI_TAG = "wifi station";

wifi_config_t wifi_config = {
    .sta = {
        .ssid = "DEFAULT_SSID",
        .password = "DEFAULT_PASSWORD"
    },
};

esp_err_t nvs_wifi_get_str(char *key, char *value, size_t *value_size) {
    esp_err_t err = nvs_flash_init_partition("wifi");

    if(err != ESP_OK) {
        ESP_LOGE(WIFI_TAG, "Error initializing nvs flash: %s", esp_err_to_name(err));
        return ESP_ERR_INVALID_STATE;
    }

    nvs_handle_t nvs_handle;
    err = nvs_open_from_partition("wifi", "wifi", NVS_READONLY, &nvs_handle);

    if(err != ESP_OK) {
        ESP_LOGE(WIFI_TAG, "Error opening NVS handle for key %s: %s", key, esp_err_to_name(err));
        nvs_close(nvs_handle);
        return ESP_ERR_NO_MEM;
    }

    err = nvs_get_str(nvs_handle, key, NULL, value_size);
    err = nvs_get_str(nvs_handle, key, value, value_size);

    if(err != ESP_OK) {
        ESP_LOGE(WIFI_TAG, "Error reading key %s: %s", key, esp_err_to_name(err));
        nvs_close(nvs_handle);
        return ESP_ERR_NOT_FOUND;
    }

    nvs_close(nvs_handle);

    err = nvs_flash_deinit_partition("wifi");

    if(err != ESP_OK) {
        ESP_LOGE(WIFI_TAG, "Error deinitializing nvs flash: %s", esp_err_to_name(err));
        return ESP_ERR_INVALID_STATE;
    }
    return ESP_OK;
}

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {

    if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
        ESP_LOGW(WIFI_TAG, "Connect to the AP failed, retrying");
    } else if(event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(WIFI_TAG, "IP " IPSTR, IP2STR(&event->ip_info.ip));
    }
}

void app_wifi_init() {
    esp_event_loop_create_default();
    ESP_ERROR_CHECK(esp_netif_init());

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
}

void app_wifi_credentials() {
    char wifi_ssid[sizeof(wifi_config.sta.ssid)];
    char wifi_password[sizeof(wifi_config.sta.password)];

    size_t value_size;
    esp_err_t err = nvs_wifi_get_str("ssid", wifi_ssid, &value_size);

    if(err == ESP_OK) {
        strncpy((char*) wifi_config.sta.ssid, wifi_ssid, value_size);
    }

    err = nvs_wifi_get_str("password", wifi_password, &value_size);

    if(err == ESP_OK) {
        strncpy((char*) wifi_config.sta.password, wifi_password, value_size);
    }
}

esp_err_t app_wifi_connect() {
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(WIFI_TAG, "Wifi initialized");
    return ESP_OK;
}

esp_err_t app_wifi_disconnect() {
    ESP_ERROR_CHECK(esp_wifi_stop());
    return ESP_OK;
}
