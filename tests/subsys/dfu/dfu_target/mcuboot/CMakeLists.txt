#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(dfu_target_test)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

target_sources(app
  PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/dfu/dfu_target/src/dfu_target.c
  )

target_include_directories(app
  PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/dfu/include
  )

target_compile_options(app
  PRIVATE
  -DCONFIG_IMG_BLOCK_BUF_SIZE=4096
  -DCONFIG_DFU_TARGET_LOG_LEVEL=2
  -DCONFIG_DFU_TARGET_MCUBOOT=1
  )
