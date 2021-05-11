#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.13.1)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(bt_mesh_sensor_subsystem_test)

target_include_directories(app PUBLIC ${NRF_DIR}/subsys/bluetooth/mesh)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

zephyr_ld_options(
    ${LINKERFLAGPREFIX},--allow-multiple-definition
  )
  