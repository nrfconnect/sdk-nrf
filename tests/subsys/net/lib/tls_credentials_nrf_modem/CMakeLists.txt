#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(tls_credentials_test)

target_include_directories(app PRIVATE
  ${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_modem/include/
  ${ZEPHYR_BASE}/subsys/net/lib/tls_credentials
)

target_compile_definitions(app PRIVATE
  CONFIG_TLS_MAX_CREDENTIALS_NUMBER=10
  CONFIG_TLS_CREDENTIALS_LOG_LEVEL=3
)

target_sources(app PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/tls_credentials/tls_credentials_nrf_modem.c
  main.c
)
