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
#include "esp_crt_bundle.h"

static const char *OTA_TAG = "ota_updates";

void print_app_desc(esp_app_desc_t app_desc, char *app, esp_log_level_t log_level) {
    ESP_LOG_LEVEL(log_level, OTA_TAG, "%s firmware project: %s", app, app_desc.project_name);
    ESP_LOG_LEVEL(log_level, OTA_TAG, "%s firmware version: %s", app, app_desc.version);
    ESP_LOG_LEVEL(log_level, OTA_TAG, "%s firmware compile date: %s", app, app_desc.date);
    ESP_LOG_LEVEL(log_level, OTA_TAG, "%s firmware compile time: %s", app, app_desc.time);
}

esp_err_t validate_image_header(esp_https_ota_handle_t *https_ota_handle) {
    esp_http_client_config_t ota_client_config = {
        .url = OTA_UPDATE_URL,
        .user_agent = OTA_UPDATE_USER_AGENT,
        .crt_bundle_attach = esp_crt_bundle_attach
    };

    esp_https_ota_config_t ota_config = {
        .http_config = &ota_client_config
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

    if(memcmp(update_app_info.project_name, running_app_info.project_name, sizeof(update_app_info.project_name)) != 0) {
        ESP_LOGE(OTA_TAG, "Invalid project name!");
        return ESP_FAIL;
    }

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
