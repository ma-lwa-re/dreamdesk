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

uint8_t checksum(uint8_t *lin_frame, uint8_t protected_id) {
    uint16_t checksum = protected_id;
    for(uint8_t i = 0; i < 8; i++) {
        checksum += lin_frame[i];
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