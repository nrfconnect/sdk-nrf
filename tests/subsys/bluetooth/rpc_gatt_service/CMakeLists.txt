#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bluetooth_rpc_gatt_service_test)

FILE(GLOB app_sources src/*.c)

target_include_directories(app PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth/rpc/common
)

# Include the source file directly for unit testing
target_sources(app PRIVATE
  ${app_sources}
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth/rpc/common/bt_rpc_gatt_common.c
)

# Include test config header before compilation to define config values
# that aren't available via Kconfig without CONFIG_BT_RPC
target_compile_options(app PRIVATE
  -include ${CMAKE_CURRENT_SOURCE_DIR}/src/test_config.h
)
