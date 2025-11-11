#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf_cloud_init_uninit_test)

# NRF CLOUD TEST START
target_sources(app PRIVATE src/main.c)

# nrf_cloud.c is only included with the poll thread,
# so it needs to be included when the thread is disabled
if(NOT CONFIG_NRF_CLOUD_CONNECTION_POLL_THREAD)
  target_sources(app PRIVATE
    ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/mqtt/src/nrf_cloud.c
  )
endif()

target_include_directories(app PRIVATE
  src
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/common/include
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/common/src
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/mqtt/include
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/mqtt/src
  ${ZEPHYR_BASE}/subsys/testsuite/include
  ${ZEPHYR_CJSON_MODULE_DIR}
)

if(CONFIG_NRF_CLOUD_MQTT OR CONFIG_NRF_CLOUD_FOTA OR CONFIG_NRF_MODEM_LIB)
  if(CONFIG_NRF_CLOUD_MQTT OR CONFIG_NRF_CLOUD_FOTA)
    # These code files are mocked and/or not necessary to be included,
    # also prevent nrf_cloud.c to be included multiple times by KConfig
    set_source_files_properties(
      ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/mqtt/src/nrf_cloud.c
      ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/common/src/nrf_cloud_mem.c
      ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/mqtt/src/nrf_cloud_fota.c
      ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/common/src/nrf_cloud_fota_common.c
      ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/common/src/nrf_cloud_codec_internal.c
      ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/mqtt/src/nrf_cloud_codec_internal.c
      ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/mqtt/src/nrf_cloud_fsm.c
      ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/mqtt/src/nrf_cloud_transport.c
      ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/common/src/nrf_cloud_codec.c
      ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/common/src/nrf_cloud_info.c
      DIRECTORY ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/
      PROPERTIES HEADER_FILE_ONLY ON
    )

    # This code file has functions/structs/variables defined when
    # POSIX_API=y in zephyr/net/socket.h, so it needs to be excluded
    # when POSIX_API=n
    set_source_files_properties(
      ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/downloader/src/downloader.c
      DIRECTORY ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/downloader/
      PROPERTIES HEADER_FILE_ONLY ON
    )
  endif()

  if(CONFIG_NRF_MODEM_LIB)
    # Similarly, these files need to be excluded when POSIX_API=n
    set_source_files_properties(
      ${ZEPHYR_NRF_MODULE_DIR}/lib/nrf_modem_lib/sanity.c
      ${ZEPHYR_NRF_MODULE_DIR}/lib/nrf_modem_lib/nrf9x_sockets.c
      DIRECTORY ${ZEPHYR_NRF_MODULE_DIR}/lib/nrf_modem_lib/
      PROPERTIES HEADER_FILE_ONLY ON
    )
  else()
    # This code file uses NRF_MODEM_LIB, should not be included when it is off
    set_source_files_properties(
      ${ZEPHYR_NRF_MODULE_DIR}/subsys/dfu/dfu_target/src/dfu_target_modem_delta.c
      DIRECTORY ${ZEPHYR_NRF_MODULE_DIR}/subsys/dfu/dfu_target/
      PROPERTIES HEADER_FILE_ONLY ON
    )
  endif()
endif()
# NRF CLOUD TEST END

target_compile_options(app
  PRIVATE
  # Set the macro explicitly to log level DBG
  -DCONFIG_NRF_CLOUD_LOG_LEVEL=4
)
