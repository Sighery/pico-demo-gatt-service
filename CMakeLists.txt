cmake_minimum_required(VERSION 3.13)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

# Define our app name
set(app pico_demo_gatt_service)

# Use the variant you want - we ommited FreeRTOS
# Either poll or background variant
set(poll 0)

project(pico_demo_gatt_service)

pico_sdk_init()

add_executable(pico_demo_gatt_service pico_demo_gatt_service.c bt_init.c)

if (${poll})
    target_sources(pico_demo_gatt_service PRIVATE main_poll.c)
    target_link_libraries(pico_demo_gatt_service pico_cyw43_arch_poll)
else()
    target_sources(pico_demo_gatt_service PRIVATE main_background.c)
    target_link_libraries(pico_demo_gatt_service pico_cyw43_arch_threadsafe_background)
endif()

#
target_include_directories(pico_demo_gatt_service PUBLIC ${CMAKE_CURRENT_LIST_DIR}/config)
set(BTSTACK_ROOT ${PICO_SDK_PATH}/lib/btstack)
#target_include_directories(pico_demo_gatt_service INTERFACE  ${BTSTACK_ROOT}/src)

# Disable LWIP
target_compile_definitions(pico_demo_gatt_service PUBLIC CYW43_LWIP=0)
# Generate GATT header
pico_btstack_make_gatt_header(pico_demo_gatt_service PUBLIC ${CMAKE_CURRENT_LIST_DIR}/pico_demo_gatt_service.gatt)

# pull in common dependencies
target_link_libraries(pico_demo_gatt_service pico_stdlib)
target_link_libraries(pico_demo_gatt_service pico_btstack_cyw43)
target_link_libraries(pico_demo_gatt_service pico_btstack_ble)
target_link_libraries(pico_demo_gatt_service pico_btstack_classic)

# enable usb output, disable uart output
pico_enable_stdio_usb(pico_demo_gatt_service 0)
pico_enable_stdio_uart(pico_demo_gatt_service 1)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(pico_demo_gatt_service)

# Why not?
suppress_btstack_warnings()
