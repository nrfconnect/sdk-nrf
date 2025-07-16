#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(contin_array)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

target_include_directories(app PRIVATE ./src)

target_compile_options(app
  PRIVATE
  -DCONFIG_BT_BUF_EVT_RX_COUNT=10
  -DCONFIG_BT_BUF_ACL_TX_COUNT=8
  )
