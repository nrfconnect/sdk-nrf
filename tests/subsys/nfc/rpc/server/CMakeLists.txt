#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nfc_rpc_t2t_server_test)

FILE(GLOB app_sources src/*.c)

target_include_directories(app PRIVATE
  # Needed to access OpenThread RPC command IDs.
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/nfc/rpc/common
  ../common
)

target_sources(app PRIVATE
  ${app_sources}
  ../common/nrf_rpc_single_thread.c
)

zephyr_include_directories(${ZEPHYR_NRFXLIB_MODULE_DIR}/nfc/include/)

# Enforce single-threaded nRF RPC command processing.
target_link_options(app PUBLIC
  -Wl,--wrap=nrf_rpc_os_init,--wrap=nrf_rpc_os_thread_pool_send
)
