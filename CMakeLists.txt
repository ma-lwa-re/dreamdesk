# CMake minimum version
cmake_minimum_required(VERSION 3.5)

# REQUIRED: Choose your desk type (LOGICDATA | IKEA)
set(DESK_TYPE "LOGICDATA")

# OPTIONAL: Choose your home automation ecosystem (HOMEKIT | NEST | ALEXA | NONE)
set(HOME_AUTOMATION "HOMEKIT")

# OPTIONAL: Use the sensors (ON | OFF)
set(SENSORS ON)

# OPTIONAL: Set the temperature scale (C | F | K)
set(SENSORS_SCALE "C")

# OPTIONAL: Set the temperature offset in celcius
set(SENSORS_TEMPERATURE_OFFSET 4.0)

# OPTIONAL: Set the sensor altitude in meters
set(SENSORS_SENSOR_ALTITUDE 0)

# OPTIONAL: Enable Over The Air (OTA) updates (ON | OFF)
set(OTA_UPDATES ON)

# OPTIONAL: Set the project version
set(PROJECT_VER "2.4.0.3")

# Include Sensirion SCD4x sensors lib
include_directories(esp32-scd4x)
set(EXTRA_COMPONENT_DIRS ${EXTRA_COMPONENT_DIRS} ${CMAKE_CURRENT_LIST_DIR}/lib/esp32-scd4x/)

if(HOME_AUTOMATION STREQUAL "HOMEKIT")
    set(EXTRA_COMPONENT_DIRS ${EXTRA_COMPONENT_DIRS} $ENV{IDF_PATH}/../esp-apple-homekit-adk/)
elseif(HOME_AUTOMATION STREQUAL "NEST")
    set(EXTRA_COMPONENT_DIRS ${EXTRA_COMPONENT_DIRS} $ENV{IDF_PATH}/../esp-google-iot/)
elseif(HOME_AUTOMATION STREQUAL "ALEXA")
    set(EXTRA_COMPONENT_DIRS ${EXTRA_COMPONENT_DIRS} $ENV{IDF_PATH}/../esp-aws-iot/)
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(Dreamdesk)
