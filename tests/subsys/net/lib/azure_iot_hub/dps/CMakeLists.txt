#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(azure_iot_hub_dps_test)

# Generate runner for the test
test_runner_generate(src/azure_iot_hub_dps_test.c)

# Create mock
cmock_handle(${NRF_DIR}/include/net/mqtt_helper.h)

# Add Unit Under Test source files
target_sources(app PRIVATE
        ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/azure_iot_hub/src/azure_iot_hub_dps.c

	# Azure SDK for C - CORE lib
	${ZEPHYR_AZURE_SDK_FOR_C_MODULE_DIR}/sdk/src/azure/core/az_context.c
	${ZEPHYR_AZURE_SDK_FOR_C_MODULE_DIR}/sdk/src/azure/core/az_json_reader.c
	${ZEPHYR_AZURE_SDK_FOR_C_MODULE_DIR}/sdk/src/azure/core/az_json_token.c
	${ZEPHYR_AZURE_SDK_FOR_C_MODULE_DIR}/sdk/src/azure/core/az_json_writer.c
	${ZEPHYR_AZURE_SDK_FOR_C_MODULE_DIR}/sdk/src/azure/core/az_log.c
	${ZEPHYR_AZURE_SDK_FOR_C_MODULE_DIR}/sdk/src/azure/core/az_precondition.c
	${ZEPHYR_AZURE_SDK_FOR_C_MODULE_DIR}/sdk/src/azure/core/az_span.c

	# Azure SDK for C - IoT lib
	${ZEPHYR_AZURE_SDK_FOR_C_MODULE_DIR}/sdk/src/azure/iot/az_iot_provisioning_client.c
	${ZEPHYR_AZURE_SDK_FOR_C_MODULE_DIR}/sdk/src/azure/iot/az_iot_common.c
	${ZEPHYR_AZURE_SDK_FOR_C_MODULE_DIR}/sdk/src/azure/iot/az_iot_provisioning_client.c
)

# Add test source file
target_sources(app PRIVATE src/azure_iot_hub_dps_test.c)

# Include paths
target_include_directories(app PRIVATE ${ZEPHYR_NRF_MODULE_DIR}/include/net/)
target_include_directories(app PRIVATE ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/azure_iot_hub/include/)
target_include_directories(app PRIVATE ${ZEPHYR_AZURE_SDK_FOR_C_MODULE_DIR}/sdk/inc/)

# Options that cannot be passed through Kconfig fragments.
target_compile_options(app PRIVATE
	-DCONFIG_AZURE_IOT_HUB_DPS_HOSTNAME="global.azure-devices-provisioning.net"
	-DCONFIG_AZURE_IOT_HUB_DPS_ID_SCOPE="TEST-ID-SCOPE"
	-DCONFIG_AZURE_IOT_HUB_DPS_REG_ID="test-device-registration-id"
	-DCONFIG_AZURE_IOT_HUB_DPS_TOPIC_BUFFER_SIZE=256
	-DCONFIG_AZURE_IOT_HUB_DPS_OPERATION_ID_BUFFER_SIZE=68
	-DCONFIG_AZURE_IOT_HUB_DPS_TIMEOUT_SEC=90
	-DCONFIG_AZURE_IOT_HUB_DPS_DEVICE_ID_MAX_LEN=128
	-DCONFIG_AZURE_IOT_HUB_DPS_HOSTNAME_MAX_LEN=64
	-DCONFIG_MQTT_HELPER_SEC_TAG=1
	-DCONFIG_MQTT_HELPER_PORT=8883
	-DCONFIG_AZURE_IOT_HUB_DPS_USER_NAME_BUFFER_SIZE=128
)
