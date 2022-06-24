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
#include "driver/i2c.h"
#include "esp_log.h"

static const char *SENSORS_TAG = "sensors";

char scale = SCALE_CELCIUS;
float temperature = 0.0;
float humidity = 0.0;
float co2_level = 0.0;
float co2_peak_level = 0.0;
enum air_quality_t air_quality = UNKNOWN;

uint8_t scd41_start_periodic_measurement[] = {0x21, 0xB1};
uint8_t scd41_read_measurement[]           = {0xEC, 0x05};
uint8_t scd41_stop_periodic_measurement[]  = {0x3F, 0x86};

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

void start_periodic_measurement() {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SCD41_SENSOR_ADDR << 1) | I2C_MASTER_WRITE, I2C_ACK_CHECK_EN);

    i2c_master_write(cmd, scd41_start_periodic_measurement, sizeof(scd41_start_periodic_measurement), I2C_ACK_CHECK_EN);

    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);

    i2c_cmd_link_delete(cmd);
}

void read_measurement(uint8_t *measurements, uint8_t size) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SCD41_SENSOR_ADDR << 1) | I2C_MASTER_WRITE, I2C_ACK_CHECK_EN);

    i2c_master_write(cmd, scd41_read_measurement, sizeof(scd41_read_measurement), I2C_ACK_CHECK_EN);

    i2c_master_start(cmd);

    i2c_master_write_byte(cmd, ( SCD41_SENSOR_ADDR << 1 ) | I2C_MASTER_READ, I2C_ACK_CHECK_EN);

    i2c_master_read(cmd, measurements, size - 1, I2C_ACK_VAL);
    i2c_master_read_byte(cmd, measurements + size - 1, I2C_NACK_VAL);

    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);

    i2c_cmd_link_delete(cmd);
}

void stop_periodic_measurement() {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SCD41_SENSOR_ADDR << 1) | I2C_MASTER_WRITE, I2C_ACK_CHECK_EN);

    i2c_master_write(cmd, scd41_stop_periodic_measurement, sizeof(scd41_stop_periodic_measurement), I2C_ACK_CHECK_EN);

    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);

    i2c_cmd_link_delete(cmd);
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
    ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER_NUM, i2c_config.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0));
    
    esp_log_level_set(SENSORS_TAG, ESP_LOG_INFO);

    #if defined(SENSORS_SCALE_F)
    scale = SCALE_FAHRENHEIT;
    #elif defined(SENSORS_SCALE_K)
    scale = SCALE_KELVIN;
    #endif

    for(;;) {
        measurements_t measurements = {
            .co2 = {0x00, 0x00},
            .co2_crc = 0x00,
            .temperature = {0x00, 0x00},
            .temperature_crc = 0x00,
            .humidity = {0x00, 0x00},
            .humidity_crc = 0x00
        };

        start_periodic_measurement();

        float average_temperature = 0.0;
        float average_humidity = 0.0;
        float average_co2_level = 0.0;

        for(uint8_t i = 0; i < MEASUREMENT_COUNT; i++) {
            vTaskDelay(UPDATE_INTERVAL / portTICK_PERIOD_MS);

            read_measurement((uint8_t*) &measurements, sizeof(measurements));

            average_co2_level += (measurements.co2.high << 8) + measurements.co2.low;
            average_temperature += (175 * (((measurements.temperature.high << 8) + measurements.temperature.low) / 65536.0)) - 45.0;
            average_humidity += 100 * ((measurements.humidity.high << 8) + measurements.humidity.low) / 65536.0;

            ESP_LOG_BUFFER_HEX_LEVEL(SENSORS_TAG, &measurements, sizeof(measurements), ESP_LOG_DEBUG);

            if(average_co2_level == 0x0000) {
                break;
            }
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

        stop_periodic_measurement();
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
        vTaskDelay(SLEEP_INTERVAL_15_MIN / portTICK_PERIOD_MS);
    }
}
