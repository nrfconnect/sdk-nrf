#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(mcumgr_smp_client_test)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

target_sources(app
  PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/mcumgr_smp_client/src/mcumgr_smp_client.c
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/fota_download/src/util/fota_download_util.c
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/fota_download/src/util/fota_download_smp.c
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/download_client/src/parse.c
  )

set(includes
"src/"
${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/download_client/include
)

target_include_directories(app
  PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/dfu/include
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/fota_download/include
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/mcumgr_smp_client/include
  ${includes}
  )

target_compile_options(app
  PRIVATE
  -DCONFIG_IMG_BLOCK_BUF_SIZE=4096
  -DCONFIG_DFU_TARGET_LOG_LEVEL=2
  -DCONFIG_NRF_MCUMGR_SMP_CLIENT_LOG_LEVEL=2
  -DCONFIG_FOTA_DOWNLOAD_LOG_LEVEL=2
  -DCONFIG_FOTA_DOWNLOAD_FILE_NAME_LENGTH=128
  -DCONFIG_FOTA_DOWNLOAD_HOST_NAME_LENGTH=128
  -DCONFIG_FOTA_DOWNLOAD_RESOURCE_LOCATOR_LENGTH=512
  -DCONFIG_DOWNLOAD_CLIENT_BUF_SIZE=2048
  -DCONFIG_DOWNLOAD_CLIENT_STACK_SIZE=1024
  -DCONFIG_DFU_TARGET_SMP=1
  )

  zephyr_library_link_libraries(MCUBOOT_BOOTUTIL)
