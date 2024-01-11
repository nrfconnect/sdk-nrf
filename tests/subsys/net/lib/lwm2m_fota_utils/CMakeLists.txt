#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(lwm2m_client_fota_unittest)



FILE(GLOB app_sources src/*.c )
target_sources(app
  PRIVATE
    ${app_sources}
    ${ZEPHYR_BASE}/subsys/net/lib/lwm2m/lwm2m_registry.c
    ${ZEPHYR_BASE}/subsys/net/lib/lwm2m/lwm2m_obj_firmware.c
    ${ZEPHYR_BASE}/subsys/net/lib/lwm2m/lwm2m_util.c
)


set(options
  -DCONFIG_LWM2M_ENGINE_MAX_PENDING=5
  -DCONFIG_LWM2M_ENGINE_MAX_REPLIES=5
  -DCONFIG_LWM2M_ENGINE_VALIDATION_BUFFER_SIZE=100
  -DCONFIG_LWM2M_DTLS_SUPPORT
  -DCONFIG_LWM2M_COAP_BLOCK_SIZE=512
  -DCONFIG_LWM2M_COAP_MAX_MSG_SIZE=512
  -DCONFIG_LWM2M_ENGINE_MESSAGE_HEADER_SIZE=10
  -DCONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP
  -DCONFIG_LWM2M_CLIENT_UTILS_NEIGHBOUR_CELL_LISTENER
  -DCONFIG_LWM2M_CLIENT_UTILS_CONN_MON_OBJ_SUPPORT
  -DCONFIG_LTE_LC_TAU_PRE_WARNING_NOTIFICATIONS
  -DCONFIG_DOWNLOAD_CLIENT_BUF_SIZE=2048
  -DCONFIG_DOWNLOAD_CLIENT_STACK_SIZE=4096
  -DCONFIG_LWM2M_FIRMWARE_UPDATE_OBJ_SUPPORT
  -DCONFIG_LWM2M_FIRMWARE_UPDATE_PULL_SUPPORT
  -DCONFIG_DFU_TARGET_MCUBOOT=y
  -DCONFIG_DFU_TARGET_MODEM_DELTA=y
  -DCONFIG_FOTA_DOWNLOAD_MCUBOOT_FLASH_BUF_SZ=512
  -DCONFIG_LWM2M_VERSION_1_0=y
  -DCONFIG_LWM2M_ENGINE_DEFAULT_LIFETIME=43200
)

target_compile_options(app
  PRIVATE
  ${options}
)
target_compile_options(..__nrf__subsys__net__lib__lwm2m_client_utils
  PRIVATE
  ${options}
)

target_compile_options(..__nrf__subsys__dfu__dfu_target
  PRIVATE
  ${options}
)

set(includes
"${ZEPHYR_BASE}/subsys/net/lib/lwm2m/"
"${ZEPHYR_BASE}/../nrfxlib/nrf_modem/include/"
"${ZEPHYR_BASE}/../nrf/subsys/net/lib/fota_download/include/"
"src/"
)

target_include_directories(..__nrf__subsys__net__lib__lwm2m_client_utils
  PRIVATE
  ${includes}
)

target_include_directories(app
  PRIVATE
  ${includes}
)
