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
#include "esp_log.h"
#include "ikea.h"
#include "math.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE

static const char *IKEA_TAG = "ikea";

response_frame_t response_frame = {
    .height0 = 0x00,
    .height1 = 0x00,
    .action = DESK_IDLE,
    .checksum = 0x00
};

response_frame_t keep_alive_frame = {
    .height0 = 0x00,
    .height1 = 0x00,
    .action = 0x00,
    .checksum = 0xEE
};

status_frame_t *status_frame = NULL;

void desk_move_up() {

    if(response_frame.action == DESK_IDLE) {
        ESP_LOGI(IKEA_TAG, "Moving desk up!");
    }
    response_frame.action = DESK_UP;
}

void desk_move_down() {

    if(response_frame.action == DESK_IDLE) {
        ESP_LOGI(IKEA_TAG, "Moving desk down!");
    }
    response_frame.action = DESK_DOWN;
}

void desk_stop() {
    response_frame.action = DESK_STOP;
    vTaskDelay(10);
    response_frame.action = DESK_IDLE;
    ESP_LOGI(IKEA_TAG, "Stopping desk!");
}

void desk_handle_lin_frame(lin_frame_t *lin_frame, uint8_t *event_data, uint8_t event_size) {
    uint8_t protected_id = lin_frame->protected_id & 0x3F;
    esp_log_level_set(IKEA_TAG, ESP_LOG_DEBUG);
    ESP_LOG_BUFFER_HEX_LEVEL(IKEA_TAG, event_data, event_size, ESP_LOG_DEBUG);
    
    if(protected_id == LIN_PROTECTED_ID_SYNC) {
        // TODO INIT
        /*
        I (15234) lin: Desk height 203cm @ 162%
        W (15234) lin: 00 55 92 f6 ff bf b6 
        I (15334) lin: Desk height 203cm @ 162%
        W (15334) lin: 00 55 92 f6 ff ff 76 
        I (15434) lin: Desk height 102cm @ 81%
        W (15434) lin: 00 55 92 66 0f fc fa 
        */
    } else if(protected_id == LIN_PROTECTED_ID_KEEP_ALIVE) {
            //keep_alive_frame.checksum = checksum((uint8_t *) &keep_alive_frame, lin_frame->protected_id);
            //uart_write_bytes(UART_NUM_2, &keep_alive_frame, sizeof(keep_alive_frame));
            //ESP_LOG_BUFFER_HEX_LEVEL(IKEA_TAG, &keep_alive_frame, sizeof(keep_alive_frame), ESP_LOG_DEBUG);
    } else if(protected_id == LIN_PROTECTED_ID_MOVE) {

        // check ready
        /*if (lin_frame->data[1] == 0xFF || lin_frame->data[2] == 0xBF) {
            ESP_LOGW(LIN_TAG, "Synchronizing");
            return;
        }*/

        if(response_frame.action != DESK_IDLE && status_frame != NULL) {
            response_frame.height0 = status_frame->height0;
            response_frame.height1 = status_frame->height1;
            response_frame.checksum = checksum((uint8_t*) &response_frame, lin_frame->protected_id);
            //uart_write_bytes(UART_NUM_2, &response_frame, sizeof(response_frame));
            //ESP_LOG_BUFFER_HEX_LEVEL(IKEA_TAG, &response_frame, sizeof(response_frame), ESP_LOG_DEBUG);
        }
    } else if(protected_id == LIN_PROTECTED_ID_STATUS) {
        status_frame = (status_frame_t*)lin_frame;

        uint16_t new_desk_height = status_frame->height0 + (status_frame->height1 << 8);
        new_desk_height = round((12741.0 + (2.0 * (float)new_desk_height)) / 201.0);

        if(new_desk_height != current_desk_height) {

            if(current_desk_height == 0xFF) {
                target_desk_height = new_desk_height;
            }

            /*if (checksum(lin_frame->data, lin_frame->protected_id) != lin_frame->checksum) {
                ESP_LOGE(LIN_TAG, "Skipping invalid r %02x!", lin_frame->checksum);
                ESP_LOG_BUFFER_HEX_LEVEL(LIN_TAG, lin_frame, event_size, ESP_LOG_ERROR);
                return;
            }*/

            current_desk_height = new_desk_height;
            desk_percentage = (current_desk_height / (float)DESK_MAX_HEIGHT) * 100;
            ESP_LOGI(IKEA_TAG, "Desk height %dcm @ %d%%", current_desk_height, desk_percentage);
        }
    }
}