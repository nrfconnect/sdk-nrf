#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hello_world)

sdp_assembly_generate("${CMAKE_CURRENT_SOURCE_DIR}/src/add_1.c;${CMAKE_CURRENT_SOURCE_DIR}/src/add_10.c;${CMAKE_CURRENT_SOURCE_DIR}/src/add_100.c")
sdp_assembly_check("${CMAKE_CURRENT_SOURCE_DIR}/src/add_1.c;${CMAKE_CURRENT_SOURCE_DIR}/src/add_10.c;${CMAKE_CURRENT_SOURCE_DIR}/src/add_100.c")
sdp_assembly_prepare_install("${CMAKE_CURRENT_SOURCE_DIR}/src/add_1.c;${CMAKE_CURRENT_SOURCE_DIR}/src/add_10.c;${CMAKE_CURRENT_SOURCE_DIR}/src/add_100.c")

target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/add_1.s)
target_sources(app PRIVATE src/add_10.s)
target_sources(app PRIVATE src/add_100.s)

add_dependencies(app asm_check)
