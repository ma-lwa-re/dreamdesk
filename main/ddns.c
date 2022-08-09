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
#include "ddns.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"

static const char *DDNS_TAG = "ddns";

esp_err_t nvs_ddns_get_str(char *key, char *value) {
    esp_err_t err = nvs_flash_init_partition("wifi");

    if(err != ESP_OK) {
        ESP_LOGE(DDNS_TAG, "Error initializing nvs flash: %s", esp_err_to_name(err));
        return ESP_ERR_INVALID_STATE;
    }

    nvs_handle_t nvs_handle;
    err = nvs_open_from_partition("wifi", "wifi", NVS_READONLY, &nvs_handle);

    if(err != ESP_OK) {
        ESP_LOGE(DDNS_TAG, "Error opening NVS handle for key %s: %s", key, esp_err_to_name(err));
        nvs_close(nvs_handle);
        return ESP_ERR_NO_MEM;
    }

    size_t value_size;
    err = nvs_get_str(nvs_handle, key, NULL, &value_size);
    err = nvs_get_str(nvs_handle, key, value, &value_size);

    if(err != ESP_OK) {
        ESP_LOGE(DDNS_TAG, "Error reading key %s: %s", key, esp_err_to_name(err));
        nvs_close(nvs_handle);
        return ESP_ERR_NOT_FOUND;
    }

    nvs_close(nvs_handle);

    err = nvs_flash_deinit_partition("wifi");

    if(err != ESP_OK) {
        ESP_LOGE(DDNS_TAG, "Error deinitializing nvs flash: %s", esp_err_to_name(err));
        return ESP_ERR_INVALID_STATE;
    }
    return ESP_OK;
}

esp_http_client_config_t ddns_config = {
    .url = "https://httpbin.org",
    .user_agent = DDNS_USER_AGENT,
    .crt_bundle_attach = esp_crt_bundle_attach
};

bool ddns_update(char *api_endpoint, esp_http_client_method_t http_method) {
    esp_http_client_handle_t client = esp_http_client_init(&ddns_config);

    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_http_client_set_url(client, api_endpoint));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_http_client_set_method(client, http_method));

    esp_err_t err = esp_http_client_perform(client);

    if(err != ESP_OK) {
        return false;
    }

    HttpStatus_Code status_code = esp_http_client_get_status_code(client);
    ESP_ERROR_CHECK(esp_http_client_cleanup(client));

    if((status_code & HttpStatus_Ok) == HttpStatus_Ok) {
        ESP_LOGI(DDNS_TAG, "DDNS record successfully updated with return code %d", status_code);
    } else {
        ESP_LOGW(DDNS_TAG, "DDNS record update error with return code %d", status_code);
    }

    return (status_code & HttpStatus_Ok) == HttpStatus_Ok ? true : false;
}

bool ddns_get_update(char *api_endpoint) {
    return ddns_update(api_endpoint, HTTP_METHOD_GET);
}

bool ddns_post_update(char *api_endpoint) {
    return ddns_update(api_endpoint, HTTP_METHOD_POST);
}

void ddns_task(void *arg) {
    vTaskDelay(DDNS_INIT_DELAY / portTICK_PERIOD_MS);
    char api_endpoint[0xFF];

    esp_err_t err = nvs_ddns_get_str("ddns", api_endpoint);

    if(err != ESP_OK) {
        ESP_LOGE(DDNS_TAG, "DDNS endpoint not found! Canceling record update.");
        vTaskDelete(NULL);
        return;
    }
 
    for(;;) {
        ESP_LOGI(DDNS_TAG, "Updating the DDNS IP record...");

        if(!ddns_post_update(api_endpoint)) {
            ESP_LOGE(DDNS_TAG, "Error updating the DDNS record!");
        }

        vTaskDelay(SLEEP_DELAY_8_HOURS / portTICK_PERIOD_MS);
    }
}
