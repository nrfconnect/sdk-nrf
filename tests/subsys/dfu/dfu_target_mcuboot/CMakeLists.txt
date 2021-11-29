#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(json)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

target_sources(app
  PRIVATE
  ${ZEPHYR_BASE}/../nrf/subsys/dfu/dfu_target/src/dfu_target_mcuboot.c
  )

target_include_directories(app
  PRIVATE
  ${ZEPHYR_BASE}/../nrf/subsys/dfu/include
  include # To get 'pm_config.h'
  ${ZEPHYR_BASE}/../nrf/include/dfu
  )

if (CONFIG_BOARD_NATIVE_POSIX)
  target_include_directories(app
    PRIVATE
    include/stub_nrfx # To get 'nrfx.h'
    )
endif()

target_compile_options(app
  PRIVATE
  -DCONFIG_IMG_BLOCK_BUF_SIZE=4096
  -DCONFIG_DFU_TARGET_LOG_LEVEL=2
  -DCONFIG_AWS_FOTA_FILE_PATH_MAX_LEN=1024
  )
