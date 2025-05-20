#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(rtt)

# create mock
cmock_handle(${ZEPHYR_SEGGER_MODULE_DIR}/SEGGER/SEGGER_RTT.h)

# generate runner for the test
test_runner_generate(src/main.c)

target_include_directories(app PRIVATE src)

# add test file
target_sources(app PRIVATE src/main.c)

# add unit under test
target_sources(app PRIVATE ${ZEPHYR_NRF_MODULE_DIR}/lib/nrf_modem_lib/trace_backends/rtt/rtt.c)

# include paths
target_include_directories(app PRIVATE ${ZEPHYR_SEGGER_MODULE_DIR}/Config/)
target_include_directories(app PRIVATE ${ZEPHYR_NRF_MODULE_DIR}/include/modem/)
