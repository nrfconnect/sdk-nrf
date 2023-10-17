#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(pdn)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

test_runner_generate(src/main.c)

target_sources(app
  PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/lib/pdn/pdn.c
  ${ZEPHYR_NRF_MODULE_DIR}/lib/pdn/esm.c
)

zephyr_include_directories(${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_modem/include/)
zephyr_include_directories(${ZEPHYR_NRF_MODULE_DIR}/include/modem/)
zephyr_include_directories(${ZEPHYR_BASE}/subsys/testsuite/include)

add_compile_definitions(CONFIG_PDN_ESM_TIMEOUT=1000)
