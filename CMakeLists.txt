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

# Define our application name
set(APP pico_demo_gatt_service)

# Use the variant you want - we omitted FreeRTOS
# Poll variant runs btstack callbacks from the main thread
# Background variant runs btstack callbacks from IRQs
set(POLL 0)

project(${APP})

pico_sdk_init()

add_executable(${APP} ${APP}.c led.c)

# Link CYW43 driver for selected async context: main thread or interrupts
if (${POLL})
    target_link_libraries(${APP} pico_cyw43_arch_poll)
else()
    target_link_libraries(${APP} pico_cyw43_arch_threadsafe_background)
endif()

# Provide btstack configuration
target_include_directories(${APP} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/config)

# Disable LWIP
target_compile_definitions(${APP} PUBLIC CYW43_LWIP=0)

# Generate GATT header
pico_btstack_make_gatt_header(${APP} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/${APP}.gatt)

# Link Pico stdlib
target_link_libraries(${APP} pico_stdlib)

# Link BT stack
target_link_libraries(${APP} pico_btstack_cyw43)
# You may not need both BLE and Classic depending of your application
target_link_libraries(${APP} pico_btstack_ble)
target_link_libraries(${APP} pico_btstack_classic)

# Decide if you want stdio over USB or UART
pico_enable_stdio_usb(${APP} 0)
pico_enable_stdio_uart(${APP} 1)

# Create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(${APP})

# Do we really need this?
suppress_btstack_warnings()
