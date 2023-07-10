#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf_modem_lib_trace)

# create mock
cmock_handle(${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_modem/include/nrf_modem.h)
cmock_handle(${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_modem/include/nrf_modem_os.h)
cmock_handle(${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_modem/include/nrf_modem_trace.h)
cmock_handle(trace_backend_mock.h)

# generate runner for the test
test_runner_generate(src/main.c)

target_include_directories(app PRIVATE src)

# add test file
target_sources(app PRIVATE src/main.c)

# add mock for backend
target_sources(app PRIVATE trace_backend_mock.c)

# add unit under test
target_sources(app PRIVATE ${ZEPHYR_NRF_MODULE_DIR}/lib/nrf_modem_lib/nrf_modem_lib_trace.c)

# include paths
target_include_directories(app PRIVATE ${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_modem/include/)
target_include_directories(app PRIVATE ${ZEPHYR_NRF_MODULE_DIR}/include/modem/)
zephyr_include_directories(${ZEPHYR_BASE}/subsys/testsuite/include)

# Required for calling libmodem hooks
zephyr_linker_sources(RODATA ${ZEPHYR_NRF_MODULE_DIR}/lib/nrf_modem_lib/nrf_modem_lib.ld)
