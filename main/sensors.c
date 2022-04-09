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

void sensors_task(void *arg) {

    i2c_config_t i2c_config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_MASTER_SCL,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ
    };

    ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER_NUM, i2c_config.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0));
    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &i2c_config));

    for(;;) {
        ESP_LOGW(SENSORS_TAG, "Sensors probe");

        uint8_t data[9];
        uint8_t size = 9;
        uint16_t reg_addr = SCD41_START_PERIODIC_MEASUREMENT;

        measurements_t measurements;
        uint8_t ms = sizeof(measurements);

        // start_periodic_measurement

        //i2c_master_write_read_device(I2C_MASTER_NUM, SCD41_SENSOR_ADDR, &reg_addr, 2, data, 9, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);

        // read_measurement

        //reg_addr = SCD41_READ_MEASUREMENT;
        //i2c_master_write_read_device(I2C_MASTER_NUM, SCD41_SENSOR_ADDR, &reg_addr, 2, data, 9, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
        //vTaskDelay(100);
        //ESP_LOG_BUFFER_HEX_LEVEL(SENSORS_TAG, data, 9, ESP_LOG_ERROR);

        // stop_periodic_measurement
        
        //reg_addr = SCD41_STOP_PERIODIC_MEASUREMENT;
        //i2c_master_write_read_device(I2C_MASTER_NUM, SCD41_SENSOR_ADDR, &reg_addr, 2, data, 9, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);


         // start_periodic_measurement
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (SCD41_SENSOR_ADDR << 1) | I2C_MASTER_WRITE, I2C_ACK_CHECK_EN);

        //i2c_master_write(cmd, data_wr, size, ACK_CHECK_EN);
        i2c_master_write_byte(cmd, 0x21, I2C_ACK_CHECK_EN);
        i2c_master_write_byte(cmd, 0xB1, I2C_ACK_CHECK_EN);

        i2c_master_stop(cmd);
        i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);

        i2c_cmd_link_delete(cmd);

        vTaskDelay(100);

        // read_measurement

        cmd = i2c_cmd_link_create();
    
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (SCD41_SENSOR_ADDR << 1) | I2C_MASTER_WRITE, I2C_ACK_CHECK_EN);

        //i2c_master_write(cmd, data_wr, size, ACK_CHECK_EN);
        i2c_master_write_byte(cmd, 0xEC, I2C_ACK_CHECK_EN);
        i2c_master_write_byte(cmd, 0x05, I2C_ACK_CHECK_EN);

        i2c_master_start(cmd);

        i2c_master_write_byte(cmd, ( SCD41_SENSOR_ADDR << 1 ) | I2C_MASTER_READ, I2C_ACK_CHECK_EN);
       
        i2c_master_read(cmd, data, size - 1, I2C_ACK_VAL);
        i2c_master_read_byte(cmd, data + size - 1, I2C_NACK_VAL);

        i2c_master_stop(cmd);
        i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);

        i2c_cmd_link_delete(cmd);

        vTaskDelay(100);
        ESP_LOG_BUFFER_HEX_LEVEL(SENSORS_TAG, data, size, ESP_LOG_ERROR);

        // stop_periodic_measurement

        cmd = i2c_cmd_link_create();
    
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (SCD41_SENSOR_ADDR << 1) | I2C_MASTER_WRITE, I2C_ACK_CHECK_EN);

        //i2c_master_write(cmd, data_wr, size, ACK_CHECK_EN);
        i2c_master_write_byte(cmd, 0x3F, I2C_ACK_CHECK_EN);
        i2c_master_write_byte(cmd, 0x86, I2C_ACK_CHECK_EN);

        i2c_master_stop(cmd);
        i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);

        i2c_cmd_link_delete(cmd);

        //ESP_ERROR_CHECK(i2c_master_write_read_device(I2C_MASTER_NUM, SCD41_SENSOR_ADDR, &reg_addr, 1, data, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));
        ESP_LOG_BUFFER_HEX_LEVEL(SENSORS_TAG, data, 9, ESP_LOG_ERROR);

        vTaskDelay((1000*60) / portTICK_PERIOD_MS);
    }
}