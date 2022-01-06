#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(mqtt-simple)

set(azure_sdk_for_c_SOURCE_DIR $ENV{ZEPHYR_BASE}/../modules/lib/azure-sdk-for-c/sdk)

target_include_directories(app PRIVATE ${azure_sdk_for_c_SOURCE_DIR}/inc)

# NORDIC SDK APP START
target_sources(app PRIVATE
    # Azure SDK for C - CORE lib
    ${azure_sdk_for_c_SOURCE_DIR}/src/azure/core/az_context.c
    ${azure_sdk_for_c_SOURCE_DIR}/src/azure/core/az_http_pipeline.c
    ${azure_sdk_for_c_SOURCE_DIR}/src/azure/core/az_http_policy.c
    ${azure_sdk_for_c_SOURCE_DIR}/src/azure/core/az_http_policy_logging.c
    ${azure_sdk_for_c_SOURCE_DIR}/src/azure/core/az_http_policy_retry.c
    ${azure_sdk_for_c_SOURCE_DIR}/src/azure/core/az_http_request.c
    ${azure_sdk_for_c_SOURCE_DIR}/src/azure/core/az_http_response.c
    ${azure_sdk_for_c_SOURCE_DIR}/src/azure/core/az_json_reader.c
    ${azure_sdk_for_c_SOURCE_DIR}/src/azure/core/az_json_token.c
    ${azure_sdk_for_c_SOURCE_DIR}/src/azure/core/az_json_writer.c
    ${azure_sdk_for_c_SOURCE_DIR}/src/azure/core/az_log.c
    ${azure_sdk_for_c_SOURCE_DIR}/src/azure/core/az_precondition.c
    ${azure_sdk_for_c_SOURCE_DIR}/src/azure/core/az_span.c

    # Azure SDK for C - IoT lib
    ${azure_sdk_for_c_SOURCE_DIR}/src/azure/iot/az_iot_provisioning_client.c
    ${azure_sdk_for_c_SOURCE_DIR}/src/azure/iot/az_iot_common.c
    ${azure_sdk_for_c_SOURCE_DIR}/src/azure/iot/az_iot_hub_client.c
    ${azure_sdk_for_c_SOURCE_DIR}/src/azure/iot/az_iot_provisioning_client.c
    ${azure_sdk_for_c_SOURCE_DIR}/src/azure/iot/az_iot_common.c
    ${azure_sdk_for_c_SOURCE_DIR}/src/azure/iot/az_iot_hub_client_commands.c

    # Application sources
    src/main.c)
# NORDIC SDK APP END
