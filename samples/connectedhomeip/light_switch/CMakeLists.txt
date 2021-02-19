#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})

project(chip-light-switch)

set(COMMON_ROOT ${CMAKE_CURRENT_LIST_DIR}/../common)

# NORDIC SDK APP START
target_include_directories(app PRIVATE src ${COMMON_ROOT}/src ${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app)

target_compile_options(app PRIVATE -Wno-deprecated-declarations)

target_sources(app PRIVATE
               src/app_task.cpp
               src/light_switch.cpp
               src/main.cpp
               ${COMMON_ROOT}/src/led_widget.cpp
               ${COMMON_ROOT}/src/thread_util.cpp)
# NORDIC SDK APP END
