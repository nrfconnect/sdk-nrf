#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)
# set(KCONFIG_ROOT ${ZEPHYR_NRF_MODULE_DIR}/samples/nfc/system_off/Kconfig)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(system_off)

target_sources(app PRIVATE ${ZEPHYR_NRF_MODULE_DIR}/samples/nfc/system_off/src/main.c)
