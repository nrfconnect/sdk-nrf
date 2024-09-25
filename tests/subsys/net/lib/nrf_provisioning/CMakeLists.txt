#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(nrf_provisioning_unittest)

set(app_sources)

set(options
  -DCONFIG_NRF_MODEM_LIB_MEM_DIAG=y
  -DCONFIG_NRF_PROVISIONING_JWT_MAX_VALID_TIME_S=300
  -DCONFIG_MODEM_JWT_MAX_LEN=850
)

cmock_handle(${ZEPHYR_NRF_MODULE_DIR}/include/date_time.h)
cmock_handle(${ZEPHYR_NRF_MODULE_DIR}/include/modem/lte_lc.h
	FUNC_EXCLUDE ".*(lte_lc_rai_req|lte_lc_rai_param_set)"
	WORD_EXCLUDE "__deprecated")
cmock_handle(${ZEPHYR_NRF_MODULE_DIR}/include/modem/modem_key_mgmt.h)
cmock_handle(${ZEPHYR_NRF_MODULE_DIR}/include/modem/modem_attest_token.h)
cmock_handle(${ZEPHYR_NRF_MODULE_DIR}/include/modem/modem_info.h)
cmock_handle(${ZEPHYR_NRF_MODULE_DIR}/include/modem/nrf_modem_lib.h)
cmock_handle(${ZEPHYR_BASE}/include/zephyr/settings/settings.h)
cmock_handle(${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_provisioning/include/nrf_provisioning_at.h)
cmock_handle(${ZEPHYR_NRF_MODULE_DIR}/include/net/rest_client.h)
cmock_handle(include/nrf_modem_at.h)

if(CONFIG_SETTINGS_CUSTOM)
  test_runner_generate(src/http_jwt.c)
  list(APPEND app_sources src/http_jwt.c)
  cmock_handle(${ZEPHYR_NRF_MODULE_DIR}/include/modem/modem_jwt.h)

elseif(CONFIG_NRF_PROVISIONING_COAP)
  test_runner_generate(src/coap.c)
  list(APPEND app_sources src/coap.c)
  list (APPEND options
    -DCONFIG_COAP_CLIENT_MESSAGE_HEADER_SIZE=48
    -DCONFIG_COAP_CLIENT_MESSAGE_SIZE=512
    -DCONFIG_COAP_CLIENT_STACK_SIZE=1024
    -DCONFIG_NET_SOCKETS_POSIX_NAMES=y
    -DCONFIG_MODEM_INFO_BUFFER_SIZE=128
    -DCONFIG_COAP_CLIENT_MAX_REQUESTS=2
    -DCONFIG_COAP_CLIENT_BLOCK_SIZE=256
  )
  cmock_handle(${ZEPHYR_BASE}/include/zephyr/net/coap_client.h)
  cmock_handle(${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_provisioning/include/nrf_provisioning_jwt.h)

elseif(CONFIG_NRF_PROVISIONING_HTTP)
  test_runner_generate(src/main.c)
  list(APPEND app_sources src/main.c)
  cmock_handle(${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_provisioning/include/nrf_provisioning_jwt.h)

endif()

target_sources(app PRIVATE ${app_sources})

target_compile_options(app
  PRIVATE
  ${options}
)

target_compile_options(..__nrf__subsys__net__lib__nrf_provisioning
  PRIVATE
  ${options}
)

set(includes
  "include/" # Must be included first to redefine functions as non-static-inline
  "${ZEPHYR_NRF_MODULE_DIR}/include/"
  "${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_provisioning/include/"
  "${NRFXLIB_DIR}/nrf_modem/include/"
  "${ZEPHYR_BASE}/include/"
  "src/"
)

target_include_directories(app
  PRIVATE
  ${includes}
)

target_include_directories(..__nrf__subsys__net__lib__nrf_provisioning
  PRIVATE
  ${includes}
)

target_link_libraries(..__nrf__subsys__net__lib__nrf_provisioning
  PRIVATE
  "-Wl,--wrap=k_malloc,--wrap=k_free"
)
