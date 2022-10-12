#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(lwm2m_client_unittest)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

set(options
  -DCONFIG_LWM2M_ENGINE_MAX_PENDING=5
  -DCONFIG_LWM2M_ENGINE_MAX_REPLIES=5
  -DCONFIG_LWM2M_ENGINE_VALIDATION_BUFFER_SIZE=100
  -DCONFIG_LWM2M_DTLS_SUPPORT
  -DCONFIG_LWM2M_COAP_BLOCK_SIZE=512
  -DCONFIG_LWM2M_COAP_MAX_MSG_SIZE=512
  -DCONFIG_LWM2M_ENGINE_MESSAGE_HEADER_SIZE=10
  -DCONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP
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
)

target_include_directories(..__nrf__subsys__net__lib__lwm2m_client_utils
  PRIVATE
  ${includes}
)

target_include_directories(app
  PRIVATE
  ${includes}
)
