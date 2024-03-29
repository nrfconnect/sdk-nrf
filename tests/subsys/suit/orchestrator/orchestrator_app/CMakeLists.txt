#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(integration_test_suit_orchestrator_app)
include(../../cmake/test_template.cmake)

FILE(GLOB manifest_sources ../manifest/src/*.c)
target_sources(app PRIVATE
  ${manifest_sources}
  )

# Link with the CMake target, that includes SUIT platform internal APIs header
zephyr_library_link_libraries(suit_utils)
if(CONFIG_TESTS_SUIT_ORCHESTRATOR_APP_FULL_PROCESSING)
  zephyr_library_link_libraries(suit_cache_interface)
  zephyr_library_link_libraries(suit_stream_sources_interface)
endif()
