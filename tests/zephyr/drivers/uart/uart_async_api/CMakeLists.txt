#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(uart_high_level_api)

target_sources(app PRIVATE ${ZEPHYR_BASE}/tests/drivers/uart/uart_async_api/src/test_uart_async.c)
