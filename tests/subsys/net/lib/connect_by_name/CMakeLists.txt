#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(connect_by_name_test)

target_sources(app PRIVATE src/main.c)

# Modem readiness depends on the modem library, so enable it here with a faked AT interface
if(CONFIG_TEST_MODEM_READINESS)
  zephyr_compile_definitions(
    CONFIG_CONNECT_BY_NAME_MODEM_READINESS=1
    CONFIG_CONNECT_BY_NAME_MODEM_POLL_INTERVAL_MS=10
  )
  zephyr_include_directories(${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_modem/include)
endif()
