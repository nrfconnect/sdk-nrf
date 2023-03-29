#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(lwm2m_client_unittest)

FILE(GLOB app_sources src/*.c)
target_sources(app
  PRIVATE
    ${app_sources}
    $ENV{ZEPHYR_BASE}/subsys/net/lib/lwm2m/lwm2m_util.c
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
  -DCONFIG_LWM2M_CLIENT_UTILS_SIGNAL_MEAS_INFO_OBJ_SUPPORT
  -DCONFIG_LTE_LC_TAU_PRE_WARNING_NOTIFICATIONS
  -DCONFIG_LTE_PSM_REQ_RPTAU="10101010"
  -DCONFIG_LTE_PSM_REQ_RAT="00001111"
  -DCONFIG_LTE_EDRX_REQ_VALUE_LTE_M="0001"
  -DCONFIG_LTE_PTW_VALUE_LTE_M="0000"
  -DCONFIG_DOWNLOAD_CLIENT_BUF_SIZE=2048
  -DCONFIG_DOWNLOAD_CLIENT_STACK_SIZE=4096
  -DCONFIG_LWM2M_FIRMWARE_UPDATE_OBJ_SUPPORT
  -DCONFIG_LWM2M_FIRMWARE_UPDATE_PULL_SUPPORT
  -DCONFIG_DFU_TARGET_MCUBOOT
  -DCONFIG_FOTA_DOWNLOAD_MCUBOOT_FLASH_BUF_SZ=512
  -DCONFIG_LTE_PTW_VALUE_NBIOT="0000"
  -DCONFIG_LTE_EDRX_REQ_VALUE_NBIOT="0010"
  -DCONFIG_NET_MGMT_EVENT_INFO
  -DCONFIG_NET_MGMT_EVENT
  -DCONFIG_LWM2M_CLIENT_UTILS_RAI
)

target_compile_options(app
  PRIVATE
  ${options}
)
target_compile_options(..__nrf__subsys__net__lib__lwm2m_client_utils
  PRIVATE
  ${options}
)

set(includes
"$ENV{ZEPHYR_BASE}/subsys/net/lib/lwm2m/"
"$ENV{ZEPHYR_BASE}/../nrfxlib/nrf_modem/include/"
"$ENV{ZEPHYR_BASE}/../modules/lib/cjson/"
"$ENV{ZEPHYR_BASE}/../nrf/subsys/net/lib/nrf_cloud/include/"
"$ENV{ZEPHYR_BASE}/../nrf/subsys/net/lib/fota_download/include/"
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
