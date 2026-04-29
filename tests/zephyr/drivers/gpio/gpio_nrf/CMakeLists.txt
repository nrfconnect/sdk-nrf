#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(gpio_nrf_specific)

target_sources(app PRIVATE ${ZEPHYR_BASE}/tests/drivers/gpio/gpio_nrf/src/main.c)
