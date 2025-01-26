#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(modem_key_mgmt)

target_sources(app PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/lib/modem_key_mgmt/modem_key_mgmt.c
  src/main.c
)

zephyr_include_directories(${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_modem/include/)
zephyr_include_directories(${ZEPHYR_NRF_MODULE_DIR}/include/modem/)

target_compile_options(app PRIVATE
  -DCONFIG_MODEM_KEY_MGMT_LOG_LEVEL=3
)
