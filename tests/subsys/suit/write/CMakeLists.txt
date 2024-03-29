#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(integration_test_copy)
include(../cmake/test_template.cmake)

zephyr_library_link_libraries(suit)
zephyr_library_link_libraries(suit_platform_interface)
zephyr_library_link_libraries(suit_memptr_storage_interface)
zephyr_library_link_libraries(suit_sink_selector_interface)
zephyr_library_link_libraries(suit_memory_layout_interface)
