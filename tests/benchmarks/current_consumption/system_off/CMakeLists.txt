#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)
set(KCONFIG_ROOT $ENV{ZEPHYR_BASE}/samples/boards/nordic/system_off/Kconfig)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(system_off)

target_sources(app PRIVATE $ENV{ZEPHYR_BASE}/samples/boards/nordic/system_off/src/main.c)
if(CONFIG_APP_USE_RETAINED_MEM)
  target_sources(app PRIVATE $ENV{ZEPHYR_BASE}/samples/boards/nordic/system_off/src/retained.c)
endif()
