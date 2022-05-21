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
#include "dreamdesk.h"

typedef struct master_frame {
    uint8_t sync;
    uint8_t pid;
} master_frame_t;

master_frame_t master_frame = {
    .sync = LIN_HEADER_SYNC,
    .pid = 0x00
};

uint8_t checksum(uint8_t *lin_data, uint8_t protected_id) {
    uint16_t checksum = (protected_id & 0x3F) | parity(protected_id);
    for(uint8_t i = 0; i < LIN_DATA_SIZE; i++) {
        checksum += lin_data[i];
        if(checksum > 0xFF) {
            checksum -= 0xFF;
        }
    }
    return (~checksum & 0xFF);
}

uint8_t parity(uint8_t pid) {
    uint8_t p0 = P(pid, 0) ^ P(pid, 1) ^ P(pid, 2) ^ P(pid, 4);
    uint8_t p1 = ~(P(pid, 1) ^ P(pid, 3) ^ P(pid, 4) ^ P(pid, 5));
    return (p0 | (p1 << 1)) << 6;
}

void master_start_frame(uint8_t pid) {
    ets_delay_us(6000);

    master_frame.pid = pid | parity(pid);

    uart_flush_input(UART_PORT);
    xQueueReset(uart_queue);

    uart_set_line_inverse(UART_PORT, UART_SIGNAL_TXD_INV);
    ets_delay_us(678);
    uart_set_line_inverse(UART_PORT, UART_SIGNAL_INV_DISABLE);

    xQueueSend(uart_queue, (void*) &(uart_event_t){.type = UART_BREAK}, 0);
    uart_write_bytes(UART_PORT, &master_frame, sizeof(master_frame));
}
