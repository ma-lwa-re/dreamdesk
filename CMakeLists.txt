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

if(HOME_AUTOMATION STREQUAL "HOMEKIT")
    set(EXTRA_COMPONENT_DIRS ${CMAKE_CURRENT_LIST_DIR}/../esp-apple-homekit-adk/)
elseif(HOME_AUTOMATION STREQUAL "NEST")
    set(EXTRA_COMPONENT_DIRS ${CMAKE_CURRENT_LIST_DIR}/../esp-va-sdk/)
elseif(HOME_AUTOMATION STREQUAL "ALEXA")
    set(EXTRA_COMPONENT_DIRS ${CMAKE_CURRENT_LIST_DIR}/../esp-va-sdk/)
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(Dreamdesk)
