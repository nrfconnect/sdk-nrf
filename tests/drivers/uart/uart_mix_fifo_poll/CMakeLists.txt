#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(uart_mix_fifo_poll)

target_sources(app PRIVATE ${ZEPHYR_BASE}/tests/drivers/uart/uart_mix_fifo_poll/src/main.c)
