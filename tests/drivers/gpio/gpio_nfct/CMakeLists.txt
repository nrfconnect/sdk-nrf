#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(gpio_nfct)

target_sources(app PRIVATE ${ZEPHYR_NRF_MODULE_DIR}/tests/drivers/gpio/gpio_more_loops/src/main.c)
