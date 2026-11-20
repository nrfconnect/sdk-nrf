#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(bluetooth_utp)

# Reuse the sources of the Zephyr hci_uart_async sample.
FILE(GLOB hci_uart_async_sources $ENV{ZEPHYR_BASE}/samples/bluetooth/hci_uart_async/src/*.c)
target_sources(app PRIVATE ${hci_uart_async_sources})
