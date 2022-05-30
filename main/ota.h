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
#include <stdlib.h>
#include "esp_efuse.h"
#include "esp_ota_ops.h"
#include "esp_https_ota.h"

#define OTA_PROJECT_NAME        (PROJECT_NAME)
#define OTA_PROJECT_VER         (PROJECT_VER)
#define OTA_UPDATE_URL          ("https://ma.lwa.re/ota/Dreamdesk.bin")
#define OTA_UPDATE_USER_AGENT   ("ESP32 HTTP Client/1.0")
#define SLEEP_INTERVAL_10_SEC   (1000 * 10)
#define SLEEP_INTERVAL_12_HOURS (1000 * 60 * 60 * 12)

void ota_task(void *arg);
