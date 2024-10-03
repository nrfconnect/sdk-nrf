#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(system_off)

target_sources(app PRIVATE $ENV{ZEPHYR_BASE}/samples/boards/nordic/system_off/src/main.c)
