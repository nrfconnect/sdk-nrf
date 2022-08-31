#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(NOT CONFIG_AZURE_SDK)
	return()
endif()

zephyr_library()

# Turn off precondition checking and logging to reduce the code footprint and RAM usage
zephyr_compile_definitions(
	AZ_NO_PRECONDITION_CHECKING
	AZ_NO_LOGGING
)

zephyr_include_directories(
	${ZEPHYR_AZURE_SDK_FOR_C_MODULE_DIR}/sdk/inc
)

zephyr_library_sources(
	# Azure SDK for C - CORE lib
	${ZEPHYR_AZURE_SDK_FOR_C_MODULE_DIR}/sdk/src/azure/core/az_context.c
	${ZEPHYR_AZURE_SDK_FOR_C_MODULE_DIR}/sdk/src/azure/core/az_json_reader.c
	${ZEPHYR_AZURE_SDK_FOR_C_MODULE_DIR}/sdk/src/azure/core/az_json_token.c
	${ZEPHYR_AZURE_SDK_FOR_C_MODULE_DIR}/sdk/src/azure/core/az_json_writer.c
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
	${ZEPHYR_AZURE_SDK_FOR_C_MODULE_DIR}/sdk/src/azure/iot/az_iot_provisioning_client.c
)
