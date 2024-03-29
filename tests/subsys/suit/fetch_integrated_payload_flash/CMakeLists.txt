#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(integration_test_fetch_integrated_payload_flash)
include(../cmake/test_template.cmake)

zephyr_library_link_libraries(suit_storage_interface)
zephyr_library_link_libraries(suit_execution_mode)
