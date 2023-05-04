#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(modem_battery)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

test_runner_generate(src/main.c)

target_sources(app
  PRIVATE
  ${NRF_DIR}/lib/modem_battery/modem_battery.c
)

zephyr_include_directories(${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_modem/include/)
zephyr_include_directories(${NRF_DIR}/include/modem/)
zephyr_include_directories(${ZEPHYR_BASE}/subsys/testsuite/include)

target_compile_options(app
  PRIVATE
  -DCONFIG_MODEM_BATTERY_LOW_LEVEL=3300
  -DCONFIG_MODEM_BATTERY_POFWARN_VOLTAGE=30
)
