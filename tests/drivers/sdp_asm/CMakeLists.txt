#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(sdp_asm)

set(sdp_files
  ${CMAKE_CURRENT_SOURCE_DIR}/src/add_1.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/add_10.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/add_100.c
)

sdp_assembly_install(app "${sdp_files}")

target_sources(app PRIVATE src/main.c)
