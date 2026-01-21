#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
zephyr_include_directories(include)

project(adc)

target_sources(app PRIVATE ./../common/main.c)
target_sources(app PRIVATE src/driver_test.c)

if(CONFIG_SOC_NRF54H20_CPUPPR)
  message(DEBUG "Power Mode handler for RISC V is included.")
  target_sources(app PRIVATE ${ZEPHYR_NRF_MODULE_DIR}/tests/benchmarks/multicore/common/power_off.c)
endif()
