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
#include "lin.h"

#define DESK_MIN_HEIGHT              (65)
#define DESK_MAX_HEIGHT              (125)

#undef LIN_DATA_SIZE
#define LIN_DATA_SIZE                (0x04)

#define LIN_PROTECTED_ID_SYNC        (0x06) // Not really needed ?? not tested yet
#define LIN_PROTECTED_ID_KEEP_ALIVE  (0x11)
#define LIN_PROTECTED_ID_MOVE        (0x12)
#define LIN_PROTECTED_ID_STATUS      (0x08)

#define DESK_MOVE_THRESHOLD          (0x02)
#define DESK_UP                      (0x86)
#define DESK_DOWN                    (0x85)
#define DESK_IDLE                    (0xFC)
//#define DESK_BEFORE_MOVE             (0xC4) // Not needed ??
//#define DESK_BEFORE_STOP             (0x84) // To test ??
#define DESK_STOP                    (0x87)
#define DESK_STATUS_READY            (0x60)
#define DESK_STATUS_BUSY             (0x02) // Frist value after init ??

typedef struct lin_frame {
    uint8_t protected_id;
    uint8_t data[3];
    uint8_t checksum;
} lin_frame_t;

typedef struct status_frame {
    uint8_t protected_id;
    uint8_t height0;
    uint8_t height1;
    uint8_t status;
    uint8_t checksum;
} status_frame_t;

typedef struct response_frame {
    uint8_t height0;
    uint8_t height1;
    uint8_t action;
    uint8_t checksum;
} response_frame_t;

extern uint8_t current_desk_height;
extern uint8_t target_desk_height;
extern uint8_t desk_percentage;

extern uint8_t desk_ready;
extern uint8_t desk_reset;
extern uint8_t desk_control;

void desk_move_up();

void desk_move_down();

void desk_stop();

void desk_handle_lin_frame(lin_frame_t *lin_frame, uint8_t *event_data, uint8_t event_size);