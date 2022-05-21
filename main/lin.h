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
#include "driver/uart.h"

#define LIN_BAUD_RATE               (19200)
#define LIN_HEADER_BREAK_DURATION   (678)
#define LIN_HEADER_BREAK            (0x00)
#define LIN_HEADER_SYNC             (0x55)
#define LIN_HEADER_SIZE             (0x03)
#define LIN_DATA_SIZE               (0x08)
#define LIN_CHECKSUM_SIZE           (0x01)
#define LIN_PROTECTED_ID_MIN        (0x00)
#define LIN_PROTECTED_ID_MAX        (0x3F)

#define P(pid, shift) ((pid & (1 << shift)) >> shift)

uint8_t checksum(uint8_t *lin_frame, uint8_t protected_id);

uint8_t parity(uint8_t pid);

void master_start_frame(uint8_t pid);
