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
#include <stdio.h>

#define I2C_MASTER_SDA                      (GPIO_NUM_6)
#define I2C_MASTER_SCL                      (GPIO_NUM_7)
#define I2C_MASTER_RX_BUF_DISABLE           (0)
#define I2C_MASTER_TX_BUF_DISABLE           (0)
#define I2C_MASTER_FREQ_HZ                  (100000)
#define I2C_MASTER_TIMEOUT_MS               (1000)
#define I2C_MASTER_NUM                      (0)
#define I2C_ACK_CHECK_DIS                   (0x00)
#define I2C_ACK_CHECK_EN                    (0x01)
#define I2C_ACK_VAL                         (0x00)
#define I2C_NACK_VAL                        (0x01)
#define SCD41_SENSOR_ADDR                   (0x62)
#define SCD41_START_PERIODIC_MEASUREMENT    (0x21B1)
#define SCD41_READ_MEASUREMENT              (0xEC05)
#define SCD41_STOP_PERIODIC_MEASUREMENT     (0x3F86)
#define MEASUREMENT_COUNT                   (0x05)
#define SLEEP_PERIOD_MS                     (1000 * 60 * 15)
#define CO2_LEVEL_ERROR                     (100)
#define CO2_LEVEL_GOOD                      (800)
#define CO2_LEVEL_MEDIOCRE                  (1500)

typedef struct msb_lsb {
    uint8_t high;
    uint8_t low;
} msb_lsb_t;

typedef struct measurements {
    msb_lsb_t co2;
    uint8_t co2_crc;
    msb_lsb_t temperature;
    uint8_t temperature_crc;
    msb_lsb_t humidity;
    uint8_t humidity_crc;
} measurements_t;

void sensors_task(void *arg);