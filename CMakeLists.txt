# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==

# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(atividade C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(atividade atividade.c inc/ssd1306.c)

pico_set_program_name(atividade "atividade")
pico_set_program_version(atividade "0.1")

# Generate PIO header
pico_generate_pio_header(atividade ${CMAKE_CURRENT_LIST_DIR}/ws2818b.pio)

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(atividade 0)
pico_enable_stdio_usb(atividade 1)

# Add the standard library to the build
target_link_libraries(atividade
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(atividade PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

# Add any user requested libraries
target_link_libraries(atividade 
        hardware_pwm
        hardware_pio
        hardware_clocks
        hardware_i2c
        hardware_adc
        )

pico_add_extra_outputs(atividade)

