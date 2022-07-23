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

#define TEMPERATURE_OFFSET                  (SENSORS_TEMPERATURE_OFFSET)
#define SENSOR_ALTITUDE                     (SENSORS_SENSOR_ALTITUDE)
#define FAHRENHEIT(celcius)                 (((celcius * 9.0) / 5.0) + 32.0)
#define KELVIN(celcius)                     (celcius + 273.15)
#define SCALE_CELCIUS                       ('C')
#define SCALE_FAHRENHEIT                    ('F')
#define SCALE_KELVIN                        ('K')
#define MEASUREMENT_COUNT                   (0x05)

#define CO2_LEVEL_UNKNOWN                   (200)
#define CO2_LEVEL_EXCELLENT                 (600)
#define CO2_LEVEL_GOOD                      (1000)
#define CO2_LEVEL_FAIR                      (1400)
#define CO2_LEVEL_INFERIOR                  (1800)
#define CO2_LEVEL_POOR                      (2200)

enum air_quality_t {UNKNOWN, EXCELLENT, GOOD,
                    FAIR, INFERIOR, POOR};

char get_temperature_scale();

float get_current_temperature();

float get_current_relative_humidity();

float get_co2_level();

float get_co2_peak_level();

enum air_quality_t get_air_quality();

void sensors_task(void *arg);
