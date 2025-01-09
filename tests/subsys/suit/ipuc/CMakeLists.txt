#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(integration_test_ipuc)
include(../cmake/test_template.cmake)

target_compile_options(suit_memory_layout_interface INTERFACE -DSDFW_UPDATE_AREA_ADDR=0x1000)
target_compile_options(suit_memory_layout_interface INTERFACE -DSDFW_UPDATE_AREA_SIZE=0xFFFFE000)

zephyr_library_link_libraries(suit_ipuc)
zephyr_library_link_libraries(suit_memory_layout_interface)
