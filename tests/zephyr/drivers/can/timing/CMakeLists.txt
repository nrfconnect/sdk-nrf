#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(can_timing_extended)

FILE(GLOB app_sources ${ZEPHYR_BASE}/tests/drivers/can/timing/src/*.c)
target_sources(app PRIVATE ${app_sources})
