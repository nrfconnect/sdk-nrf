#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

include(../cmake/test_template.cmake)

project(suit_metadata)

target_include_directories(testbinary PRIVATE ${SUIT_SUBSYS_DIR}/metadata/include)

target_sources(testbinary PRIVATE
  src/main.c
  ${SUIT_SUBSYS_DIR}/metadata/src/suit_metadata.c
)
