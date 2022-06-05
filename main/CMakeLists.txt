if(DESK_TYPE STREQUAL "LOGICDATA")
    set(INCLUDE_DESK ./logicdata.c)
elseif(DESK_TYPE STREQUAL "IKEA")
    set(INCLUDE_DESK ./ikea.c)
endif()

if(HOME_AUTOMATION STREQUAL "HOMEKIT")
    set(WIFI ON)
    set(INCLUDE_HOME ./homekit.c)
elseif(HOME_AUTOMATION STREQUAL "NEST")
    set(WIFI ON)
    set(INCLUDE_HOME ./nest.c)
elseif(HOME_AUTOMATION STREQUAL "ALEXA")
    set(WIFI ON)
    set(INCLUDE_HOME ./alexa.c)
endif()

if(SENSORS)
    set(INCLUDE_SENSORS ./sensors.c)
    add_definitions(-DSENSORS_SCALE_${SENSORS_SCALE})
endif()

if(OTA_UPDATES)
    set(WIFI ON)
    set(INCLUDE_OTA_UPDATES ./ota.c)
endif()

if(WIFI)
    set(INCLUDE_WIFI ./wifi.c)
endif()

idf_component_register(SRCS ./main.c ./dreamdesk.c ./lin.c ${INCLUDE_DESK} ${INCLUDE_WIFI}
                       ${INCLUDE_HOME} ${INCLUDE_SENSORS} ${INCLUDE_OTA_UPDATES} INCLUDE_DIRS ".")

add_definitions(-DPROJECT_NAME="${CMAKE_PROJECT_NAME}" -DPROJECT_VER="${PROJECT_VER}" -D${DESK_TYPE}
                -DWIFI_${WIFI} -D${HOME_AUTOMATION} -DSENSORS_${SENSORS} -DOTA_UPDATES_${OTA_UPDATES})
