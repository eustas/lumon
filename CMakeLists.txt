cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

project(lumon_umbrella C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.5.1")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.5.1 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

# Initialize the SDK
pico_sdk_init()

add_executable(lumon
  src/main.c
  src/renderA.c
  # src/renderB.c
)
target_include_directories(lumon PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)

pico_generate_pio_header(lumon ${CMAKE_CURRENT_LIST_DIR}/src/ws2812.pio)

target_link_libraries(lumon
  hardware_pio
  pico_btstack_ble
  pico_btstack_cyw43
  pico_cyw43_arch_none
  pico_multicore
  pico_stdlib
)

pico_add_extra_outputs(lumon)
