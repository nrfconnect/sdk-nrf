#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(lwm2m_codec_helpers)

# Turn off warnings of deprecated declarations
zephyr_compile_options(-Wno-deprecated-declarations)

# Enable 64 bit support in Unity
zephyr_compile_definitions(UNITY_SUPPORT_64)

# Generate runner for the test
test_runner_generate(src/lwm2m_codec_helpers_test.c)

set(ASSET_TRACKER_V2_DIR ../..)

# Create mocks
cmock_handle(${ZEPHYR_BASE}/include/zephyr/net/socket.h net)
cmock_handle(${ZEPHYR_BASE}/subsys/net/lib/lwm2m/lwm2m_engine.h lwm2m)
cmock_handle(${ZEPHYR_BASE}/include/zephyr/net/lwm2m.h lwm2m)
cmock_handle(${ZEPHYR_NRF_MODULE_DIR}/include/net/lwm2m_client_utils.h lwm2m_client_utils)
cmock_handle(${ZEPHYR_NRF_MODULE_DIR}/include/net/lwm2m_client_utils_location.h lwm2m_client_utils)
cmock_handle(${ZEPHYR_NRF_MODULE_DIR}/include/modem/lte_lc.h lte_lc
	FUNC_EXCLUDE ".*(lte_lc_rai_req|lte_lc_rai_param_set)"
	WORD_EXCLUDE "__deprecated")
cmock_handle(${ZEPHYR_NRF_MODULE_DIR}/include/date_time.h date_time)

# Add Unit Under Test source files
target_sources(app PRIVATE ${ASSET_TRACKER_V2_DIR}/src/cloud/cloud_codec/lwm2m/lwm2m_codec_helpers.c)

# Add test source file
target_sources(app PRIVATE src/lwm2m_codec_helpers_test.c)

# Add directories
target_include_directories(app PRIVATE ${ASSET_TRACKER_V2_DIR}/src/cloud/cloud_codec/)
target_include_directories(app PRIVATE ${ASSET_TRACKER_V2_DIR}/src/cloud/cloud_codec/lwm2m/)
target_include_directories(app PRIVATE ${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_modem/include/)
target_include_directories(app PRIVATE ${ZEPHYR_BASE}/subsys/net/lib/lwm2m/)
target_include_directories(app PRIVATE ${ZEPHYR_NRF_MODULE_DIR}/modules/cjson/include/)

# Options that cannot be passed through Kconfig fragments.
target_compile_options(app PRIVATE
	-DCONFIG_LWM2M_CLIENT_UTILS_SIGNAL_MEAS_INFO_OBJ_SUPPORT=y
	-DCONFIG_LWM2M_CLIENT_UTILS_GROUND_FIX_OBJ_SUPPORT=y
	-DCONFIG_LWM2M_CLIENT_UTILS_GNSS_ASSIST_OBJ_SUPPORT=y
	-DCONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_AGNSS=y
	-DCONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_PGPS=y
	-DCONFIG_CLOUD_CODEC_MANUFACTURER="nordicsemi"
	-DCONFIG_LWM2M_IPSO_PUSH_BUTTON_INSTANCE_COUNT=2
	-DCONFIG_APP_EVENT_MANAGER_MAX_EVENT_CNT=32
	-DCONFIG_CLOUD_CODEC_LWM2M_PATH_ENTRY_SIZE_MAX=12
	-DCONFIG_CLOUD_CODEC_LWM2M_PATH_LIST_ENTRIES_MAX=15
	-DCONFIG_CLOUD_CODEC_APN_LEN_MAX=50
	-DCONFIG_ASSET_TRACKER_V2_APP_VERSION_MAX_LEN=50
	-DCONFIG_LWM2M=y
	-DCONFIG_LWM2M_ENGINE_MAX_PENDING=5
	-DCONFIG_LWM2M_ENGINE_MAX_REPLIES=5
	-DCONFIG_LWM2M_ENGINE_VALIDATION_BUFFER_SIZE=128
	-DCONFIG_LWM2M_ENGINE_DEFAULT_LIFETIME=60
	-DCONFIG_LWM2M_CLIENT_UTILS_SECURITY_OBJ_SUPPORT=y
	-DCONFIG_LWM2M_CLIENT_UTILS_FIRMWARE_UPDATE_OBJ_SUPPORT=y
	-DCONFIG_LWM2M_ENGINE_MESSAGE_HEADER_SIZE=48
	-DCONFIG_LWM2M_COAP_BLOCK_SIZE=256
	-DCONFIG_LWM2M_COAP_MAX_MSG_SIZE=256
	-DCONFIG_CLOUD_CODEC_LWM2M_THINGY91_SENSORS=y
	-DCONFIG_LTE_NEIGHBOR_CELLS_MAX=10
)
