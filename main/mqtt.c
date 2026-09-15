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
#include "mqtt.h"
#include "dreamdesk.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "cJSON.h"
#include "math.h"
#include <string.h>
#include <strings.h>

#if defined(SENSORS_ON)
#include "sensors.h"
#endif

static const char *MQTT_TAG = "mqtt";

static esp_mqtt_client_handle_t mqtt_client = NULL;
static bool mqtt_connected = false;

static char device_id[32] = {0};
static char topic_prefix[64] = MQTT_DEFAULT_TOPIC_PREFIX;
static char discovery_prefix[64] = MQTT_DEFAULT_DISCOVERY_PREFIX;
static char lwt_topic[128] = {0};

static uint8_t last_published_height = 0xFF;
static uint8_t last_published_target_height = 0xFF;
static uint8_t last_published_percentage = 0xFF;
static uint8_t last_published_min_height = 0xFF;
static uint8_t last_published_max_height = 0xFF;
static uint8_t last_published_presets[7] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

#if defined(SENSORS_ON)
static float last_published_temp = -999.0f;
static float last_published_hum = -999.0f;
static float last_published_co2 = -999.0f;
static float last_published_co2_peak = -999.0f;
static enum air_quality_t last_published_aq = UNKNOWN;
#endif

static esp_err_t nvs_mqtt_get_str(const char *key, char *value, size_t max_size) {
    esp_err_t err = nvs_flash_init_partition("wifi");
    if(err != ESP_OK) {
        ESP_LOGE(MQTT_TAG, "Error initializing NVS partition 'wifi': %s", esp_err_to_name(err));
        return err;
    }

    nvs_handle_t nvs_handle;
    err = nvs_open_from_partition("wifi", "wifi", NVS_READONLY, &nvs_handle);
    if(err != ESP_OK) {
        ESP_LOGW(MQTT_TAG, "NVS key '%s' open failed: %s", key, esp_err_to_name(err));
        nvs_flash_deinit_partition("wifi");
        return err;
    }

    size_t req_size = 0;
    err = nvs_get_str(nvs_handle, key, NULL, &req_size);
    if(err == ESP_OK && req_size <= max_size) {
        err = nvs_get_str(nvs_handle, key, value, &req_size);
    }

    nvs_close(nvs_handle);
    nvs_flash_deinit_partition("wifi");
    return err;
}

#if defined(SENSORS_ON)
static const char* air_quality_to_str(enum air_quality_t aq) {
    switch(aq) {
        case EXCELLENT: return "Excellent";
        case GOOD: return "Good";
        case FAIR: return "Fair";
        case INFERIOR: return "Inferior";
        case POOR: return "Poor";
        default: return "Unknown";
    }
}
#endif

static cJSON* create_device_info_json() {
    cJSON *dev = cJSON_CreateObject();
    if(!dev) return NULL;

    cJSON *ids = cJSON_CreateArray();
    if(ids) {
        cJSON_AddItemToArray(ids, cJSON_CreateString(device_id));
        cJSON_AddItemToObject(dev, "identifiers", ids);
    }
    cJSON_AddStringToObject(dev, "name", "Dreamdesk");
    cJSON_AddStringToObject(dev, "model", "Dreamdesk v2.5");
    cJSON_AddStringToObject(dev, "manufacturer", "ma.lwa.re");
    #if defined(PROJECT_VER)
    cJSON_AddStringToObject(dev, "sw_version", PROJECT_VER);
    #else
    cJSON_AddStringToObject(dev, "sw_version", "2.4.0.4");
    #endif
    return dev;
}

static void publish_ha_discovery_entity(const char *component, const char *object_id, cJSON *config_json) {
    if(!config_json) return;

    char disc_topic[256];
    snprintf(disc_topic, sizeof(disc_topic), "%s/%s/%s/%s/config", discovery_prefix, component, device_id, object_id);

    char *payload = cJSON_PrintUnformatted(config_json);
    if(payload) {
        ESP_LOGI(MQTT_TAG, "Publishing HA discovery for %s/%s", component, object_id);
        esp_mqtt_client_publish(mqtt_client, disc_topic, payload, 0, 1, 1);
        free(payload);
    }
    cJSON_Delete(config_json);
}

static void publish_ha_discovery(void) {
    char topic_buf[128];
    char uid_buf[128];

    // 1. Cover entity (Desk Position Control)
    {
        cJSON *root = cJSON_CreateObject();
        snprintf(uid_buf, sizeof(uid_buf), "%s_cover", device_id);
        cJSON_AddStringToObject(root, "name", "Desk");
        cJSON_AddStringToObject(root, "unique_id", uid_buf);

        snprintf(topic_buf, sizeof(topic_buf), "%s/cover/set", topic_prefix);
        cJSON_AddStringToObject(root, "command_topic", topic_buf);

        snprintf(topic_buf, sizeof(topic_buf), "%s/cover/position", topic_prefix);
        cJSON_AddStringToObject(root, "position_topic", topic_buf);

        snprintf(topic_buf, sizeof(topic_buf), "%s/cover/set_position", topic_prefix);
        cJSON_AddStringToObject(root, "set_position_topic", topic_buf);

        cJSON_AddStringToObject(root, "payload_open", "OPEN");
        cJSON_AddStringToObject(root, "payload_close", "CLOSE");
        cJSON_AddStringToObject(root, "payload_stop", "STOP");
        cJSON_AddNumberToObject(root, "position_open", 100);
        cJSON_AddNumberToObject(root, "position_closed", 0);

        cJSON_AddStringToObject(root, "availability_topic", lwt_topic);
        cJSON_AddStringToObject(root, "payload_available", "online");
        cJSON_AddStringToObject(root, "payload_not_available", "offline");

        cJSON_AddItemToObject(root, "device", create_device_info_json());
        publish_ha_discovery_entity("cover", "desk", root);
    }

    // 2. Sensor entity (Current Height in cm)
    {
        cJSON *root = cJSON_CreateObject();
        snprintf(uid_buf, sizeof(uid_buf), "%s_height", device_id);
        cJSON_AddStringToObject(root, "name", "Desk Height");
        cJSON_AddStringToObject(root, "unique_id", uid_buf);

        snprintf(topic_buf, sizeof(topic_buf), "%s/height/state", topic_prefix);
        cJSON_AddStringToObject(root, "state_topic", topic_buf);
        cJSON_AddStringToObject(root, "unit_of_measurement", "cm");
        cJSON_AddStringToObject(root, "icon", "mdi:arrow-up-down");

        cJSON_AddStringToObject(root, "availability_topic", lwt_topic);
        cJSON_AddStringToObject(root, "payload_available", "online");
        cJSON_AddStringToObject(root, "payload_not_available", "offline");

        cJSON_AddItemToObject(root, "device", create_device_info_json());
        publish_ha_discovery_entity("sensor", "height", root);
    }

    // 3. Number entity (Target Height in cm)
    {
        cJSON *root = cJSON_CreateObject();
        snprintf(uid_buf, sizeof(uid_buf), "%s_target_height", device_id);
        cJSON_AddStringToObject(root, "name", "Desk Target Height");
        cJSON_AddStringToObject(root, "unique_id", uid_buf);

        snprintf(topic_buf, sizeof(topic_buf), "%s/target_height/set", topic_prefix);
        cJSON_AddStringToObject(root, "command_topic", topic_buf);

        snprintf(topic_buf, sizeof(topic_buf), "%s/target_height/state", topic_prefix);
        cJSON_AddStringToObject(root, "state_topic", topic_buf);

        cJSON_AddNumberToObject(root, "min", 40);
        cJSON_AddNumberToObject(root, "max", 150);
        cJSON_AddNumberToObject(root, "step", 1);
        cJSON_AddStringToObject(root, "unit_of_measurement", "cm");
        cJSON_AddStringToObject(root, "mode", "slider");
        cJSON_AddStringToObject(root, "icon", "mdi:arrow-expand-vertical");

        cJSON_AddStringToObject(root, "availability_topic", lwt_topic);
        cJSON_AddStringToObject(root, "payload_available", "online");
        cJSON_AddStringToObject(root, "payload_not_available", "offline");

        cJSON_AddItemToObject(root, "device", create_device_info_json());
        publish_ha_discovery_entity("number", "target_height", root);
    }

    // 3b. Number entity (Min Height in cm)
    {
        cJSON *root = cJSON_CreateObject();
        snprintf(uid_buf, sizeof(uid_buf), "%s_min_height", device_id);
        cJSON_AddStringToObject(root, "name", "Desk Minimum Height");
        cJSON_AddStringToObject(root, "unique_id", uid_buf);

        snprintf(topic_buf, sizeof(topic_buf), "%s/min_height/set", topic_prefix);
        cJSON_AddStringToObject(root, "command_topic", topic_buf);

        snprintf(topic_buf, sizeof(topic_buf), "%s/min_height/state", topic_prefix);
        cJSON_AddStringToObject(root, "state_topic", topic_buf);

        cJSON_AddNumberToObject(root, "min", 30);
        cJSON_AddNumberToObject(root, "max", 100);
        cJSON_AddNumberToObject(root, "step", 1);
        cJSON_AddStringToObject(root, "unit_of_measurement", "cm");
        cJSON_AddStringToObject(root, "mode", "box");
        cJSON_AddStringToObject(root, "icon", "mdi:arrow-collapse-down");

        cJSON_AddStringToObject(root, "availability_topic", lwt_topic);
        cJSON_AddStringToObject(root, "payload_available", "online");
        cJSON_AddStringToObject(root, "payload_not_available", "offline");

        cJSON_AddItemToObject(root, "device", create_device_info_json());
        publish_ha_discovery_entity("number", "min_height", root);
    }

    // 3c. Number entity (Max Height in cm)
    {
        cJSON *root = cJSON_CreateObject();
        snprintf(uid_buf, sizeof(uid_buf), "%s_max_height", device_id);
        cJSON_AddStringToObject(root, "name", "Desk Maximum Height");
        cJSON_AddStringToObject(root, "unique_id", uid_buf);

        snprintf(topic_buf, sizeof(topic_buf), "%s/max_height/set", topic_prefix);
        cJSON_AddStringToObject(root, "command_topic", topic_buf);

        snprintf(topic_buf, sizeof(topic_buf), "%s/max_height/state", topic_prefix);
        cJSON_AddStringToObject(root, "state_topic", topic_buf);

        cJSON_AddNumberToObject(root, "min", 70);
        cJSON_AddNumberToObject(root, "max", 180);
        cJSON_AddNumberToObject(root, "step", 1);
        cJSON_AddStringToObject(root, "unit_of_measurement", "cm");
        cJSON_AddStringToObject(root, "mode", "box");
        cJSON_AddStringToObject(root, "icon", "mdi:arrow-collapse-up");

        cJSON_AddStringToObject(root, "availability_topic", lwt_topic);
        cJSON_AddStringToObject(root, "payload_available", "online");
        cJSON_AddStringToObject(root, "payload_not_available", "offline");

        cJSON_AddItemToObject(root, "device", create_device_info_json());
        publish_ha_discovery_entity("number", "max_height", root);
    }

    // 4. Button entity: Stop
    {
        cJSON *root = cJSON_CreateObject();
        snprintf(uid_buf, sizeof(uid_buf), "%s_btn_stop", device_id);
        cJSON_AddStringToObject(root, "name", "Desk Stop");
        cJSON_AddStringToObject(root, "unique_id", uid_buf);

        snprintf(topic_buf, sizeof(topic_buf), "%s/button/stop", topic_prefix);
        cJSON_AddStringToObject(root, "command_topic", topic_buf);
        cJSON_AddStringToObject(root, "icon", "mdi:stop-circle");

        cJSON_AddStringToObject(root, "availability_topic", lwt_topic);
        cJSON_AddStringToObject(root, "payload_available", "online");
        cJSON_AddStringToObject(root, "payload_not_available", "offline");

        cJSON_AddItemToObject(root, "device", create_device_info_json());
        publish_ha_discovery_entity("button", "stop", root);
    }

    // 5. Button and Number entities: Memory Presets 1 to 7
    for(uint8_t i = 1; i <= 7; i++) {
        // Preset Button
        {
            cJSON *root = cJSON_CreateObject();
            char obj_id[32];
            snprintf(obj_id, sizeof(obj_id), "preset%u", (unsigned int) i);

            snprintf(uid_buf, sizeof(uid_buf), "%s_btn_p%u", device_id, (unsigned int) i);
            cJSON_AddStringToObject(root, "unique_id", uid_buf);

            char name_buf[64];
            snprintf(name_buf, sizeof(name_buf), "Preset %u", (unsigned int) i);
            cJSON_AddStringToObject(root, "name", name_buf);

            snprintf(topic_buf, sizeof(topic_buf), "%s/button/memory%u", topic_prefix, (unsigned int) i);
            cJSON_AddStringToObject(root, "command_topic", topic_buf);

            char icon_buf[32];
            snprintf(icon_buf, sizeof(icon_buf), "mdi:numeric-%u-box", (unsigned int) i);
            cJSON_AddStringToObject(root, "icon", icon_buf);

            cJSON_AddStringToObject(root, "availability_topic", lwt_topic);
            cJSON_AddStringToObject(root, "payload_available", "online");
            cJSON_AddStringToObject(root, "payload_not_available", "offline");

            cJSON_AddItemToObject(root, "device", create_device_info_json());
            publish_ha_discovery_entity("button", obj_id, root);
        }

        // Preset Height Number (configurable in HA)
        {
            cJSON *root = cJSON_CreateObject();
            char obj_id[32];
            snprintf(obj_id, sizeof(obj_id), "preset%u_height", (unsigned int) i);

            snprintf(uid_buf, sizeof(uid_buf), "%s_preset_%u", device_id, (unsigned int) i);
            cJSON_AddStringToObject(root, "unique_id", uid_buf);

            char name_buf[64];
            snprintf(name_buf, sizeof(name_buf), "Desk Preset %u Height", (unsigned int) i);
            cJSON_AddStringToObject(root, "name", name_buf);

            snprintf(topic_buf, sizeof(topic_buf), "%s/preset/%u/set", topic_prefix, (unsigned int) i);
            cJSON_AddStringToObject(root, "command_topic", topic_buf);

            snprintf(topic_buf, sizeof(topic_buf), "%s/preset/%u/state", topic_prefix, (unsigned int) i);
            cJSON_AddStringToObject(root, "state_topic", topic_buf);

            cJSON_AddNumberToObject(root, "min", 30);
            cJSON_AddNumberToObject(root, "max", 180);
            cJSON_AddNumberToObject(root, "step", 1);
            cJSON_AddStringToObject(root, "unit_of_measurement", "cm");
            cJSON_AddStringToObject(root, "mode", "box");

            char icon_buf[32];
            snprintf(icon_buf, sizeof(icon_buf), "mdi:numeric-%u-box-outline", (unsigned int) i);
            cJSON_AddStringToObject(root, "icon", icon_buf);

            cJSON_AddStringToObject(root, "availability_topic", lwt_topic);
            cJSON_AddStringToObject(root, "payload_available", "online");
            cJSON_AddStringToObject(root, "payload_not_available", "offline");

            cJSON_AddItemToObject(root, "device", create_device_info_json());
            publish_ha_discovery_entity("number", obj_id, root);
        }
    }

#if defined(SENSORS_ON)
    // 6. Sensor entity: Temperature
    {
        cJSON *root = cJSON_CreateObject();
        snprintf(uid_buf, sizeof(uid_buf), "%s_temp", device_id);
        cJSON_AddStringToObject(root, "name", "Desk Temperature");
        cJSON_AddStringToObject(root, "unique_id", uid_buf);

        snprintf(topic_buf, sizeof(topic_buf), "%s/sensor/temperature", topic_prefix);
        cJSON_AddStringToObject(root, "state_topic", topic_buf);
        cJSON_AddStringToObject(root, "device_class", "temperature");
        cJSON_AddStringToObject(root, "state_class", "measurement");

        char temp_scale_str[8] = "°C";
        char scale_char = get_temperature_scale();
        if(scale_char == SCALE_FAHRENHEIT) {
            strcpy(temp_scale_str, "°F");
        } else if(scale_char == SCALE_KELVIN) {
            strcpy(temp_scale_str, "K");
        }
        cJSON_AddStringToObject(root, "unit_of_measurement", temp_scale_str);

        cJSON_AddStringToObject(root, "availability_topic", lwt_topic);
        cJSON_AddStringToObject(root, "payload_available", "online");
        cJSON_AddStringToObject(root, "payload_not_available", "offline");

        cJSON_AddItemToObject(root, "device", create_device_info_json());
        publish_ha_discovery_entity("sensor", "temperature", root);
    }

    // 7. Sensor entity: Humidity
    {
        cJSON *root = cJSON_CreateObject();
        snprintf(uid_buf, sizeof(uid_buf), "%s_humidity", device_id);
        cJSON_AddStringToObject(root, "name", "Desk Humidity");
        cJSON_AddStringToObject(root, "unique_id", uid_buf);

        snprintf(topic_buf, sizeof(topic_buf), "%s/sensor/humidity", topic_prefix);
        cJSON_AddStringToObject(root, "state_topic", topic_buf);
        cJSON_AddStringToObject(root, "device_class", "humidity");
        cJSON_AddStringToObject(root, "state_class", "measurement");
        cJSON_AddStringToObject(root, "unit_of_measurement", "%");

        cJSON_AddStringToObject(root, "availability_topic", lwt_topic);
        cJSON_AddStringToObject(root, "payload_available", "online");
        cJSON_AddStringToObject(root, "payload_not_available", "offline");

        cJSON_AddItemToObject(root, "device", create_device_info_json());
        publish_ha_discovery_entity("sensor", "humidity", root);
    }

    // 8. Sensor entity: CO2
    {
        cJSON *root = cJSON_CreateObject();
        snprintf(uid_buf, sizeof(uid_buf), "%s_co2", device_id);
        cJSON_AddStringToObject(root, "name", "Desk CO₂");
        cJSON_AddStringToObject(root, "unique_id", uid_buf);

        snprintf(topic_buf, sizeof(topic_buf), "%s/sensor/co2", topic_prefix);
        cJSON_AddStringToObject(root, "state_topic", topic_buf);
        cJSON_AddStringToObject(root, "device_class", "carbon_dioxide");
        cJSON_AddStringToObject(root, "state_class", "measurement");
        cJSON_AddStringToObject(root, "unit_of_measurement", "ppm");

        cJSON_AddStringToObject(root, "availability_topic", lwt_topic);
        cJSON_AddStringToObject(root, "payload_available", "online");
        cJSON_AddStringToObject(root, "payload_not_available", "offline");

        cJSON_AddItemToObject(root, "device", create_device_info_json());
        publish_ha_discovery_entity("sensor", "co2", root);
    }

    // 9. Sensor entity: CO2 Peak
    {
        cJSON *root = cJSON_CreateObject();
        snprintf(uid_buf, sizeof(uid_buf), "%s_co2_peak", device_id);
        cJSON_AddStringToObject(root, "name", "Desk CO₂ Peak");
        cJSON_AddStringToObject(root, "unique_id", uid_buf);

        snprintf(topic_buf, sizeof(topic_buf), "%s/sensor/co2_peak", topic_prefix);
        cJSON_AddStringToObject(root, "state_topic", topic_buf);
        cJSON_AddStringToObject(root, "device_class", "carbon_dioxide");
        cJSON_AddStringToObject(root, "state_class", "measurement");
        cJSON_AddStringToObject(root, "unit_of_measurement", "ppm");

        cJSON_AddStringToObject(root, "availability_topic", lwt_topic);
        cJSON_AddStringToObject(root, "payload_available", "online");
        cJSON_AddStringToObject(root, "payload_not_available", "offline");

        cJSON_AddItemToObject(root, "device", create_device_info_json());
        publish_ha_discovery_entity("sensor", "co2_peak", root);
    }

    // 10. Sensor entity: Air Quality
    {
        cJSON *root = cJSON_CreateObject();
        snprintf(uid_buf, sizeof(uid_buf), "%s_air_quality", device_id);
        cJSON_AddStringToObject(root, "name", "Desk Air Quality");
        cJSON_AddStringToObject(root, "unique_id", uid_buf);

        snprintf(topic_buf, sizeof(topic_buf), "%s/sensor/air_quality", topic_prefix);
        cJSON_AddStringToObject(root, "state_topic", topic_buf);
        cJSON_AddStringToObject(root, "icon", "mdi:air-filter");

        cJSON_AddStringToObject(root, "availability_topic", lwt_topic);
        cJSON_AddStringToObject(root, "payload_available", "online");
        cJSON_AddStringToObject(root, "payload_not_available", "offline");

        cJSON_AddItemToObject(root, "device", create_device_info_json());
        publish_ha_discovery_entity("sensor", "air_quality", root);
    }
#endif
}

static void handle_mqtt_command(const char *topic, const char *data) {
    char expected_topic[128];

    // Cover command (OPEN, CLOSE, STOP)
    snprintf(expected_topic, sizeof(expected_topic), "%s/cover/set", topic_prefix);
    if(strcmp(topic, expected_topic) == 0) {
        if(strcasecmp(data, "OPEN") == 0) {
            ESP_LOGI(MQTT_TAG, "Command received: OPEN");
            desk_set_target_height(desk_max_height);
        } else if(strcasecmp(data, "CLOSE") == 0) {
            ESP_LOGI(MQTT_TAG, "Command received: CLOSE");
            desk_set_target_height(desk_min_height);
        } else if(strcasecmp(data, "STOP") == 0) {
            ESP_LOGI(MQTT_TAG, "Command received: STOP");
            desk_stop_movement();
        }
        return;
    }

    // Cover set position (0-100)
    snprintf(expected_topic, sizeof(expected_topic), "%s/cover/set_position", topic_prefix);
    if(strcmp(topic, expected_topic) == 0) {
        int pos = atoi(data);
        if(pos >= 0 && pos <= 100) {
            ESP_LOGI(MQTT_TAG, "Command received: SET_POSITION %d%%", pos);
            desk_set_target_percentage((uint8_t) pos);
        }
        return;
    }

    // Target height set (cm)
    snprintf(expected_topic, sizeof(expected_topic), "%s/target_height/set", topic_prefix);
    if(strcmp(topic, expected_topic) == 0) {
        int height = atoi(data);
        if(height >= desk_min_height && height <= desk_max_height) {
            ESP_LOGI(MQTT_TAG, "Command received: TARGET_HEIGHT %dcm", height);
            desk_set_target_height((uint8_t) height);
        } else {
            ESP_LOGW(MQTT_TAG, "Target height %dcm out of range (%d-%d)!", height, desk_min_height, desk_max_height);
        }
        return;
    }

    // Min height set (cm)
    snprintf(expected_topic, sizeof(expected_topic), "%s/min_height/set", topic_prefix);
    if(strcmp(topic, expected_topic) == 0) {
        int min_h = atoi(data);
        ESP_LOGI(MQTT_TAG, "Command received: SET MIN HEIGHT %dcm", min_h);
        desk_set_min_height((uint8_t) min_h);
        return;
    }

    // Max height set (cm)
    snprintf(expected_topic, sizeof(expected_topic), "%s/max_height/set", topic_prefix);
    if(strcmp(topic, expected_topic) == 0) {
        int max_h = atoi(data);
        ESP_LOGI(MQTT_TAG, "Command received: SET MAX HEIGHT %dcm", max_h);
        desk_set_max_height((uint8_t) max_h);
        return;
    }

    // Preset height config set (e.g. dreamdesk/preset/1/set or dreamdesk/preset1/set)
    for(uint8_t i = 1; i <= 7; i++) {
        char p_topic1[128], p_topic2[128];
        snprintf(p_topic1, sizeof(p_topic1), "%s/preset/%u/set", topic_prefix, (unsigned int) i);
        snprintf(p_topic2, sizeof(p_topic2), "%s/preset%u/set", topic_prefix, (unsigned int) i);
        if(strcmp(topic, p_topic1) == 0 || strcmp(topic, p_topic2) == 0) {
            int p_val = atoi(data);
            ESP_LOGI(MQTT_TAG, "Command received: SET PRESET %u HEIGHT %dcm", (unsigned int) i, p_val);
            desk_set_preset_height(i, (uint8_t) p_val);
            return;
        }
    }

    // General preset drive command (1-7)
    snprintf(expected_topic, sizeof(expected_topic), "%s/preset/set", topic_prefix);
    if(strcmp(topic, expected_topic) == 0) {
        int preset = atoi(data);
        if(preset >= 1 && preset <= 7) {
            ESP_LOGI(MQTT_TAG, "Command received: DRIVE TO PRESET %d (%dcm)", preset, desk_preset_heights[preset - 1]);
            desk_set_target_height(desk_preset_heights[preset - 1]);
        } else {
            ESP_LOGW(MQTT_TAG, "Invalid preset %d!", preset);
        }
        return;
    }

    // Stop button
    snprintf(expected_topic, sizeof(expected_topic), "%s/button/stop", topic_prefix);
    if(strcmp(topic, expected_topic) == 0) {
        ESP_LOGI(MQTT_TAG, "Button pressed: STOP");
        desk_stop_movement();
        return;
    }

    // Preset buttons (memory 1 to 7)
    for(uint8_t i = 1; i <= 7; i++) {
        char b_topic1[128], b_topic2[128];
        snprintf(b_topic1, sizeof(b_topic1), "%s/button/memory%u", topic_prefix, (unsigned int) i);
        snprintf(b_topic2, sizeof(b_topic2), "%s/button/preset%u", topic_prefix, (unsigned int) i);
        if(strcmp(topic, b_topic1) == 0 || strcmp(topic, b_topic2) == 0) {
            ESP_LOGI(MQTT_TAG, "Button pressed: Preset %u (driving to %dcm)", (unsigned int) i, desk_preset_heights[i - 1]);
            desk_set_target_height(desk_preset_heights[i - 1]);
            return;
        }
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t) event_data;
    char sub_topic[128];

    switch((esp_mqtt_event_id_t) event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(MQTT_TAG, "MQTT connected!");
            mqtt_connected = true;

            // Publish availability
            esp_mqtt_client_publish(mqtt_client, lwt_topic, "online", 0, 1, 1);

            // Publish Auto Discovery payloads
            publish_ha_discovery();

            // Subscribe to command topics
            snprintf(sub_topic, sizeof(sub_topic), "%s/cover/set", topic_prefix);
            esp_mqtt_client_subscribe(mqtt_client, sub_topic, 1);

            snprintf(sub_topic, sizeof(sub_topic), "%s/cover/set_position", topic_prefix);
            esp_mqtt_client_subscribe(mqtt_client, sub_topic, 1);

            snprintf(sub_topic, sizeof(sub_topic), "%s/target_height/set", topic_prefix);
            esp_mqtt_client_subscribe(mqtt_client, sub_topic, 1);

            snprintf(sub_topic, sizeof(sub_topic), "%s/min_height/set", topic_prefix);
            esp_mqtt_client_subscribe(mqtt_client, sub_topic, 1);

            snprintf(sub_topic, sizeof(sub_topic), "%s/max_height/set", topic_prefix);
            esp_mqtt_client_subscribe(mqtt_client, sub_topic, 1);

            snprintf(sub_topic, sizeof(sub_topic), "%s/preset/#", topic_prefix);
            esp_mqtt_client_subscribe(mqtt_client, sub_topic, 1);

            snprintf(sub_topic, sizeof(sub_topic), "%s/button/#", topic_prefix);
            esp_mqtt_client_subscribe(mqtt_client, sub_topic, 1);

            // Invalidate cache so fresh state is published immediately
            last_published_height = 0xFF;
            last_published_target_height = 0xFF;
            last_published_percentage = 0xFF;
            last_published_min_height = 0xFF;
            last_published_max_height = 0xFF;
            for(int i = 0; i < 7; i++) {
                last_published_presets[i] = 0xFF;
            }
            #if defined(SENSORS_ON)
            last_published_temp = -999.0f;
            last_published_hum = -999.0f;
            last_published_co2 = -999.0f;
            last_published_co2_peak = -999.0f;
            last_published_aq = UNKNOWN;
            #endif
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(MQTT_TAG, "MQTT disconnected!");
            mqtt_connected = false;
            break;

        case MQTT_EVENT_DATA: {
            char topic_buf[128] = {0};
            char data_buf[128] = {0};

            int tlen = event->topic_len < (sizeof(topic_buf) - 1) ? event->topic_len : (sizeof(topic_buf) - 1);
            memcpy(topic_buf, event->topic, tlen);
            topic_buf[tlen] = '\0';

            int dlen = event->data_len < (sizeof(data_buf) - 1) ? event->data_len : (sizeof(data_buf) - 1);
            memcpy(data_buf, event->data, dlen);
            data_buf[dlen] = '\0';

            ESP_LOGI(MQTT_TAG, "MQTT received topic: %s, data: %s", topic_buf, data_buf);
            handle_mqtt_command(topic_buf, data_buf);
            break;
        }

        case MQTT_EVENT_ERROR:
            ESP_LOGE(MQTT_TAG, "MQTT error event!");
            break;

        default:
            break;
    }
}

void home_task(void *arg) {
    esp_log_level_set(MQTT_TAG, ESP_LOG_INFO);
    ESP_LOGI(MQTT_TAG, "Initializing Home Assistant MQTT integration...");

    // Derive device ID from WiFi MAC address
    uint8_t mac[6] = {0};
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(device_id, sizeof(device_id), "dreamdesk_%02x%02x%02x%02x%02x%02x",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    // Load custom topic/discovery prefix if configured in NVS
    char custom_prefix[sizeof(topic_prefix)] = {0};
    if(nvs_mqtt_get_str("mqtt_prefix", custom_prefix, sizeof(custom_prefix)) == ESP_OK && strlen(custom_prefix) > 0) {
        strncpy(topic_prefix, custom_prefix, sizeof(topic_prefix));
    }

    char custom_discovery[sizeof(discovery_prefix)] = {0};
    if(nvs_mqtt_get_str("mqtt_discovery", custom_discovery, sizeof(custom_discovery)) == ESP_OK && strlen(custom_discovery) > 0) {
        strncpy(discovery_prefix, custom_discovery, sizeof(discovery_prefix));
    }

    snprintf(lwt_topic, sizeof(lwt_topic), "%s/status", topic_prefix);

    // Read MQTT Broker credentials from NVS or use default
    char mqtt_uri[128] = MQTT_DEFAULT_BROKER_URI;
    char mqtt_user[64] = {0};
    char mqtt_pass[64] = {0};

    char nvs_uri[sizeof(mqtt_uri)] = {0};
    if(nvs_mqtt_get_str("mqtt_uri", nvs_uri, sizeof(nvs_uri)) == ESP_OK && strlen(nvs_uri) > 0) {
        strncpy(mqtt_uri, nvs_uri, sizeof(mqtt_uri));
    }

    nvs_mqtt_get_str("mqtt_user", mqtt_user, sizeof(mqtt_user));
    nvs_mqtt_get_str("mqtt_password", mqtt_pass, sizeof(mqtt_pass));

    ESP_LOGI(MQTT_TAG, "MQTT Broker URI: %s", mqtt_uri);
    ESP_LOGI(MQTT_TAG, "Device ID: %s, Topic Prefix: %s", device_id, topic_prefix);

    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = mqtt_uri,
        .client_id = device_id,
        .lwt_topic = lwt_topic,
        .lwt_msg = "offline",
        .lwt_qos = 1,
        .lwt_retain = 1,
        .keepalive = 60,
    };

    if(strlen(mqtt_user) > 0) {
        mqtt_cfg.username = mqtt_user;
    }
    if(strlen(mqtt_pass) > 0) {
        mqtt_cfg.password = mqtt_pass;
    }

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL));
    ESP_ERROR_CHECK(esp_mqtt_client_start(mqtt_client));

    uint32_t loop_counter = 0;
    char payload_buf[32];
    char pub_topic[128];

    for(;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        loop_counter++;

        if(!mqtt_connected) {
            continue;
        }

        bool force_periodic = (loop_counter % 30 == 0);

        // Publish Desk Height & Position
        if(current_desk_height != 0xFF) {
            if(force_periodic || current_desk_height != last_published_height) {
                last_published_height = current_desk_height;
                snprintf(pub_topic, sizeof(pub_topic), "%s/height/state", topic_prefix);
                snprintf(payload_buf, sizeof(payload_buf), "%d", current_desk_height);
                esp_mqtt_client_publish(mqtt_client, pub_topic, payload_buf, 0, 1, 0);
            }

            if(force_periodic || desk_percentage != last_published_percentage) {
                last_published_percentage = desk_percentage;
                snprintf(pub_topic, sizeof(pub_topic), "%s/cover/position", topic_prefix);
                snprintf(payload_buf, sizeof(payload_buf), "%d", desk_percentage);
                esp_mqtt_client_publish(mqtt_client, pub_topic, payload_buf, 0, 1, 0);
            }
        }

        if(target_desk_height != 0xFF) {
            if(force_periodic || target_desk_height != last_published_target_height) {
                last_published_target_height = target_desk_height;
                snprintf(pub_topic, sizeof(pub_topic), "%s/target_height/state", topic_prefix);
                snprintf(payload_buf, sizeof(payload_buf), "%d", target_desk_height);
                esp_mqtt_client_publish(mqtt_client, pub_topic, payload_buf, 0, 1, 0);
            }
        }

        // Publish Min/Max limits state
        if(force_periodic || desk_min_height != last_published_min_height) {
            last_published_min_height = desk_min_height;
            snprintf(pub_topic, sizeof(pub_topic), "%s/min_height/state", topic_prefix);
            snprintf(payload_buf, sizeof(payload_buf), "%d", desk_min_height);
            esp_mqtt_client_publish(mqtt_client, pub_topic, payload_buf, 0, 1, 0);
        }

        if(force_periodic || desk_max_height != last_published_max_height) {
            last_published_max_height = desk_max_height;
            snprintf(pub_topic, sizeof(pub_topic), "%s/max_height/state", topic_prefix);
            snprintf(payload_buf, sizeof(payload_buf), "%d", desk_max_height);
            esp_mqtt_client_publish(mqtt_client, pub_topic, payload_buf, 0, 1, 0);
        }

        // Publish Preset heights state
        for(uint8_t i = 1; i <= 7; i++) {
            if(force_periodic || desk_preset_heights[i - 1] != last_published_presets[i - 1]) {
                last_published_presets[i - 1] = desk_preset_heights[i - 1];
                snprintf(pub_topic, sizeof(pub_topic), "%s/preset/%u/state", topic_prefix, (unsigned int) i);
                snprintf(payload_buf, sizeof(payload_buf), "%d", desk_preset_heights[i - 1]);
                esp_mqtt_client_publish(mqtt_client, pub_topic, payload_buf, 0, 1, 0);
            }
        }

#if defined(SENSORS_ON)
        float current_temp = round(get_current_temperature() * 10.0f) / 10.0f;
        float current_hum = round(get_current_relative_humidity() * 10.0f) / 10.0f;
        float current_co2 = round(get_co2_level());
        float current_co2_peak = round(get_co2_peak_level());
        enum air_quality_t current_aq = get_air_quality();

        if(current_temp > 0.0f || current_hum > 0.0f || current_co2 > 0.0f) {
            if(force_periodic || fabsf(current_temp - last_published_temp) >= 0.1f) {
                last_published_temp = current_temp;
                snprintf(pub_topic, sizeof(pub_topic), "%s/sensor/temperature", topic_prefix);
                snprintf(payload_buf, sizeof(payload_buf), "%.1f", current_temp);
                esp_mqtt_client_publish(mqtt_client, pub_topic, payload_buf, 0, 1, 0);
            }

            if(force_periodic || fabsf(current_hum - last_published_hum) >= 0.5f) {
                last_published_hum = current_hum;
                snprintf(pub_topic, sizeof(pub_topic), "%s/sensor/humidity", topic_prefix);
                snprintf(payload_buf, sizeof(payload_buf), "%.1f", current_hum);
                esp_mqtt_client_publish(mqtt_client, pub_topic, payload_buf, 0, 1, 0);
            }

            if(force_periodic || fabsf(current_co2 - last_published_co2) >= 1.0f) {
                last_published_co2 = current_co2;
                snprintf(pub_topic, sizeof(pub_topic), "%s/sensor/co2", topic_prefix);
                snprintf(payload_buf, sizeof(payload_buf), "%.0f", current_co2);
                esp_mqtt_client_publish(mqtt_client, pub_topic, payload_buf, 0, 1, 0);
            }

            if(force_periodic || fabsf(current_co2_peak - last_published_co2_peak) >= 1.0f) {
                last_published_co2_peak = current_co2_peak;
                snprintf(pub_topic, sizeof(pub_topic), "%s/sensor/co2_peak", topic_prefix);
                snprintf(payload_buf, sizeof(payload_buf), "%.0f", current_co2_peak);
                esp_mqtt_client_publish(mqtt_client, pub_topic, payload_buf, 0, 1, 0);
            }

            if(force_periodic || current_aq != last_published_aq) {
                last_published_aq = current_aq;
                snprintf(pub_topic, sizeof(pub_topic), "%s/sensor/air_quality", topic_prefix);
                const char *aq_str = air_quality_to_str(current_aq);
                esp_mqtt_client_publish(mqtt_client, pub_topic, aq_str, 0, 1, 0);
            }
        }
#endif
    }
}
