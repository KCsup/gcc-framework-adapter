cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(gcc_framework_project C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(adapter)

target_sources(adapter PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}/src/adapter.c
  ${CMAKE_CURRENT_LIST_DIR}/src/commands.c
  ${CMAKE_CURRENT_LIST_DIR}/src/usb_descriptors.c
)

target_include_directories(adapter PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)

pico_generate_pio_header(adapter ${CMAKE_CURRENT_LIST_DIR}/src/adapter.pio)


# pico_enable_stdio_usb(adapter 1)
# pico_enable_stdio_uart(adapter 1)

target_link_libraries(adapter
  pico_stdlib
  hardware_pio
  hardware_clocks
  hardware_dma
  tinyusb_device
  tinyusb_board
)

pico_add_extra_outputs(adapter)
