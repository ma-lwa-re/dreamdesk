# REQUIRED: Choose your desk type (LOGICDATA | IKEA)
set(DESK_TYPE "LOGICDATA")

# OPTIONAL: Choose your home automation type (HOMEKIT | NEST | ALEXA | NONE)
set(HOME_AUTOMATION "HOMEKIT")

# OPTIONAL: Use the sensors (ON | OFF)
set(SENSORS ON)

if(DESK_TYPE STREQUAL "LOGICDATA")
    set(INCLUDE_DESK ./logicdata.c)
elseif(DESK_TYPE STREQUAL "IKEA")
    set(INCLUDE_DESK ./ikea.c)
endif()

if(HOME_AUTOMATION STREQUAL "HOMEKIT")
    set(INCLUDE_HOME ./wifi.c ./homekit.c)
elseif(HOME_AUTOMATION STREQUAL "NEST")
    set(INCLUDE_HOME ./wifi.c ./nest.c)
elseif(HOME_AUTOMATION STREQUAL "ALEXA")
    set(INCLUDE_HOME ./wifi.c ./alexa.c)
endif()

if(SENSORS)
    set(INCLUDE_SENSORS ./sensors.c)
endif()

idf_component_register(SRCS ./main.c ./dreamdesk.c ./lin.c ${INCLUDE_DESK} ${INCLUDE_HOME} ${INCLUDE_SENSORS} INCLUDE_DIRS ".")

add_definitions(-D${DESK_TYPE} -D${HOME_AUTOMATION} -DSENSORS_${SENSORS})
