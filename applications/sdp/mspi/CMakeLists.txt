#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(sdp_mspi)

sdp_assembly_generate("${CMAKE_SOURCE_DIR}/src/hrt/hrt.c")
sdp_assembly_check("${CMAKE_SOURCE_DIR}/src/hrt/hrt.c")
sdp_assembly_prepare_install("${CMAKE_SOURCE_DIR}/src/hrt/hrt.c")

target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/hrt/hrt.s)

add_dependencies(app asm_check)
