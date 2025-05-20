#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(spi_dt_spec)

target_sources(app PRIVATE ${ZEPHYR_BASE}/tests/drivers/spi/dt_spec/src/main.c)
