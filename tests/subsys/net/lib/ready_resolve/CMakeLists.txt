#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ready_resolve_test)

target_sources(app PRIVATE src/main.c)

# Modem tracking uses a faked AT interface instead of the modem library
if(CONFIG_TEST_MODEM_READINESS)
  zephyr_include_directories(${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_modem/include)
endif()
