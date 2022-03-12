#idf_component_register(SRCS ./wifi.c ./main.c ./dreamdesk.c ./homekit.c ./ikea.c ./lin.c INCLUDE_DIRS ".")
idf_component_register(SRCS ./wifi.c ./main.c ./dreamdesk.c ./homekit.c ./logicdata.c ./lin.c INCLUDE_DIRS ".")

set(LOGICDATA 1)
#set(IKEA 1)

#if(LOGICDATA)
#add_definitions(-DLOGICDATA=1)
#idf_component_register(SRCS ./wifi.c ./main.c ./dreamdesk.c ./homekit.c ./logicmotion.c ./lin.c INCLUDE_DIRS ".")
#endif()

#if(IKEA)
#    add_definitions(-DIKEA=1)
#    idf_component_register(SRCS ./wifi.c ./main.c ./dreamdesk.c ./homekit.c ./ikea.c ./lin.c INCLUDE_DIRS ".")
#endif()

add_definitions(-DHAP_LOG_LEVEL=${CONFIG_HAP_LOG_LEVEL})

# Choose your side
#add_compile_definitions(APPLE_HOMEKIT)
#add_compile_definitions(GOOGLE_HOME)
#add_compile_definitions(AMAZON_ALEXA)

# Choose your side
#add_compile_definitions(LOGICDATA=1)
#add_compile_definitions(IKEA)
#add_definitions(-DLOGICDATA=1)