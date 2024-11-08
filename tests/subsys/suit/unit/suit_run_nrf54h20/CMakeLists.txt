#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

include(../cmake/test_template.cmake)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../../mocks" "${PROJECT_BINARY_DIR}/test_mocks_sdfw")

project(plat_version)

target_include_directories(testbinary PRIVATE ${SUIT_SUBSYS_DIR}/platform/include)
target_include_directories(testbinary PRIVATE ${SUIT_SUBSYS_DIR}/platform/sdfw/include)
target_compile_options(test_interface INTERFACE -DCONFIG_SUIT_PLATFORM)

target_sources(testbinary PRIVATE
  src/main.c
  ${SUIT_SUBSYS_DIR}/platform/sdfw/src/runners/suit_run_nrf54h20.c
)
