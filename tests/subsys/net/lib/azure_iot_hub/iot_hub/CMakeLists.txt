#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(azure_iot_hub_test)

# Generate runner for the test
test_runner_generate(src/azure_iot_hub_test.c)

# Create mock
cmock_handle(${ZEPHYR_NRF_MODULE_DIR}/include/net/mqtt_helper.h)

# Add Unit Under Test source files
target_sources(app PRIVATE
        ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/azure_iot_hub/src/azure_iot_hub.c

	# Azure SDK for C - CORE lib
	${ZEPHYR_AZURE_SDK_FOR_C_MODULE_DIR}/sdk/src/azure/core/az_context.c
	${ZEPHYR_AZURE_SDK_FOR_C_MODULE_DIR}/sdk/src/azure/core/az_json_reader.c
	${ZEPHYR_AZURE_SDK_FOR_C_MODULE_DIR}/sdk/src/azure/core/az_json_token.c
	${ZEPHYR_AZURE_SDK_FOR_C_MODULE_DIR}/sdk/src/azure/core/az_json_writer.c
	${ZEPHYR_AZURE_SDK_FOR_C_MODULE_DIR}/sdk/src/azure/core/az_log.c
	${ZEPHYR_AZURE_SDK_FOR_C_MODULE_DIR}/sdk/src/azure/core/az_precondition.c
	${ZEPHYR_AZURE_SDK_FOR_C_MODULE_DIR}/sdk/src/azure/core/az_span.c

	# Azure SDK for C - IoT lib
	${ZEPHYR_AZURE_SDK_FOR_C_MODULE_DIR}/sdk/src/azure/iot/az_iot_common.c
	${ZEPHYR_AZURE_SDK_FOR_C_MODULE_DIR}/sdk/src/azure/iot/az_iot_hub_client.c
	${ZEPHYR_AZURE_SDK_FOR_C_MODULE_DIR}/sdk/src/azure/iot/az_iot_hub_client_c2d.c
	${ZEPHYR_AZURE_SDK_FOR_C_MODULE_DIR}/sdk/src/azure/iot/az_iot_hub_client_commands.c
	${ZEPHYR_AZURE_SDK_FOR_C_MODULE_DIR}/sdk/src/azure/iot/az_iot_hub_client_methods.c
	${ZEPHYR_AZURE_SDK_FOR_C_MODULE_DIR}/sdk/src/azure/iot/az_iot_hub_client_properties.c
	${ZEPHYR_AZURE_SDK_FOR_C_MODULE_DIR}/sdk/src/azure/iot/az_iot_hub_client_telemetry.c
	${ZEPHYR_AZURE_SDK_FOR_C_MODULE_DIR}/sdk/src/azure/iot/az_iot_hub_client_twin.c
)

# Add test source file
target_sources(app PRIVATE src/azure_iot_hub_test.c)

# Include paths
target_include_directories(app PRIVATE ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/azure_iot_hub/include/)
target_include_directories(app PRIVATE ${ZEPHYR_AZURE_SDK_FOR_C_MODULE_DIR}/sdk/inc/)

# # Preinclude file to the module under test to redefine IS_ENABLED() macro
# # which is used in the module.
# set_property(SOURCE ${ZEPHYR_BASE}/include/net/mqtt.h PROPERTY COMPILE_FLAGS
	# "-DCONFIG_MQTT_LIB_TLS")

# # Options that cannot be passed through Kconfig fragments.
target_compile_options(app PRIVATE
	-DCONFIG_AZURE_IOT_HUB_HOSTNAME="test-hostname.azure-devices.net"
        -DCONFIG_MQTT_HELPER_PORT=8883
	-DCONFIG_MQTT_HELPER_SEND_TIMEOUT_SEC=60
	-DCONFIG_MQTT_HELPER_STATIC_IP_ADDRESS=""
	-DCONFIG_MQTT_HELPER_RX_TX_BUFFER_SIZE=256
	-DCONFIG_MQTT_HELPER_PAYLOAD_BUFFER_LEN=2304
	-DCONFIG_AZURE_IOT_HUB_DPS_DEVICE_ID_MAX_LEN=30
	-DCONFIG_AZURE_IOT_HUB_DPS_HOSTNAME_MAX_LEN=64
	-DCONFIG_MQTT_HELPER_STACK_SIZE=2560
	-DCONFIG_AZURE_IOT_HUB_TOPIC_MAX_LEN=180
	-DCONFIG_AZURE_IOT_HUB_USER_NAME_BUF_SIZE=128
	-DCONFIG_AZURE_IOT_HUB_MSG_PROPERTY_RECV_MAX_COUNT=2
	-DCONFIG_AZURE_IOT_HUB_MSG_PROPERTY_BUFFER_SIZE=128
	-DCONFIG_MQTT_HELPER_SEC_TAG=1
	-DCONFIG_AZURE_IOT_HUB_DEVICE_ID="test-device-id"
)
