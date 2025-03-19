#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)
set(ZEPHYR_TEST_DIR "\${ZEPHYR_BASE}/tests/drivers/spi/spi_controller_peripheral/")
set(KCONFIG_ROOT "\${ZEPHYR_BASE}/tests/drivers/spi/spi_controller_peripheral/Kconfig")
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(spi_controller_peripheral)

target_sources(app PRIVATE ${ZEPHYR_BASE}/tests/drivers/spi/spi_controller_peripheral/src/main.c)
