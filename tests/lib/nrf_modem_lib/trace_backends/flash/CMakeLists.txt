#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(flash)

target_include_directories(app PRIVATE src)

# Add test sources
target_sources(app PRIVATE src/main.c)

# Provide compile-time definitions for configs expected by the backend
target_compile_definitions(app PRIVATE
        CONFIG_NRF_MODEM_LIB_TRACE_FLASH_SECTORS=16
        CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_FLASH_BUF_SIZE=1024
        CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_FLASH_PARTITION_SIZE=0x10000
)

# Generate runner for the test
test_runner_generate(src/main.c)

# Add the actual flash backend implementation
target_sources(app PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/lib/nrf_modem_lib/trace_backends/flash/flash.c)
