#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(dect_rpc_client_test)

file(GLOB app_sources src/*.c)

target_include_directories(app PRIVATE
  # Needed to access DECT NR+ RPC command/event IDs.
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/dect/rpc/common
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/dect/rpc/client
  ../common
)

target_sources(app PRIVATE
  ${app_sources}
  ../common/status_codec_helpers.c
  ../common/mock_dect_rpc_transport.c
)
