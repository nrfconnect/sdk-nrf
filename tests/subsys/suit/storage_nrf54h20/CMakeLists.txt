#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(integration_test_suit_storage_nrf54h20)
include(../cmake/test_template.cmake)

# Link with the CMake target, that includes SUIT platform internal APIs header
zephyr_library_link_libraries(suit_utils)
zephyr_library_link_libraries(suit_storage_interface)
zephyr_library_link_libraries(suit_update_magic_values)
