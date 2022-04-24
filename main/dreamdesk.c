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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "hal/uart_types.h"
#include "sdkconfig.h"
#include "string.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "nvs_flash.h"
#include "dreamdesk.h"

static const char *DREAMDESK_TAG = "dreamdesk";
static const char *LIN_TAG = "lin";

uint8_t current_desk_height = 0xFF;
uint8_t target_desk_height = 0xFF;
uint8_t desk_percentage = 0xFF;

uint8_t desk_control = false;

void chip_info() {
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
  
    ESP_LOGI(DREAMDESK_TAG, "%s with %d CPU cores, WiFi%s%s, revision %d, %dMB %s flash",
        CONFIG_IDF_TARGET, chip_info.cores, (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
        (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "", chip_info.revision,
        spi_flash_get_chip_size() / (1024 * 1024),
        (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external"
    );
}

void memory_init() {
    esp_err_t flash_error = nvs_flash_init();
    if(flash_error == ESP_ERR_NVS_NO_FREE_PAGES || flash_error == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        flash_error = nvs_flash_init();
    }
    ESP_ERROR_CHECK(flash_error);
}

void desk_set_target_percentage(uint8_t target_percentage) {
    desk_set_target_height((((DESK_MAX_HEIGHT - DESK_MIN_HEIGHT) / 100.0) * target_percentage) + DESK_MIN_HEIGHT);
}

void desk_set_target_height(uint8_t target_height) {

    if(target_height < DESK_MIN_HEIGHT || target_height > DESK_MAX_HEIGHT) {
        ESP_LOGE(DREAMDESK_TAG, "Target height %dcm is out of range!", target_height);
        return;
    }

    target_desk_height = target_height;
    desk_control = true;
    ESP_LOGI(DREAMDESK_TAG, "Setting the desk at %dcm", target_height);
}

void rx_task(void *arg) {
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB
    };

    QueueHandle_t uart_queue = NULL;

    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_2, UART_NUM_2_TXD, UART_NUM_2_RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_2, UART_FIFO_LEN * 2, 0, 10, &uart_queue, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_2, &uart_config));
    ESP_ERROR_CHECK(uart_set_rx_timeout(UART_NUM_2, 1));

    //#ifdef IKEA
    //ESP_ERROR_CHECK(uart_set_line_inverse(UART_NUM_2, UART_NUM_2_RXD));
    //ESP_ERROR_CHECK(uart_set_line_inverse(UART_NUM_2, UART_NUM_2_TXD));
    //#endif

    esp_log_level_set(LIN_TAG, ESP_LOG_INFO);

    uart_event_t lin_event;
    uint8_t *event_data = (uint8_t*) malloc(128);

    while(xQueueReceive(uart_queue, (void*) &lin_event, portMAX_DELAY)) {

        if(lin_event.type == UART_DATA) {
            gpio_set_level(LED_ACTIVITY, ON);
            memset(event_data, 0x00, 128);

            int16_t protected_id = -1;
            lin_frame_t *lin_frame = NULL;
            
            uart_read_bytes(UART_NUM_2, event_data, lin_event.size, 1);
            ESP_LOG_BUFFER_HEX_LEVEL(LIN_TAG, event_data, lin_event.size, ESP_LOG_DEBUG);
            
            for(uint8_t i = 0; i <= LIN_HEADER_SIZE; i++) {

                if(event_data[i] == LIN_HEADER_SYNC){
                    lin_frame = (lin_frame_t*) &event_data[i + 1];
                    break;
                } else if((event_data[i] == LIN_HEADER_BREAK && event_data[i + 1] == LIN_HEADER_SYNC)) {
                    lin_frame = (lin_frame_t*) &event_data[i + 2];
                    break;
                }
            }

            if (lin_frame == NULL) {
                continue;
            }

            protected_id = lin_frame->protected_id & 0x3F;

            if(protected_id < LIN_PROTECTED_ID_MIN || protected_id > LIN_PROTECTED_ID_MAX) {
                ESP_LOGE(LIN_TAG, "Invalid protected_id %d", protected_id);
                ESP_LOG_BUFFER_HEX_LEVEL(LIN_TAG, event_data, lin_event.size, ESP_LOG_ERROR);
            }

            // Check checksum ?? nope break the communication, only use for specific tasks
            /*if (event_size > LIN_HEADER_SIZE) {
                if (checksum(lin_frame->data, lin_frame->protected_id) != lin_frame->checksum) {
                    ESP_LOGE(LIN_TAG, "Skipping invalid frame %02x!", lin_frame->checksum);
                    ESP_LOG_BUFFER_HEX_LEVEL(LIN_TAG, lin_frame, event_size, ESP_LOG_ERROR);
                    continue;
                }
            }*/

            desk_handle_lin_frame(lin_frame, event_data, lin_event.size);
        }
        gpio_set_level(LED_ACTIVITY, OFF);
    }
}

void move_task(void *arg) {
    for(;;) {

        if(desk_control) {

            if(target_desk_height != current_desk_height) {

                if(target_desk_height < current_desk_height) {
                    desk_move_down();
                }

                if(target_desk_height > current_desk_height) {
                    desk_move_up();
                }
            }

            if(target_desk_height == current_desk_height) {
                desk_stop();
                desk_control = false;
                target_desk_height = current_desk_height;
            }
        }
        vTaskDelay(10);
    }
}

void usb_task(void *arg) {
    uart_config_t uart_config = {
        .baud_rate = CONSOLE_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB
    };

    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_0, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0, UART_FIFO_LEN * 2, 0, 10, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_0, &uart_config));
    ESP_ERROR_CHECK(uart_set_rx_timeout(UART_NUM_0, 1));

    uint8_t move_data = 0;

    for(;;) {
        ESP_ERROR_CHECK(uart_get_buffered_data_len(UART_NUM_0, (size_t*) &move_data));

        if(move_data > 0) {
            keyboard_t keyboard = {
                .memory = 0x00,
                .reserved0 = {0x00},
                .arrow_key = 0x00
            };

            uart_read_bytes(UART_NUM_0, &keyboard, sizeof(keyboard), 0x01);
        
            if(keyboard.arrow_key == ARROW_KEY_UP) {
                desk_set_target_height(target_desk_height + 0x01);
            }

            if(keyboard.arrow_key == ARROW_KEY_DOWN) {
                desk_set_target_height(target_desk_height - 0x01);
            }

            if(keyboard.memory == MEMORY_1) {
                desk_set_target_height(MEMORY_1_HEIGHT);
            }

            if(keyboard.memory == MEMORY_2) {
                desk_set_target_height(MEMORY_2_HEIGHT);
            }

            if(keyboard.memory == MEMORY_3) {
                desk_set_target_height(MEMORY_3_HEIGHT);
            }

            if(keyboard.memory == MEMORY_4) {
                desk_set_target_height(MEMORY_4_HEIGHT);
            }

            if(keyboard.memory == MEMORY_5) {
                desk_set_target_height(MEMORY_5_HEIGHT);
            }

            if(keyboard.memory == MEMORY_6) {
                desk_set_target_height(MEMORY_6_HEIGHT);
            }

            if(keyboard.memory == MEMORY_7) {
                desk_set_target_height(MEMORY_7_HEIGHT);
            }
            ESP_LOG_BUFFER_HEX_LEVEL(LIN_TAG, &keyboard, sizeof(keyboard), ESP_LOG_DEBUG);
        }
        vTaskDelay(10);
    }
}