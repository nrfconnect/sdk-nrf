#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(integration_test_suit_decompress_filter)
include(../cmake/test_template.cmake)

generate_inc_file_for_target(
  app
  ${ZEPHYR_NRFXLIB_MODULE_DIR}/tests/subsys/nrf_compress/decompression/dummy_data_input.txt.lzma
  ${ZEPHYR_BINARY_DIR}/include/generated/dummy_data_input.inc
  )

zephyr_library_link_libraries(suit_stream_filters_interface)
zephyr_library_link_libraries(suit_stream_sources_interface)
