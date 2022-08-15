# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# initalize pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
#set(PICO_SDK_PATH "~/pico/pico-sdk")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(particlesim C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(particlesim
        particlesim.cpp particlesim.h
        MPU6050.cpp MPU6050.h
        hub75.cpp hub75.h
        simulation.cpp simulation.h
        animations_basic.cpp animations_basic.h
        anim_helpers.cpp anim_helpers.h
        snake.cpp snake.h
        GameOfLife.cpp GameOfLife.h
        images/img_all.h gol/gol_all.h
        )

# Compile stages and universes
# TODO: add all files to output, via CMake generator expression
add_custom_command(
        OUTPUT gol/gol_all.h images/img_all.h
        COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/scripts/png_to_header.py auto
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

pico_generate_pio_header(particlesim ${CMAKE_CURRENT_LIST_DIR}/hub75.pio)

#pico_set_program_name(particlesim "particlesim")
pico_set_program_version(particlesim "0.1")

pico_enable_stdio_uart(particlesim 0)
pico_enable_stdio_usb(particlesim 1)

# Add the standard library to the build
target_link_libraries(particlesim pico_stdlib)

# Add any user requested libraries
target_link_libraries(particlesim
        pico_multicore
        hardware_i2c
        hardware_dma
        hardware_pio
        hardware_interp
        hardware_timer
        )

pico_add_extra_outputs(particlesim)

