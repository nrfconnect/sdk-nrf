#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: BSD-5-Clause-Nordic
#

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

target_sources(app PRIVATE
  src/main.c
  ../gatt/nus.c
)

zephyr_library_include_directories($ENV{ZEPHYR_BASE}/samples/bluetooth/)
zephyr_library_include_directories(${CMAKE_CURRENT_SOURCE_DIR})
