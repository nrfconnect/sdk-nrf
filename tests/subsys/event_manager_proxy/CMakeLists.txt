#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

set(ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_LIST_DIR})

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(event_manager_proxy)

add_subdirectory(common_config)
add_subdirectory(common_events)
add_subdirectory(modules)

target_include_directories(app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_sources(app PRIVATE
  src/test_init.c
  src/data.c
  src/simple.c
)
