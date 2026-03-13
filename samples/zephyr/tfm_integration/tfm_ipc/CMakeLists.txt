#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(tfm_ipc)

target_sources(app PRIVATE ${ZEPHYR_BASE}/samples/tfm_integration/tfm_ipc/src/main.c)

target_include_directories(app PRIVATE
  $<TARGET_PROPERTY:tfm,TFM_BINARY_DIR>/api_ns/interface/include
)
