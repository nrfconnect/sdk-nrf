#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(i2c_slave)

FILE(GLOB app_sources ${ZEPHYR_BASE}/tests/boards/nrf/i2c/i2c_slave/src/*.c)
target_sources(app PRIVATE ${app_sources})
