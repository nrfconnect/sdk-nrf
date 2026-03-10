#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(mbox_error_cases)

target_sources(app PRIVATE ${ZEPHYR_BASE}/tests/drivers/mbox/mbox_error_cases/src/main.c)
