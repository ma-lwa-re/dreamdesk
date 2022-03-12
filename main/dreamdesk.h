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

#define LOGICDATA 1
//#define IKEA 1

#ifdef LOGICDATA
#include "logicdata.h"
#elif IKEA
#include "ikea.h"
#else
#error No desk type defined!
#endif

#define UART_NUM_2_TXD          (GPIO_NUM_4)
#define UART_NUM_2_RXD          (GPIO_NUM_5)
#define UART_NUM_2_RTS          (UART_PIN_NO_CHANGE)
#define UART_NUM_2_CTS          (UART_PIN_NO_CHANGE)
#define UART_BAUD_RATE          (19200)
#define UART_STACK_SIZE         (4096)

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

void key_task(void *arg);

void move_task(void *arg);