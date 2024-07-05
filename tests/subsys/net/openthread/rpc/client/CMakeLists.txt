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
  )

target_sources(app PRIVATE
  ${app_sources}
  )
