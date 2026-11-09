#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(uart_nrfx)

zephyr_get(TEST_BOUNCE_BUF_SZ)
if(NOT DEFINED TEST_BOUNCE_BUF_SZ)
  set(TEST_BOUNCE_BUF_SZ 8)
endif()

# Stub headers for nrfx, pinctrl and DMM. They take precedence over the Zephyr headers.
target_include_directories(app BEFORE PRIVATE stubs)
target_include_directories(app PRIVATE src)
target_include_directories(app PRIVATE ${ZEPHYR_NRF_MODULE_DIR}/include/modem/)

# Provide compile-time definitions for configs expected by the backend
target_compile_definitions(app PRIVATE
  CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_UART_NRFX_CHUNK_SZ=16
  CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_UART_NRFX_BOUNCE_BUF_SZ=${TEST_BOUNCE_BUF_SZ}
  CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_UART_NRFX_TX_TIMEOUT_MS=20
)

# Generate runner for the test
test_runner_generate(src/main.c)

target_sources(app PRIVATE
  src/main.c
  src/fake_nrfx_uarte.c
)

# Add the unit under test
target_sources(app PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/lib/nrf_modem_lib/trace_backends/uart_nrfx/uart_nrfx.c)
