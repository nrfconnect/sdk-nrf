#
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

set(azure_sdk_for_c_SOURCE_DIR $ENV{ZEPHYR_BASE}/../modules/lib/azure-sdk-for-c)

zephyr_library()

# Turn off precondition checking and logging to reduce the code footprint and RAM usage
add_compile_definitions(AZ_NO_PRECONDITION_CHECKING)
add_compile_definitions(AZ_NO_LOGGING)

zephyr_library_sources(
	src/azure_iot_hub.c
	src/azure_iot_hub_mqtt.c

	# Azure SDK for C - CORE lib
	${azure_sdk_for_c_SOURCE_DIR}/sdk/src/azure/core/az_context.c
	${azure_sdk_for_c_SOURCE_DIR}/sdk/src/azure/core/az_json_reader.c
	${azure_sdk_for_c_SOURCE_DIR}/sdk/src/azure/core/az_json_token.c
	${azure_sdk_for_c_SOURCE_DIR}/sdk/src/azure/core/az_json_writer.c
	${azure_sdk_for_c_SOURCE_DIR}/sdk/src/azure/core/az_span.c

	# Azure SDK for C - IoT lib
	${azure_sdk_for_c_SOURCE_DIR}/sdk/src/azure/iot/az_iot_common.c
	${azure_sdk_for_c_SOURCE_DIR}/sdk/src/azure/iot/az_iot_hub_client.c
	${azure_sdk_for_c_SOURCE_DIR}/sdk/src/azure/iot/az_iot_hub_client_c2d.c
	${azure_sdk_for_c_SOURCE_DIR}/sdk/src/azure/iot/az_iot_hub_client_commands.c
	${azure_sdk_for_c_SOURCE_DIR}/sdk/src/azure/iot/az_iot_hub_client_methods.c
	${azure_sdk_for_c_SOURCE_DIR}/sdk/src/azure/iot/az_iot_hub_client_properties.c
	${azure_sdk_for_c_SOURCE_DIR}/sdk/src/azure/iot/az_iot_hub_client_telemetry.c
	${azure_sdk_for_c_SOURCE_DIR}/sdk/src/azure/iot/az_iot_hub_client_twin.c
)

zephyr_library_sources_ifdef(CONFIG_AZURE_IOT_HUB_DPS
	src/azure_iot_hub_dps.c
	${azure_sdk_for_c_SOURCE_DIR}/sdk/src/azure/iot/az_iot_provisioning_client.c
)

zephyr_include_directories(
	include
	${azure_sdk_for_c_SOURCE_DIR}/sdk/inc
)
