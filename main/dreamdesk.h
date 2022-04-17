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
#include <stdbool.h>

#define LOG_MAXIMUM_LEVEL ESP_LOG_VERBOSE

#if defined(LOGICDATA)
#include "logicdata.h"
#elif defined(IKEA)
#include "ikea.h"
#else
#error No desk type defined!
#endif

#define LED_STATUS              (GPIO_NUM_1)
#define LED_ACTIVITY            (GPIO_NUM_2)
#define OFF                     (0x00)
#define ON                      (0x01)
#define UART_NUM_2_TXD          (GPIO_NUM_4)
#define UART_NUM_2_RXD          (GPIO_NUM_5)
#define UART_BAUD_RATE          (19200)
#define UART_STACK_SIZE         (4096)
#define CONSOLE_BAUD_RATE       (115200)
#define ARROW_KEY_UP            (0x41)
#define ARROW_KEY_DOWN          (0x42)
#define MEMORY_1                (0x31)
#define MEMORY_2                (0x32)
#define MEMORY_3                (0x33)
#define MEMORY_4                (0x34)
#define MEMORY_5                (0x35)
#define MEMORY_6                (0x36)
#define MEMORY_7                (0x37)

extern uint8_t current_desk_height;
extern uint8_t target_desk_height;
extern uint8_t desk_percentage;

extern uint8_t desk_ready;
extern uint8_t desk_reset;
extern uint8_t desk_control;

void chip_info();

void memory_init();

void desk_handle_lin_frame(lin_frame_t *lin_frame, uint8_t *event_data, uint8_t event_size);

void desk_update_height(status_frame_t *status_frame);

void desk_set_target_percentage(uint8_t target_percentage);

void desk_set_target_height(uint8_t target_height);

void rx_task(void *arg);

void usb_task(void *arg);

void move_task(void *arg);