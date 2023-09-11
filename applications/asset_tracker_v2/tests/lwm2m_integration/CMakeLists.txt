#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(lwm2m_integration)

# Turn off warnings of deprecated declarations
zephyr_compile_options(-Wno-deprecated-declarations)

# Generate runner for the test
test_runner_generate(src/lwm2m_integration_test.c)

set(ASSET_TRACKER_V2_DIR ../..)

# Create mocks
cmock_handle(${ZEPHYR_BASE}/include/zephyr/net/socket.h net)
cmock_handle(${ZEPHYR_BASE}/subsys/net/lib/lwm2m/lwm2m_engine.h lwm2m)
cmock_handle(${ZEPHYR_BASE}/include/zephyr/net/lwm2m.h lwm2m)
cmock_handle(${ZEPHYR_NRF_MODULE_DIR}/include/net/lwm2m_client_utils.h lwm2m_client_utils)
cmock_handle(${ZEPHYR_NRF_MODULE_DIR}/include/net/lwm2m_client_utils_location.h lwm2m_client_utils)
cmock_handle(${ZEPHYR_NRF_MODULE_DIR}/include/modem/lte_lc.h lte_lc FUNC_EXCLUDE ".*(lte_lc_rai_req|lte_lc_rai_param_set)")

# Add Unit Under Test source files
target_sources(app PRIVATE ${ASSET_TRACKER_V2_DIR}/src/cloud/lwm2m_integration/lwm2m_integration.c)

target_include_directories(app PRIVATE .)
target_include_directories(app PRIVATE ${ASSET_TRACKER_V2_DIR}/src/)
target_include_directories(app PRIVATE ${ASSET_TRACKER_V2_DIR}/src/cloud/)
target_include_directories(app PRIVATE ${ASSET_TRACKER_V2_DIR}/src/cloud/lwm2_integration)
target_include_directories(app PRIVATE ${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_modem/include/)
target_include_directories(app PRIVATE ${ZEPHYR_BASE}/subsys/net/lib/lwm2m/)
target_include_directories(app PRIVATE ${ZEPHYR_NRF_MODULE_DIR}/include/zephyr/net/)
target_include_directories(app PRIVATE ${ZEPHYR_NRF_MODULE_DIR}/include/net/)
target_include_directories(app PRIVATE ${ZEPHYR_NRF_MODULE_DIR}/include/modem/)

# Add test source file
target_sources(app PRIVATE src/lwm2m_integration_test.c)

# Include paths
target_include_directories(app PRIVATE ${ASSET_TRACKER_v2_DIR}/src/cloud/cloud_wrapper.h)

# Options that cannot be passed through Kconfig fragments.
target_compile_options(app PRIVATE
	-DCONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_AGPS=y
	-DCONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_PGPS=y
	-DCONFIG_LWM2M_INTEGRATION_ENDPOINT_PREFIX=":urn:id:"
	-DCONFIG_LWM2M_INTEGRATION=y
	-DCONFIG_LWM2M=y
	-DCONFIG_LWM2M_ENGINE_MAX_PENDING=5
        -DCONFIG_LWM2M_ENGINE_MAX_REPLIES=5
	-DCONFIG_LWM2M_ENGINE_VALIDATION_BUFFER_SIZE=128
	-DCONFIG_LWM2M_ENGINE_DEFAULT_LIFETIME=60
	-DCONFIG_LWM2M_INTEGRATION_MODEM_MODE_REQUEST_RETRY_SECONDS=10
	-DCONFIG_NET_SOCKETS_POSIX_NAMES=1
	-DCONFIG_LWM2M_CLIENT_UTILS_SECURITY_OBJ_SUPPORT=y
	-DCONFIG_LWM2M_CLIENT_UTILS_FIRMWARE_UPDATE_OBJ_SUPPORT=y
	-DCONFIG_CLOUD_CLIENT_ID_USE_CUSTOM=y
	-DCONFIG_CLOUD_CLIENT_ID="test"
	-DCONFIG_LWM2M_ENGINE_MESSAGE_HEADER_SIZE=48
	-DCONFIG_LWM2M_COAP_BLOCK_SIZE=256
	-DCONFIG_LWM2M_COAP_MAX_MSG_SIZE=256
	-DCONFIG_LWM2M_FIRMWARE_UPDATE_PULL_SUPPORT=y
	-DCONFIG_LWM2M_FIRMWARE_UPDATE_OBJ_SUPPORT=y
)
