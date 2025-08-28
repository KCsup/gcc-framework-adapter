cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(gcc_framework_project C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(adapter
  src/adapter.c
  src/commands.c
  src/usb_descriptors.c
)

pico_generate_pio_header(adapter ${CMAKE_CURRENT_LIST_DIR}/src/adapter.pio)


pico_enable_stdio_usb(adapter 1)
pico_enable_stdio_uart(adapter 1)
pico_add_extra_outputs(adapter)
target_link_libraries(adapter
  pico_stdlib
  hardware_pio
  hardware_clocks
  hardware_dma
  tinyusb_device
  tinyusb_board
)
