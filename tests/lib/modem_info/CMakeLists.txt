#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(modem_info)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

test_runner_generate(src/main.c)

target_sources(app
  PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/lib/modem_info/modem_info.c
)

zephyr_include_directories(${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_modem/include/)
zephyr_include_directories(${ZEPHYR_NRF_MODULE_DIR}/include/modem/)
zephyr_include_directories(${ZEPHYR_BASE}/subsys/testsuite/include)

target_compile_options(app
  PRIVATE
  -DCONFIG_MODEM_INFO_BUFFER_SIZE=128
  -DCONFIG_MODEM_INFO_MAX_AT_PARAMS_RSP=10
)
