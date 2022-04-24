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
#include "dreamdesk.h"
#if defined(HOMEKIT)
#include "homekit.h"
#endif
#if defined(SENSORS_ON)
#include "sensors.h"
#endif
#include "esp_log.h"
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

static const char *DREAMDESK_TAG = "dreamdesk";

void app_main() {
    esp_log_level_set(DREAMDESK_TAG, ESP_LOG_INFO);
    ESP_LOGI(DREAMDESK_TAG, "Hello there!");

    chip_info();
    memory_init();

    #if defined(SENSORS_ON)
    xTaskCreate(sensors_task, "sensors_task", UART_STACK_SIZE, NULL, configMAX_PRIORITIES-9, NULL);
    #endif

    #if defined(HOMEKIT) || defined(NEST) || defined(ALEXA)
    xTaskCreate(home_task, "home_task", HOMEKIT_STACK_SIZE, NULL, configMAX_PRIORITIES-7, NULL);
    #endif

    xTaskCreate(usb_task, "usb_task", UART_STACK_SIZE, NULL, configMAX_PRIORITIES-5, NULL);
    xTaskCreate(move_task, "move_task", UART_STACK_SIZE, NULL, configMAX_PRIORITIES-3, NULL);
    xTaskCreate(rx_task, "rx_task", UART_STACK_SIZE, NULL, configMAX_PRIORITIES-1, NULL);

    gpio_config(&(gpio_config_t){
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = ((1ULL << LED_STATUS) | (1ULL << LED_ACTIVITY))
    });

    gpio_set_level(LED_ACTIVITY, OFF);
    gpio_set_level(LED_STATUS, ON);
}
