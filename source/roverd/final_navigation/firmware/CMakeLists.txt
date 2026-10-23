cmake_minimum_required(VERSION 3.13)

# ---------------------------------------------------------------------------
# Pico SDK bootstrapping
# Set PICO_SDK_PATH in your environment, or pass it at configure time:
#   cmake -DPICO_SDK_PATH=/path/to/pico-sdk ..
# ---------------------------------------------------------------------------
if(NOT PICO_SDK_PATH)
    set(PICO_SDK_PATH "$ENV{PICO_SDK_PATH}")
endif()
if(NOT EXISTS "${PICO_SDK_PATH}/external/pico_sdk_import.cmake")
    message(FATAL_ERROR "Set PICO_SDK_PATH to your Raspberry Pi Pico SDK directory")
endif()
include("${PICO_SDK_PATH}/external/pico_sdk_import.cmake")

project(rover_move C CXX ASM)
set(CMAKE_C_STANDARD   11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

# ---------------------------------------------------------------------------
# Pico executable
# ---------------------------------------------------------------------------
add_executable(rover_move
    rover_move.cpp
)

target_link_libraries(rover_move
    pico_stdlib
    hardware_pwm
    hardware_gpio
    hardware_uart
    hardware_clocks
)

# Route stdio to USB (print debugging via USB serial, e.g. minicom / screen).
# The Pi -> Pico motor-command link uses hardware UART0 on GP17 RX.
pico_enable_stdio_usb(rover_move  1)
pico_enable_stdio_uart(rover_move 0)

# Generate .uf2 (drag-and-drop flash) and .hex alongside the .elf
pico_add_extra_outputs(rover_move)
