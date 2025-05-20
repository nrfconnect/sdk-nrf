#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)
set(KCONFIG_ROOT "\${ZEPHYR_BASE}/tests/drivers/i2c/i2c_target_api/Kconfig")
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(i2c_target_api)

target_sources(app PRIVATE ${ZEPHYR_BASE}/tests/drivers/i2c/i2c_target_api/common/i2c_virtual.c)
target_sources(app PRIVATE ${ZEPHYR_BASE}/tests/drivers/i2c/i2c_target_api/src/main.c)
