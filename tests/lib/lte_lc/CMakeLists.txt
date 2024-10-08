#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(lte_lc)

test_runner_generate(src/main.c)

cmock_handle(${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_modem/include/nrf_modem_at.h
	     FUNC_EXCLUDE ".*nrf_modem_at_scanf"
	     FUNC_EXCLUDE ".*nrf_modem_at_printf")

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

target_sources(app
  PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/lib/lte_link_control/lte_lc_helpers.c
)

target_include_directories(app
  PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/lib/lte_link_control/
  ${ZEPHYR_NRF_MODULE_DIR}/include/modem/
)

target_compile_options(app
  PRIVATE
  -DCONFIG_LTE_LINK_CONTROL_LOG_LEVEL=0
  -DCONFIG_LTE_NEIGHBOR_CELLS_MAX=10
)
