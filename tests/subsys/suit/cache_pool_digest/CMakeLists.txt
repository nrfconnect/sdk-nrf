#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(integration_test_cache_pool_digest)
include(../cmake/test_template.cmake)


zephyr_library_link_libraries(suit_stream_sinks_interface)
zephyr_library_link_libraries(suit_utils)
zephyr_library_link_libraries(suit_platform_interface)
zephyr_library_link_libraries(suit_execution_mode)
zephyr_library_link_libraries(suit_memptr_storage_interface)
