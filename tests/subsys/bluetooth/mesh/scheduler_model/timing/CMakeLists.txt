#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bt_mesh_scheduler_model_test)

target_include_directories(app PUBLIC
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth/mesh
  ${ZEPHYR_BASE}/subsys/bluetooth
  ../common
  )

FILE(GLOB app_sources src/*.c)

target_sources(app PRIVATE
  ${app_sources}
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth/mesh/scheduler_srv.c
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth/mesh/time_util.c
  ../common/sched_test.c
  )

target_compile_options(app
  PRIVATE
  -DCONFIG_BT_MESH_MODEL_KEY_COUNT=5
  -DCONFIG_BT_MESH_MODEL_GROUP_COUNT=5
  -DCONFIG_BT_LOG_LEVEL=0
  -DCONFIG_BT_MESH_SCHEDULER_SRV=1
  -DCONFIG_BT_MESH_USES_TINYCRYPT
  )

zephyr_ld_options(
    ${LINKERFLAGPREFIX},--allow-multiple-definition
    )
