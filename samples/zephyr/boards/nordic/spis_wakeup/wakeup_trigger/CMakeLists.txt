#
# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(wakeup_trigger)

FILE(GLOB app_sources ${ZEPHYR_BASE}/samples/boards/nordic/spis_wakeup/wakeup_trigger/src/*.c)
target_sources(app PRIVATE ${app_sources})
