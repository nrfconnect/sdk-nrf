#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hybrid_verify)

target_include_directories(app PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../ml_dsa/src
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/nrf_security/src/drivers/cracen/cracenpsa/src
)

target_sources(app PRIVATE src/main.c)
