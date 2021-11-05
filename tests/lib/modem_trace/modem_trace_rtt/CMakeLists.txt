#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(modem_trace_test_rtt)

# create mock
cmock_handle(${ZEPHYR_SEGGER_MODULE_DIR}/SEGGER/SEGGER_RTT.h )

# generate runner for the test
test_runner_generate(src/modem_trace_test.c)

target_include_directories(app PRIVATE src)

# add test file
target_sources(app PRIVATE src/modem_trace_test.c)

# add unit under test
target_sources(app PRIVATE ${NRF_DIR}/lib/nrf_modem_lib/nrf_modem_lib_trace.c)

# include paths
target_include_directories(app PRIVATE ${ZEPHYR_SEGGER_MODULE_DIR}/Config/)
target_include_directories(app PRIVATE ${NRF_DIR}/include/modem/)
target_include_directories(app PRIVATE ${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_modem/include/ )
