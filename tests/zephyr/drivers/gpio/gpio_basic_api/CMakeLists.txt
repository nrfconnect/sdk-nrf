#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(gpio_basic_api)

FILE(GLOB app_sources ${ZEPHYR_BASE}/tests/drivers/gpio/gpio_basic_api/src/main.c ${ZEPHYR_BASE}/tests/drivers/gpio/gpio_basic_api/src/test*.c)
target_sources(app PRIVATE ${app_sources})
target_sources_ifdef(CONFIG_GPIO_EMUL app PRIVATE ${ZEPHYR_BASE}/tests/drivers/gpio/gpio_basic_api/src/gpio_emul_callback.c)
