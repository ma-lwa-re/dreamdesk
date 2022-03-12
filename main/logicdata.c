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
#include "esp_log.h"
#include "logicdata.h"

static const char *LOGICDATA_TAG = "logicdata";
uint8_t desk_sleep = true;

response_frame_t response_frame = {
    .random = 0x00,
    .reserved0 = { 0x00 },
    .direction = DESK_DOWN,
    .reserved1 = { 0x00, 0x00, 0xFF },
    .action = DESK_IDLE,
    .reserved2 = { 0x01 },
    .checksum = 0x00
};

status_frame_t *status_frame = NULL;

void desk_wake_up() {
    uint8_t cafebabe[] = { 0xCA, 0xFE, 0xBA, 0xBE };
    uart_write_bytes(UART_NUM_2, cafebabe, sizeof(cafebabe));
    ESP_LOGI(LOGICDATA_TAG, "Waking up desk!");
}

void desk_move_up() {
    if (desk_sleep && response_frame.action == DESK_IDLE) {
        desk_wake_up();
    }
    response_frame.direction = DESK_UP;
    response_frame.action = DESK_MOVE;
    ESP_LOGI(LOGICDATA_TAG, "Moving desk up!");
}

void desk_move_down() {
    if (desk_sleep && response_frame.action == DESK_IDLE) {
        desk_wake_up();
    }
    response_frame.direction = DESK_DOWN;
    response_frame.action = DESK_MOVE;
    ESP_LOGI(LOGICDATA_TAG, "Moving desk down!");
}

void desk_stop() {
    if (response_frame.action != DESK_MOVE) {
        return;
    }
    response_frame.action = DESK_STOP;
    vTaskDelay(10);
    response_frame.action = DESK_IDLE;
    ESP_LOGI(LOGICDATA_TAG, "Stopping desk!");
}

void desk_handle_lin_frame(lin_frame_t *lin_frame, uint8_t *event_data, uint8_t event_size) {
    uint8_t protected_id = lin_frame->protected_id & 0x3F;
    desk_sleep = false;

    if (protected_id == LIN_PROTECTED_ID_SYNC) {

        if (event_size > LIN_HEADER_SIZE) {
            ESP_LOGI(LOGICDATA_TAG, "Pairing sequence %d%%", (uint8_t)((lin_frame->data[0] / 7.0) * 100));
            ESP_LOG_BUFFER_HEX_LEVEL(LOGICDATA_TAG, event_data, event_size, ESP_LOG_VERBOSE);
        }
    } else if (protected_id == LIN_PROTECTED_ID_MOVE) {

        //if ((desk_ready || desk_reset) && response_frame.action != DESK_IDLE) {
        if (response_frame.action != DESK_IDLE) {
            response_frame.random = rand() % 0xFF;                        
            response_frame.checksum = checksum((uint8_t *) &response_frame, lin_frame->protected_id);
            uart_write_bytes(UART_NUM_2, &response_frame, sizeof(response_frame));
        }
    } else if (protected_id == LIN_PROTECTED_ID_STATUS) {

        if (event_size < (LIN_DATA_SIZE - LIN_HEADER_SIZE)) {
            ESP_LOGW(LOGICDATA_TAG, "Status event too small");
            ESP_LOG_BUFFER_HEX_LEVEL(LOGICDATA_TAG, event_data, event_size, ESP_LOG_WARN);
            return;
        }

        status_frame = (status_frame_t*)lin_frame;

        if (status_frame->ready == DESK_READY) {
            //desk_ready = true;
            //desk_reset = false;
            uint8_t new_desk_height = ((lin_frame->data[3] << 8) + lin_frame->data[4]) / 10;

            if (new_desk_height != current_desk_height) {

                if (current_desk_height == 0xFF) {
                    target_desk_height = new_desk_height;
                }

                /*if (checksum(lin_frame->data, lin_frame->protected_id) != lin_frame->checksum) {
                    ESP_LOGE(LIN_TAG, "Skipping invalid r %02x!", lin_frame->checksum);
                    ESP_LOG_BUFFER_HEX_LEVEL(LIN_TAG, lin_frame, event_size, ESP_LOG_ERROR);
                    return;
                }*/

                current_desk_height = new_desk_height;
                desk_percentage = (lin_frame->data[5] / 255.0) * 100;
                ESP_LOGI(LOGICDATA_TAG, "Desk height %dcm @ %d%%", current_desk_height, desk_percentage);
            }
        } else if (status_frame->ready == DESK_NOT_READY) {
            //desk_ready = false;

            if (status_frame->status == DESK_PAIRING) {

                switch (status_frame->status_code) {
                    case 0x00:
                        ESP_LOGW(LOGICDATA_TAG, "Synchronizing");
                        break;
                    case 0x01:
                        ESP_LOGE(LOGICDATA_TAG, "Desk error, need to be reset!");
                        //desk_reset = true;
                        break;
                    default:
                        ESP_LOGE(LOGICDATA_TAG, "Unknown status (0x%02x)!", lin_frame->data[5]);
                        break;
                }
            } else if (status_frame->status == DESK_ERROR) {
                //desk_ready = false;

                switch (status_frame->error_code) {
                    case 0x01:
                        ESP_LOGE(LOGICDATA_TAG, "Firmware Error: Disconnect the Power Unit "
                            "from the Mains. Then, disconnect System from the Power "
                            "Unit. Reconnect the system again, then operate the DM "
                            "System as normal.");
                        break;
                    case 0x02:
                        ESP_LOGE(LOGICDATA_TAG, "Motor Over Current: Release all "
                            "Keys and wait for 5 seconds. Then, try again.");
                        break;
                    case 0x03:
                        ESP_LOGE(LOGICDATA_TAG, "DC Over Voltage: Release all "
                            "Keys and wait for 5 seconds. Then, try again.");
                        break;
                    case 0x08:
                        ESP_LOGE(LOGICDATA_TAG, "Impulse Detection Timeout: Perform a "
                            "Position Reset Procedure (see System Manual)");
                        break;
                    case 0x0B:
                        ESP_LOGE(LOGICDATA_TAG, "Speed cannot be achieved: Release all "
                            "Keys and wait for 5 seconds. Then, try again.");
                        break;
                    case 0x0C:
                        ESP_LOGE(LOGICDATA_TAG, "Power Stage Overcurrent: Release all "
                            "Keys and wait for 5 seconds. Then, try again.");
                        break;
                    case 0x0D:
                        ESP_LOGE(LOGICDATA_TAG, "DC Under Voltage: Release all "
                            "Keys and wait for 5 seconds. Then, try again.");
                        break;
                    case 0x0E:
                        ESP_LOGE(LOGICDATA_TAG, "Critical DC Over Voltage: Release all "
                            "Keys and wait for 5 seconds. Then, try again.");
                        break;
                    case 0x0F:
                        ESP_LOGE(LOGICDATA_TAG, "Strain Gauge is defective: Release all "
                            "Keys and wait for 5 seconds. Then, try again. "
                            "Contact LOGICDATA if the problem persists. "
                            "Do not operate the DM System if components are broken.");
                        break;
                    case 0x11:
                        ESP_LOGE(LOGICDATA_TAG, "Error during pairing sequence: Disconnect "
                            "the Power Unit from the Mains. Then, disconnect System "
                            "from the Power Unit. Reconnect the system again, then "
                            "operate the DM System as normal. If this fails, perform "
                            "a factory reset (see DM System Manual).");
                        break;
                    case 0x12:
                        ESP_LOGE(LOGICDATA_TAG, "Parameterization or firmware of different "
                        "Actuators in the Table System are incompatible: Re- "
                        "paramaterize the Actuators. Contact LOGICDATA for further "
                        "information.");
                        break;
                    case 0x13:
                        ESP_LOGE(LOGICDATA_TAG, "Too many / too few Actuators connected: "
                            "Connect the correct number of Actuators (as specified in "
                            "setup).");
                        break;
                    case 0x14:
                        ESP_LOGE(LOGICDATA_TAG, "Motor short circuit and/or open load: "
                            "Contact LOGICDATA.");
                        break;
                    case 0x15:
                        ESP_LOGE(LOGICDATA_TAG, "Firmware Error: Disconnect the Power Unit "
                            "from the Mains. Then, disconnect System from the Power "
                            "Unit. Reconnect the system again, then operate the DM "
                            "System as normal.");
                        break;
                    case 0x16:
                        ESP_LOGE(LOGICDATA_TAG, "Power Unit overload: Release all "
                            "Keys and wait for 5 seconds. Then, try again.");
                        break;
                    case 0x17:
                        ESP_LOGE(LOGICDATA_TAG, "Motor Under Voltage: Release all "
                            "Keys and wait for 5 seconds. Then, try again.");
                        break;                                        
                    default:
                        ESP_LOGE(LOGICDATA_TAG, "Unknown error code! (0x%02x)", 
                            lin_frame->data[6]);
                        break;
                }
            }
        } else {
            ESP_LOGE(LOGICDATA_TAG, "Unknown error code (0x%02x)!", lin_frame->data[2]);
            ESP_LOG_BUFFER_HEX_LEVEL(LOGICDATA_TAG, event_data, event_size, ESP_LOG_VERBOSE);
        }
    }
    desk_sleep = true;
    ESP_LOG_BUFFER_HEX_LEVEL(LOGICDATA_TAG, event_data, event_size, ESP_LOG_VERBOSE);
}