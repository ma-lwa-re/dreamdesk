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
#include "ota.h"
#include "esp_log.h"

static const char *OTA_TAG = "ota_updates";

/*
* Certificate:
*     Data:
*         Version: 3 (0x2)
*         Serial Number:
*             82:10:cf:b0:d2:40:e3:59:44:63:e0:bb:63:82:8b:00
*         Signature Algorithm: sha256WithRSAEncryption
*         Issuer: C = US, O = Internet Security Research Group, CN = ISRG Root X1
*         Validity
*             Not Before: Jun  4 11:04:38 2015 GMT
*             Not After : Jun  4 11:04:38 2035 GMT
*         Subject: C = US, O = Internet Security Research Group, CN = ISRG Root X1
*/
const char *LETS_ENCRYPT_X1_ROOT_CA = "-----BEGIN CERTIFICATE-----\n"
 "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\n"
 "-----END CERTIFICATE-----";

char *ota_user_agent;

void print_app_desc(esp_app_desc_t app_desc, char *app, esp_log_level_t log_level) {
    ESP_LOG_LEVEL(log_level, OTA_TAG, "%s firmware project: %s", app, app_desc.project_name);
    ESP_LOG_LEVEL(log_level, OTA_TAG, "%s firmware version: %s", app, app_desc.version);
    ESP_LOG_LEVEL(log_level, OTA_TAG, "%s firmware compile date: %s", app, app_desc.date);
    ESP_LOG_LEVEL(log_level, OTA_TAG, "%s firmware compile time: %s", app, app_desc.time);
}

esp_err_t validate_image_header(esp_https_ota_handle_t *https_ota_handle) {
    esp_http_client_config_t config = {
        .url = OTA_UPDATE_URL,
        .cert_pem = LETS_ENCRYPT_X1_ROOT_CA,
        .user_agent = ota_user_agent
    };

    esp_https_ota_config_t ota_config = {
        .http_config = &config
    };

    const esp_partition_t *running_partition = esp_ota_get_running_partition();
    esp_app_desc_t running_app_info;

    if(esp_ota_get_partition_description(running_partition, &running_app_info) != ESP_OK) {
        return ESP_FAIL;
    }

    print_app_desc(running_app_info, "Running", ESP_LOG_INFO);

    esp_err_t err = esp_https_ota_begin(&ota_config, https_ota_handle);
    if(err != ESP_OK) {
        ESP_LOGE(OTA_TAG, "esp_https_ota_begin failed!");
        return ESP_FAIL;
    }

    esp_app_desc_t update_app_info;
    err = esp_https_ota_get_img_desc(*https_ota_handle, &update_app_info);
    if(err != ESP_OK) {
        ESP_LOGE(OTA_TAG, "esp_https_ota_get_img_desc failed!");
        return ESP_FAIL;
    }

    print_app_desc(update_app_info, "Update", ESP_LOG_WARN);

    if(memcmp(update_app_info.version, running_app_info.version, sizeof(update_app_info.version)) <= 0) {
        ESP_LOGI(OTA_TAG, "Running firmware version is up to date!");
        return ESP_ERR_INVALID_VERSION;
    }

    const uint32_t hw_sec_version = esp_efuse_read_secure_version();
    if(update_app_info.secure_version < hw_sec_version) {
        ESP_LOGW(OTA_TAG, "New firmware security version is less than eFuse programmed, %d < %d", update_app_info.secure_version, hw_sec_version);
        return ESP_FAIL;
    }
    return ESP_OK;
}

void ota_task(void *arg) {
    asprintf(&ota_user_agent, "%s - %s v%s", OTA_UPDATE_USER_AGENT, OTA_PROJECT_NAME, OTA_PROJECT_VER);
    vTaskDelay(SLEEP_INTERVAL_10_SEC / portTICK_PERIOD_MS);

    for(;;) {
        ESP_LOGI(OTA_TAG, "Checking for updates...");
        esp_https_ota_handle_t https_ota_handle = NULL;

        if(validate_image_header(&https_ota_handle) == ESP_OK) {
            ESP_LOGW(OTA_TAG, "Downloading latest version...");
            for(;;) {
                if(esp_https_ota_perform(https_ota_handle) != ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
                    break;
                }
                ESP_LOGD(OTA_TAG, "Image bytes read: %d", esp_https_ota_get_image_len_read(https_ota_handle));
            }

            if(esp_https_ota_is_complete_data_received(https_ota_handle)) {
                esp_err_t ota_finish_err = esp_https_ota_finish(https_ota_handle);

                if(ota_finish_err == ESP_OK) {
                    ESP_LOGI(OTA_TAG, "OTA update successful! Rebooting ...");
                    vTaskDelay(SLEEP_INTERVAL_10_SEC / portTICK_PERIOD_MS);
                    esp_restart();
                } else {
                    if(ota_finish_err == ESP_ERR_OTA_VALIDATE_FAILED) {
                        ESP_LOGE(OTA_TAG, "Image validation failed, image is corrupted!");
                    }
                    ESP_LOGE(OTA_TAG, "OTA update failed with error: %s", esp_err_to_name(ota_finish_err));
                }
            }
        }
        vTaskDelay(SLEEP_INTERVAL_12_HOURS / portTICK_PERIOD_MS);
    }
}
