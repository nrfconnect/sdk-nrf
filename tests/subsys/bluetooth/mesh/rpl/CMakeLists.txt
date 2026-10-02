#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bt_mesh_rpl_ncs)

file(GLOB app_sources src/*.c)

target_sources(app
  PRIVATE
  ${app_sources}
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth/mesh/rpl.c
)

target_include_directories(app
  PRIVATE
  ${ZEPHYR_BASE}/subsys/bluetooth
  ${ZEPHYR_BASE}/subsys/bluetooth/mesh
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth/mesh
  ${ZEPHYR_MBEDTLS_MODULE_DIR}/include
)

target_compile_options(app
  PRIVATE
  -DCONFIG_BT_MESH_CRPL=8
  -DCONFIG_BT_LOG_LEVEL=0
  -DCONFIG_BT_MESH_RPL_STORAGE_MODE_ZMS=1
)
