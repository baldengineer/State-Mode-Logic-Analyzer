cmake_minimum_required(VERSION 3.25)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# initalize pico_sdk from installed location
set(PICO_SDK_PATH "C:/Users/balde/Dropbox/Pico/Pico SDK 1.5.1/pico-sdk")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)
#set(PICO_SDK_PATH "C:/Users/balde/Documents/Pico/pico-sdk")

project(pico C CXX ASM)

# Initialize the SDK
pico_sdk_init()

add_executable(pico
	pico.c
	)

pico_set_program_name(pico "pico")
pico_set_program_version(pico "0.1")

# no_flash means the target is to run from RAM
#pico_set_binary_type(IIe-keyboard_emulator no_flash)

# enable usb output, disable uart output
pico_enable_stdio_usb(pico 1)
pico_enable_stdio_uart(pico 0)

# pico_generate_pio_header(IIe-keyboard_emulator src/KBD.pio)
pico_generate_pio_header(pico ${CMAKE_CURRENT_LIST_DIR}/smla.pio)


#target_include_directories(IIe-keyboard_emulator PRIVATE src)

# pull in common dependencies
target_link_libraries(pico pico_stdlib)

# Add any user requested libraries
target_link_libraries(pico
        hardware_i2c
        hardware_dma
        hardware_pio
        hardware_timer
        hardware_watchdog
        hardware_pwm
        )

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(pico)

# add url via pico_set_program_url
#example_auto_set_url(pico)

# if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
#     add_compile_options(-Wno-maybe-uninitialized)
# endif()

# add_compile_options(-Wall
#         -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
#         -Wno-unused-function # we have some for the docs that aren't called
#         )
