#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bt_mesh_ligth_hue_subsystem_test)

FILE(GLOB app_sources src/*.c)

target_sources(app
  PRIVATE
  ${app_sources}
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth/mesh/light_hue_srv.c
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth/mesh/model_utils.c
  )

target_include_directories(app
  PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth/mesh
  ${ZEPHYR_BASE}/subsys/bluetooth
  )

target_compile_options(app
  PRIVATE
  -DCONFIG_BT_MESH_MOD_ACKD_TIMEOUT_BASE=3000
  -DCONFIG_BT_MESH_MOD_ACKD_TIMEOUT_PER_HOP=50
  -DCONFIG_BT_LOG_LEVEL=0
  -DCONFIG_BT_MESH_USES_TINYCRYPT
)

zephyr_ld_options(
    ${LINKERFLAGPREFIX},--allow-multiple-definition
    )
