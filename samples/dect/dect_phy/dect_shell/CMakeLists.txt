#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(DESH)
target_include_directories(app PRIVATE src)

target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/shell.c)

add_subdirectory(src/print)
add_subdirectory(src/utils)
add_subdirectory_ifdef(CONFIG_DESH_DECT_PHY src/dect)
