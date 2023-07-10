#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bt_mesh_sensor_subsystem_test)

target_include_directories(app PUBLIC
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth/mesh
  ${ZEPHYR_BASE}/subsys/bluetooth
  )

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE
  ${app_sources}
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth/mesh/sensor_types.c
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth/mesh/sensor.c
  ${ZEPHYR_BASE}/subsys/bluetooth/mesh/msg.c
  )

target_compile_options(app
  PRIVATE
  -DCONFIG_BT_MESH_MODEL_KEY_COUNT=5
  -DCONFIG_BT_MESH_MODEL_GROUP_COUNT=5
  -DCONFIG_BT_MESH_SENSOR_ALL_TYPES=1
  -DCONFIG_BT_MESH_SENSOR_LABELS=1
  -DCONFIG_BT_MESH_SENSOR_CHANNELS_MAX=5
  -DCONFIG_BT_MESH_SENSOR_CHANNEL_ENCODED_SIZE_MAX=4
  -DCONFIG_BT_LOG_LEVEL=0
  -DCONFIG_BT_MESH_USES_TINYCRYPT
  )

zephyr_linker_sources(SECTIONS sensor_types.ld)
