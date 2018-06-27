#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: BSD-5-Clause-Nordic
#

set(KCONFIG_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/Kconfig)
set(NRF_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)
set(BOARD_ROOT ${NRF_DIR})

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

add_subdirectory(${NRF_DIR} ${PROJECT_BINARY_DIR}/nrf)

FILE(GLOB main         src/*.c)
FILE(GLOB events       src/events/*.c)
FILE(GLOB hw_interface src/hw_interface/*.c)
FILE(GLOB modules      src/modules/*.c)
FILE(GLOB services     src/services/*.c)

target_sources(app PRIVATE ${main} ${events} ${hw_interface} ${modules}
                           ${services})

zephyr_library_include_directories(src/hw_interface)
zephyr_library_include_directories(src/events)
zephyr_library_include_directories(src/modules)
zephyr_library_include_directories(src/services)
