#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(fota_download)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

target_sources(app
  PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/fota_download/src/fota_download.c
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/fota_download/src/util/fota_download_util.c
  )

target_include_directories(app
  PRIVATE
  src
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/fota_download/include
  ${ZEPHYR_NRF_MODULE_DIR}/include/net/
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/dfu/include
  . # To get 'pm_config.h'
  )

target_compile_options(app
  PRIVATE
  -DCONFIG_DOWNLOAD_CLIENT_BUF_SIZE=500
  -DCONFIG_DOWNLOAD_CLIENT_STACK_SIZE=500
  -DCONFIG_DOWNLOAD_CLIENT_MAX_FILENAME_SIZE=192
  -DCONFIG_FW_MAGIC_LEN=32
  -DABI_INFO_MAGIC=0xdededede
  -DCONFIG_FW_FIRMWARE_INFO_OFFSET=0x200
  -DCONFIG_FOTA_DOWNLOAD_LOG_LEVEL=2
  -DCONFIG_FOTA_SOCKET_RETRIES=2
  -DCONFIG_FW_INFO_MAGIC_LEN=12
  -DCONFIG_FOTA_DOWNLOAD_RESOURCE_LOCATOR_LENGTH=512
  -DCONFIG_FOTA_DOWNLOAD_FILE_NAME_LENGTH=128
  -DCONFIG_FOTA_DOWNLOAD_HOST_NAME_LENGTH=128
  -DCONFIG_FOTA_DOWNLOAD_SEC_TAG_LIST_SIZE_MAX=5
  ${info_magic}
  ${ext_api_magic}
  )
