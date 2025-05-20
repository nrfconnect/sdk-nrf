#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

include(../cmake/test_template.cmake)

project(suit_decrypt_filter)
target_include_directories(testbinary PRIVATE
    ${SUIT_SUBSYS_DIR}/stream/stream_filters/include
    ${SUIT_SUBSYS_DIR}/utils/include/
)

target_sources(testbinary PRIVATE
  src/main.c
  ${SUIT_SUBSYS_DIR}/stream/stream_filters/src/suit_decrypt_filter.c
)
