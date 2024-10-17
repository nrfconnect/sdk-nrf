#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(can_add)

target_include_directories(app PRIVATE $ENV{ZEPHYR_BASE}/tests/drivers/can/api/src/)

target_sources(app PRIVATE $ENV{ZEPHYR_BASE}/tests/drivers/can/api/src/common.c)
target_sources(app PRIVATE src/classic.c)
target_sources_ifdef(CONFIG_CAN_FD_MODE app PRIVATE src/canfd.c)
