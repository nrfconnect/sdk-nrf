#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(twim_suspend)

target_sources(app PRIVATE src/main.c)

if(CONFIG_SOC_NRF54H20_CPUPPR)
  message(STATUS "Power Mode handler for RISC V is included.")
  target_sources(app PRIVATE ${ZEPHYR_NRF_MODULE_DIR}/tests/benchmarks/multicore/common/power_off.c)
endif()
