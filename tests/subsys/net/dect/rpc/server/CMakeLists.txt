#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(dect_rpc_server_test)

file(GLOB app_sources src/*.c)

target_sources(app PRIVATE
  ${app_sources}
  ../common/status_codec_helpers.c
  ../common/mock_dect_rpc_transport.c
)

# CONFIG_DECT_MDM_DRIVER_MOCK: HAL lives in src/dect_mdm_mock.c (not under drivers/dect).
target_include_directories(app PRIVATE
  src
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/dect/rpc/common
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/dect/rpc/server
  ../common
)
