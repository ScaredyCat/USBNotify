cmake_minimum_required(VERSION 3.13)

# 1. Pull in the Pico SDK
#include(pico_sdk_import.cmake)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)


project(hid_led C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(hid_led
    main.c
    usb_descriptors.c  # This file defines the HID device type
)


target_include_directories(hid_led PRIVATE ${CMAKE_CURRENT_LIST_DIR})


# 2. Generate the header from your .pio file
pico_generate_pio_header(hid_led ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)

# 3. Link libraries (tinyusb_device is the key for HID)
target_link_libraries(hid_led 
    pico_stdlib 
    pico_flash
    hardware_pio
    hardware_flash
    hardware_sync
    tinyusb_device
    tinyusb_board
    m
)

# 4. Enable TinyUSB and standard I/O
pico_enable_stdio_usb(hid_led 0)
pico_enable_stdio_uart(hid_led 1)

# 5. Generate the .uf2 file for dragging/dropping
pico_add_extra_outputs(hid_led)

