#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(event_manager_proxy_remote)

add_subdirectory("../common_events" "${CMAKE_CURRENT_BINARY_DIR}/common_events")
add_subdirectory("../modules"       "${CMAKE_CURRENT_BINARY_DIR}/modules")

target_sources(app PRIVATE src/main.c)
