#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(fota_download)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

target_sources(app
  PRIVATE
  ${ZEPHYR_BASE}/../nrf/subsys/net/lib/fota_download/src/fota_download.c
  )

target_include_directories(app
  PRIVATE
  ${ZEPHYR_BASE}/../nrf/include/net/
  ${ZEPHYR_BASE}/../nrf/subsys/dfu/include
  . # To get 'pm_config.h'
  )

target_compile_options(app
  PRIVATE
  -DCONFIG_DOWNLOAD_CLIENT_MAX_RESPONSE_SIZE=500
  -DCONFIG_DOWNLOAD_CLIENT_STACK_SIZE=500
  -DCONFIG_FW_MAGIC_LEN=32
  -DFIRMWARE_INFO_MAGIC=0xbabababa
  -DABI_INFO_MAGIC=0xdededede
  -DCONFIG_FW_FIRMWARE_INFO_OFFSET=0x200
  -DCONFIG_FOTA_DOWNLOAD_LOG_LEVEL=2
  )
