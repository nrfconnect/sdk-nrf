#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(dfu_target_suit_test)
include(${ZEPHYR_NRF_MODULE_DIR}/tests/subsys/suit/cmake/test_template.cmake)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

target_include_directories(app
  PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/dfu/include
  "src"
  )
