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
#include "sensors.h"
#include "esp_log.h"
#include "scd4x.h"
#include "driver/i2c.h"

static const char *SENSORS_TAG = "sensors";

char scale = SCALE_CELCIUS;
float temperature = 0.0;
float humidity = 0.0;
float co2_level = 0.0;
float co2_peak_level = 0.0;
enum air_quality_t air_quality = UNKNOWN;

char get_temperature_scale() {
    return scale;
}

float get_current_temperature() {
    return temperature;
}

float get_current_relative_humidity() {
    return humidity;
}

float get_co2_level() {
    return co2_level;
}

float get_co2_peak_level() {
    return co2_peak_level;
}

void set_co2_peak_level(float co2_level) {
    co2_peak_level = co2_level > co2_peak_level ? co2_level : co2_peak_level;
}

enum air_quality_t get_air_quality() {
    return air_quality;
}

void set_air_quality(float co2_level) {
    if(co2_level <= CO2_LEVEL_UNKNOWN) {
        air_quality = UNKNOWN;
    } else if(co2_level <= CO2_LEVEL_EXCELLENT) {
        air_quality = EXCELLENT;
    } else if(co2_level <= CO2_LEVEL_GOOD) {
        air_quality = GOOD;
    } else if(co2_level <= CO2_LEVEL_FAIR) {
        air_quality = FAIR;
    } else if(co2_level <= CO2_LEVEL_INFERIOR) {
        air_quality = INFERIOR;
    } else {
        air_quality = POOR;
    }
}

void sensors_task(void *arg) {
    i2c_config_t i2c_config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_MASTER_SCL,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ
    };

    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &i2c_config));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER_NUM, i2c_config.mode,
                                       I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0));

    esp_log_level_set(SENSORS_TAG, ESP_LOG_INFO);

    #if defined(SENSORS_SCALE_F)
    scale = SCALE_FAHRENHEIT;
    #elif defined(SENSORS_SCALE_K)
    scale = SCALE_KELVIN;
    #endif

    vTaskDelay(SENSORS_INIT_DELAY / portTICK_PERIOD_MS);
    ESP_LOGI(SENSORS_TAG, "Sensor serial number 0x%012llX", scd4x_get_serial_number());

    vTaskDelay(SENSORS_INIT_DELAY / portTICK_PERIOD_MS);
    float temperature_offset = scd4x_get_temperature_offset();

    vTaskDelay(SENSORS_INIT_DELAY / portTICK_PERIOD_MS);
    uint16_t sensor_altitude = scd4x_get_sensor_altitude();

    if(temperature_offset != SCD41_READ_ERROR && sensor_altitude != SCD41_READ_ERROR) {

        if(temperature_offset != TEMPERATURE_OFFSET) {
            ESP_LOGW(SENSORS_TAG, "Temperature offset calibration from %.1f °%c to %.1f °%c",
                     temperature_offset, scale, TEMPERATURE_OFFSET, scale);

            vTaskDelay(SENSORS_INIT_DELAY / portTICK_PERIOD_MS);
            ESP_ERROR_CHECK_WITHOUT_ABORT(scd4x_set_temperature_offset(TEMPERATURE_OFFSET));

            vTaskDelay(SENSORS_INIT_DELAY / portTICK_PERIOD_MS);
            ESP_ERROR_CHECK_WITHOUT_ABORT(scd4x_persist_settings());

            vTaskDelay(SENSORS_INIT_DELAY / portTICK_PERIOD_MS);
            temperature_offset = scd4x_get_temperature_offset();
        }

        if(sensor_altitude != SENSOR_ALTITUDE) {
            ESP_LOGW(SENSORS_TAG, "Sensor altitude calibration from %d m to %d m",
                     sensor_altitude, SENSOR_ALTITUDE);

            vTaskDelay(SENSORS_INIT_DELAY / portTICK_PERIOD_MS);
            ESP_ERROR_CHECK_WITHOUT_ABORT(scd4x_set_sensor_altitude(SENSOR_ALTITUDE));

            vTaskDelay(SENSORS_INIT_DELAY / portTICK_PERIOD_MS);
            ESP_ERROR_CHECK_WITHOUT_ABORT(scd4x_persist_settings());

            vTaskDelay(SENSORS_INIT_DELAY / portTICK_PERIOD_MS);
            sensor_altitude = scd4x_get_sensor_altitude();
        }
        ESP_LOGI(SENSORS_TAG, "Temperature offset %.1f °%c - Sensor altitude %d %s",
                 temperature_offset, scale, sensor_altitude, scale == SCALE_CELCIUS ? "m" : "ft");
    } else {
        ESP_LOGE(SENSORS_TAG, "Sensor offset/altitude read error!");
    }

    vTaskDelay(SENSORS_INIT_DELAY / portTICK_PERIOD_MS);

    for(;;) {
        scd4x_start_periodic_measurement();

        uint16_t average_co2_level = 0.0;
        float average_temperature = 0.0;
        float average_humidity = 0.0;

        for(uint8_t i = 0; i < MEASUREMENT_COUNT + 2; i++) {
            scd4x_sensors_values_t sensors_values = {
                .co2 = 0x00,
                .temperature = 0x00,
                .humidity = 0x00
            };
            vTaskDelay(READ_SAMPLES_DELAY / portTICK_PERIOD_MS);

            if(scd4x_read_measurement(&sensors_values) != ESP_OK) {
                ESP_LOGE(SENSORS_TAG, "Sensors read measurement error!");
                break;
            }

            // Discard the first two sensor read outputs
            if(i < 2) {
                continue;
            }

            ESP_LOGD(SENSORS_TAG, "CO₂ %4d ppm - Temperature %2.1f °%c - Humidity %2.1f%%",
                     sensors_values.co2, sensors_values.temperature, scale, sensors_values.humidity);

            average_co2_level += sensors_values.co2;
            average_temperature += sensors_values.temperature;
            average_humidity += sensors_values.humidity;

            ESP_LOG_BUFFER_HEX_LEVEL(SENSORS_TAG, &sensors_values, sizeof(sensors_values), ESP_LOG_DEBUG);
        }

        temperature = (average_temperature / MEASUREMENT_COUNT);
        humidity = (average_humidity / MEASUREMENT_COUNT);
        co2_level = (average_co2_level / MEASUREMENT_COUNT);
        set_air_quality(co2_level);
        set_co2_peak_level(co2_level);

        #if defined(SENSORS_SCALE_F)
        temperature = FAHRENHEIT(temperature);
        #elif defined(SENSORS_SCALE_K)
        temperature = KELVIN(temperature);
        #endif

        scd4x_stop_periodic_measurement();
        esp_log_level_t air_quality_level = ESP_LOG_ERROR;

        if(air_quality == UNKNOWN) {
            ESP_LOGE(SENSORS_TAG, "Sensors read measurement error!");
        } else {
            if(air_quality < FAIR) {
                air_quality_level = ESP_LOG_INFO;
            } else if(air_quality < POOR) {
                air_quality_level = ESP_LOG_WARN;
            }

            ESP_LOG_LEVEL(air_quality_level, SENSORS_TAG, "CO₂ %4.0f ppm - Temperature %2.1f °%c - Humidity %2.1f%%",
                          co2_level, temperature, scale, humidity);
        }
        vTaskDelay(SLEEP_DELAY / portTICK_PERIOD_MS);
    }
}
