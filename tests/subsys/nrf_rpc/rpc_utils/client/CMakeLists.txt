#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf_rpc_dev_info_client_test)

FILE(GLOB app_sources src/*.c)

target_include_directories(app PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/nrf_rpc/rpc_utils/common
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/nrf_rpc/rpc_utils/client
  ../common
)

target_sources(app PRIVATE
  ${app_sources}
  ../common/nrf_rpc_single_thread.c
  src/die_temp_suite.c
)

# Enforce single-threaded nRF RPC command processing.
target_link_options(app PUBLIC
  -Wl,--wrap=nrf_rpc_os_init,--wrap=nrf_rpc_os_thread_pool_send
)
