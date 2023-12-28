cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico_w)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(stk500v2)

pico_sdk_init()

add_executable(stk500v2)

target_sources(stk500v2 PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/src/main.c
        ${CMAKE_CURRENT_LIST_DIR}/src/usb_descriptors.c
        ${CMAKE_CURRENT_LIST_DIR}/src/stk500v2.c
        ${CMAKE_CURRENT_LIST_DIR}/src/program.c
        ${CMAKE_CURRENT_LIST_DIR}/src/hal.c
        )

target_include_directories(stk500v2 PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/src)

target_link_libraries(
        stk500v2
        PUBLIC
        pico_stdlib              # for core functionality
        hardware_spi
        tinyusb_device 
        tinyusb_board
        pico_cyw43_arch_none     # we need Wifi to access the GPIO, but we don't need anything else
        )

# pico_enable_stdio_usb(stk500v2 1)  
# pico_enable_stdio_uart(stk500v2 0)

pico_add_extra_outputs(stk500v2)
