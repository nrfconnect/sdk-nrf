#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.13.1)

set_property(GLOBAL PROPERTY
  hw_unique_key_partition_PM_HEX_FILE ${CMAKE_CURRENT_LIST_DIR}/dummy_huk.hex
  )

set(b0_OVERLAY_CONFIG
  ${CMAKE_CURRENT_SOURCE_DIR}/overlay-b0.conf
  )

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(NONE)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
