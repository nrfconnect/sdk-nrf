#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(clock_control_api)

FILE(GLOB app_sources ${ZEPHYR_BASE}/tests/drivers/clock_control/clock_control_api/src/*.c)
target_sources(app PRIVATE ${app_sources})
