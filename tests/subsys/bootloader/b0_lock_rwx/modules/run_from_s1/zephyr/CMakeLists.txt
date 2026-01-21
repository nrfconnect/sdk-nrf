#
# Copyright (c) 2025 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

if(CONFIG_TEST_B0_LOCK_USE_S1)
  zephyr_library()
  zephyr_library_sources(src/run_from_s1.c)
endif()
