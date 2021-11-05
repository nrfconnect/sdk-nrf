#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(modem_trace_test_uart)

# create mock
cmock_handle(${ZEPHYR_HAL_NORDIC_MODULE_DIR}/nrfx/drivers/include/nrfx_uarte.h )

# generate runner for the test
test_runner_generate(src/modem_trace_test.c)

target_include_directories(app PRIVATE src)

# add test file
target_sources(app PRIVATE src/modem_trace_test.c)

# add unit under test
target_sources(app PRIVATE ${NRF_DIR}/lib/nrf_modem_lib/nrf_modem_lib_trace.c)

# include paths
target_include_directories(app PRIVATE ${ZEPHYR_HAL_NORDIC_MODULE_DIR}/nrfx)
target_include_directories(app PRIVATE ${ZEPHYR_HAL_NORDIC_MODULE_DIR}/nrfx/mdk)
target_include_directories(app PRIVATE ${ZEPHYR_CMSIS_MODULE_DIR}/CMSIS/Core/Include/)
target_include_directories(app PRIVATE ${ZEPHYR_BASE}/modules/hal_nordic/nrfx)
target_include_directories(app PRIVATE ${NRF_DIR}/include/modem/)
target_include_directories(app PRIVATE ${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_modem/include/ )

# Define the device. Otherwise an "unknown device" warning will be triggered when compiling
# nrfx_config.h
target_compile_definitions(app PRIVATE NRF9160_XXAA)
# Define NRFX_DECLARE_ONLY to simplify generation of mocks. If this is not done, then the
# the compiler would be unable to compile the generated mocks for nrfx uart driver.
target_compile_definitions(app PRIVATE NRFX_DECLARE_ONLY)

target_compile_definitions(app PRIVATE NRFX_UARTE1_ENABLED)
