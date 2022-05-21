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

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE

static const char *IKEA_TAG = "ikea";

response_frame_t response_frame = {
    .height0 = 0x00,
    .height1 = 0x00,
    .action = DESK_IDLE,
    .checksum = 0x00
};

response_frame_t1 response_frame1 = {
    //.height = {0x00, 0x00},
    .height.msb = 0x00,
    .height.lsb = 0x00,
    .action = DESK_IDLE,
    .checksum = 0x00
};

response_frame_t1 keep_alive_frame1 = {
    //.height = {0x00, 0x00},
    .height.msb = 0x00,
    .height.lsb = 0x00,
    .action = 0x00,
    .checksum = 0xEE
};

response_frame_t keep_alive_frame = {
    .height0 = 0x00,
    .height1 = 0x00,
    .action = 0x00,
    .checksum = 0xEE
};

volatile uint8_t msb0 = 0xAA;
volatile uint8_t lsb0 = 0xBB;

status_frame_t *status_frame_right = NULL;
status_frame_t *status_frame_left = NULL;

void master_frames() {
    master_start_frame(LIN_PROTECTED_ID_KEEP_ALIVE);
    master_start_frame(LIN_PROTECTED_ID_STATUS_RIGHT);
    master_start_frame(LIN_PROTECTED_ID_STATUS_LEFT);
    master_start_frame(LIN_PROTECTED_ID_MOVE);
}

void desk_wake_up() {
    master_frames();
    ESP_LOGI(IKEA_TAG, "Waking up desk!");
}

void desk_move(uint8_t action) {
    response_frame.action = (response_frame.action == DESK_IDLE) ? DESK_BEFORE_MOVE : action;
    master_frames();
}

void desk_move_up() {

    if(response_frame.action == DESK_IDLE) {
        ESP_LOGI(IKEA_TAG, "Moving desk up!");
    }
    desk_move(DESK_UP);
}

void desk_move_down() {

    if(response_frame.action == DESK_IDLE) {
        ESP_LOGI(IKEA_TAG, "Moving desk down!");
    }
    desk_move(DESK_DOWN);
}

void desk_stop() {
    ESP_LOGI(IKEA_TAG, "Stopping desk!");
    response_frame.action = DESK_STOP;
    master_frames();
    master_frames();
    master_frames();
    response_frame.action = DESK_BEFORE_IDLE;
    master_frames();
    response_frame.action = DESK_IDLE;
    master_frames();
}

void desk_handle_lin_frame(lin_frame_t *lin_frame, uint8_t *event_data, uint8_t event_size) {
    uint8_t protected_id = lin_frame->protected_id & 0x3F;
    esp_log_level_set(IKEA_TAG, ESP_LOG_DEBUG);

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
        uart_write_bytes(UART_PORT, &keep_alive_frame, sizeof(keep_alive_frame));
        ESP_LOG_BUFFER_HEX_LEVEL(IKEA_TAG, &keep_alive_frame, sizeof(keep_alive_frame), ESP_LOG_DEBUG);
    } else if(protected_id == LIN_PROTECTED_ID_MOVE) {

        if(status_frame_left != NULL && status_frame_right != NULL) {
            response_frame.height0 = msb0;
            response_frame.height1 = lsb0;
            response_frame.checksum = checksum((uint8_t*) &response_frame, lin_frame->protected_id);

            /*
            uint8_t ppp = 0x92;
            response_frame.height0 = 0x05;
            response_frame.height1 = 0x10;
            response_frame.action = 0x86;
            response_frame.checksum = checksum((uint8_t*) &response_frame, ppp);
            */
            uart_write_bytes(UART_PORT, &response_frame, sizeof(response_frame));

            ESP_LOG_BUFFER_HEX_LEVEL(IKEA_TAG, &response_frame, sizeof(response_frame), ESP_LOG_DEBUG);
            status_frame_right = NULL;
            status_frame_left = NULL;
        }
    } else if(protected_id == LIN_PROTECTED_ID_STATUS_RIGHT || protected_id == LIN_PROTECTED_ID_STATUS_LEFT) {

        if(event_size < (LIN_HEADER_SIZE + LIN_DATA_SIZE + LIN_CHECKSUM_SIZE)) {
            ESP_LOGW(IKEA_TAG, "Status event too small");
            ESP_LOG_BUFFER_HEX_LEVEL(IKEA_TAG, event_data, event_size, ESP_LOG_WARN);
            return;
        }

        if(protected_id == LIN_PROTECTED_ID_STATUS_LEFT) {
            status_frame_left = (status_frame_t*) lin_frame;
            return;
        }

        status_frame_right = (status_frame_t*) lin_frame;
        uint16_t new_desk_height = status_frame_right->height0 + (status_frame_right->height1 << 8);
        new_desk_height = round((6370.5 + new_desk_height) / 100.5);

        msb0 = lin_frame->data[0];
        lsb0 = lin_frame->data[1];

        //response_frame.height0 = status_frame->height0;
        //response_frame.height1 = status_frame->height1;

        ESP_LOG_BUFFER_HEX_LEVEL(IKEA_TAG, &status_frame_right, sizeof(status_frame_right), ESP_LOG_DEBUG);

        if(new_desk_height != current_desk_height) {

            if(current_desk_height == 0xFF) {
                target_desk_height = new_desk_height;
            }

            current_desk_height = new_desk_height;
            desk_percentage = round((current_desk_height / (float)DESK_MAX_HEIGHT) * 100);
            ESP_LOGI(IKEA_TAG, "Desk height %dcm @ %d%%", current_desk_height, desk_percentage);
        }
    }
}
