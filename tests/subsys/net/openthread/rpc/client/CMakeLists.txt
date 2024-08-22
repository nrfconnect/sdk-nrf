#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ot_rpc_client_test)

FILE(GLOB app_sources src/*.c)

target_include_directories(app PRIVATE
  # Needed to access OpenThread RPC command IDs.
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/openthread/rpc/common
  ../common
  )

target_sources(app PRIVATE
  ${app_sources}
  ../common/net_l2_openthread.c
  ../common/nrf_rpc_single_thread.c
  )

# Fill the gaps due to not setting NET_L2_OPENTHREAD.
zephyr_include_directories(
  ${ZEPHYR_OPENTHREAD_MODULE_DIR}/include
)

zephyr_compile_definitions(
  CONFIG_OPENTHREAD_PKT_LIST_SIZE=1
)

# Enforce single-threaded nRF RPC command processing.
target_link_options(app PUBLIC
  -Wl,--wrap=nrf_rpc_os_init,--wrap=nrf_rpc_os_thread_pool_send
)
