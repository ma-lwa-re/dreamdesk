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
#ifndef MQTT_H
#define MQTT_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "mqtt_client.h"

#define MQTT_STACK_SIZE                         (8192)
#define HOMEKIT_STACK_SIZE                      MQTT_STACK_SIZE

#define MQTT_DEFAULT_BROKER_URI                 "mqtt://10.10.10.24:1883"
#define MQTT_DEFAULT_DISCOVERY_PREFIX           "homeassistant"
#define MQTT_DEFAULT_TOPIC_PREFIX               "dreamdesk"

void home_task(void *arg);

#endif /* MQTT_H */
