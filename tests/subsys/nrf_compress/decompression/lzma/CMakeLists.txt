#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(decompression)

target_sources(app PRIVATE src/main.c)

generate_inc_file_for_target(
  app
  ${ZEPHYR_NRFXLIB_MODULE_DIR}/tests/subsys/nrf_compress/decompression/dummy_data_input.txt.lzma
  ${ZEPHYR_BINARY_DIR}/include/generated/dummy_data_input.inc
  )

generate_inc_file_for_target(
  app
  ${ZEPHYR_NRFXLIB_MODULE_DIR}/tests/subsys/nrf_compress/decompression/dummy_data_input_too_large.txt.lzma
  ${ZEPHYR_BINARY_DIR}/include/generated/dummy_data_input_too_large.inc
  )

generate_inc_file_for_target(
  app
  ${ZEPHYR_NRFXLIB_MODULE_DIR}/tests/subsys/nrf_compress/decompression/dummy_data_input.txt.lzma1
  ${ZEPHYR_BINARY_DIR}/include/generated/dummy_data_input_lzma1.inc
  )
