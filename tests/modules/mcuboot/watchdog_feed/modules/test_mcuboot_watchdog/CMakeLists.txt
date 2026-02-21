#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

if(CONFIG_TEST_MCUBOOT_WATCHDOG)
  zephyr_library()
  zephyr_library_sources(src/test_mcuboot_watchdog.c)
endif()
