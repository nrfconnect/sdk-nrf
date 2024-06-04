/*
 *
 *    Copyright (c) 2022 Project CHIP Authors
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

// THIS FILE IS GENERATED BY ZAP

// Prevent multiple inclusion
#pragma once

/**** Cluster endpoint counts ****/
#define MATTER_DM_IDENTIFY_CLUSTER_CLIENT_ENDPOINT_COUNT (1)
#define MATTER_DM_IDENTIFY_CLUSTER_SERVER_ENDPOINT_COUNT (2)
#define MATTER_DM_GROUPS_CLUSTER_SERVER_ENDPOINT_COUNT (1)
#define MATTER_DM_ON_OFF_CLUSTER_CLIENT_ENDPOINT_COUNT (1)
#define MATTER_DM_ON_OFF_CLUSTER_SERVER_ENDPOINT_COUNT (1)
#define MATTER_DM_DESCRIPTOR_CLUSTER_SERVER_ENDPOINT_COUNT (3)
#define MATTER_DM_BINDING_CLUSTER_SERVER_ENDPOINT_COUNT (1)
#define MATTER_DM_ACCESS_CONTROL_CLUSTER_SERVER_ENDPOINT_COUNT (1)
#define MATTER_DM_BASIC_INFORMATION_CLUSTER_SERVER_ENDPOINT_COUNT (1)
#define MATTER_DM_OTA_SOFTWARE_UPDATE_PROVIDER_CLUSTER_CLIENT_ENDPOINT_COUNT (1)
#define MATTER_DM_OTA_SOFTWARE_UPDATE_REQUESTOR_CLUSTER_SERVER_ENDPOINT_COUNT (1)
#define MATTER_DM_GENERAL_COMMISSIONING_CLUSTER_SERVER_ENDPOINT_COUNT (1)
#define MATTER_DM_NETWORK_COMMISSIONING_CLUSTER_SERVER_ENDPOINT_COUNT (1)
#define MATTER_DM_DIAGNOSTIC_LOGS_CLUSTER_SERVER_ENDPOINT_COUNT (1)
#define MATTER_DM_GENERAL_DIAGNOSTICS_CLUSTER_SERVER_ENDPOINT_COUNT (1)
#define MATTER_DM_SOFTWARE_DIAGNOSTICS_CLUSTER_SERVER_ENDPOINT_COUNT (1)
#define MATTER_DM_WIFI_NETWORK_DIAGNOSTICS_CLUSTER_SERVER_ENDPOINT_COUNT (1)
#define MATTER_DM_BRIDGED_DEVICE_BASIC_INFORMATION_CLUSTER_SERVER_ENDPOINT_COUNT (1)
#define MATTER_DM_SWITCH_CLUSTER_SERVER_ENDPOINT_COUNT (1)
#define MATTER_DM_ADMINISTRATOR_COMMISSIONING_CLUSTER_SERVER_ENDPOINT_COUNT (1)
#define MATTER_DM_OPERATIONAL_CREDENTIALS_CLUSTER_SERVER_ENDPOINT_COUNT (1)
#define MATTER_DM_GROUP_KEY_MANAGEMENT_CLUSTER_SERVER_ENDPOINT_COUNT (1)
#define MATTER_DM_TEMPERATURE_MEASUREMENT_CLUSTER_SERVER_ENDPOINT_COUNT (1)
#define MATTER_DM_RELATIVE_HUMIDITY_MEASUREMENT_CLUSTER_SERVER_ENDPOINT_COUNT (1)

/**** Cluster Plugins ****/

// Use this macro to check if the client side of the Identify cluster is included
#define ZCL_USING_IDENTIFY_CLUSTER_CLIENT
#define MATTER_DM_PLUGIN_IDENTIFY_CLIENT

// Use this macro to check if the server side of the Identify cluster is included
#define ZCL_USING_IDENTIFY_CLUSTER_SERVER
#define MATTER_DM_PLUGIN_IDENTIFY_SERVER
#define MATTER_DM_PLUGIN_IDENTIFY

// Use this macro to check if the server side of the Groups cluster is included
#define ZCL_USING_GROUPS_CLUSTER_SERVER
#define MATTER_DM_PLUGIN_GROUPS_SERVER
#define MATTER_DM_PLUGIN_GROUPS

// Use this macro to check if the client side of the On/Off cluster is included
#define ZCL_USING_ON_OFF_CLUSTER_CLIENT
#define MATTER_DM_PLUGIN_ON_OFF_CLIENT

// Use this macro to check if the server side of the On/Off cluster is included
#define ZCL_USING_ON_OFF_CLUSTER_SERVER
#define MATTER_DM_PLUGIN_ON_OFF_SERVER
#define MATTER_DM_PLUGIN_ON_OFF

// Use this macro to check if the server side of the Descriptor cluster is included
#define ZCL_USING_DESCRIPTOR_CLUSTER_SERVER
#define MATTER_DM_PLUGIN_DESCRIPTOR_SERVER
#define MATTER_DM_PLUGIN_DESCRIPTOR

// Use this macro to check if the server side of the Binding cluster is included
#define ZCL_USING_BINDING_CLUSTER_SERVER
#define MATTER_DM_PLUGIN_BINDING_SERVER
#define MATTER_DM_PLUGIN_BINDING

// Use this macro to check if the server side of the Access Control cluster is included
#define ZCL_USING_ACCESS_CONTROL_CLUSTER_SERVER
#define MATTER_DM_PLUGIN_ACCESS_CONTROL_SERVER
#define MATTER_DM_PLUGIN_ACCESS_CONTROL

// Use this macro to check if the server side of the Basic Information cluster is included
#define ZCL_USING_BASIC_INFORMATION_CLUSTER_SERVER
#define MATTER_DM_PLUGIN_BASIC_INFORMATION_SERVER
#define MATTER_DM_PLUGIN_BASIC_INFORMATION

// Use this macro to check if the client side of the OTA Software Update Provider cluster is included
#define ZCL_USING_OTA_SOFTWARE_UPDATE_PROVIDER_CLUSTER_CLIENT
#define MATTER_DM_PLUGIN_OTA_SOFTWARE_UPDATE_PROVIDER_CLIENT

// Use this macro to check if the server side of the OTA Software Update Requestor cluster is included
#define ZCL_USING_OTA_SOFTWARE_UPDATE_REQUESTOR_CLUSTER_SERVER
#define MATTER_DM_PLUGIN_OTA_SOFTWARE_UPDATE_REQUESTOR_SERVER
#define MATTER_DM_PLUGIN_OTA_SOFTWARE_UPDATE_REQUESTOR

// Use this macro to check if the server side of the General Commissioning cluster is included
#define ZCL_USING_GENERAL_COMMISSIONING_CLUSTER_SERVER
#define MATTER_DM_PLUGIN_GENERAL_COMMISSIONING_SERVER
#define MATTER_DM_PLUGIN_GENERAL_COMMISSIONING

// Use this macro to check if the server side of the Network Commissioning cluster is included
#define ZCL_USING_NETWORK_COMMISSIONING_CLUSTER_SERVER
#define MATTER_DM_PLUGIN_NETWORK_COMMISSIONING_SERVER
#define MATTER_DM_PLUGIN_NETWORK_COMMISSIONING

// Use this macro to check if the server side of the Diagnostic Logs cluster is included
#define ZCL_USING_DIAGNOSTIC_LOGS_CLUSTER_SERVER
#define MATTER_DM_PLUGIN_DIAGNOSTIC_LOGS_SERVER
#define MATTER_DM_PLUGIN_DIAGNOSTIC_LOGS

// Use this macro to check if the server side of the General Diagnostics cluster is included
#define ZCL_USING_GENERAL_DIAGNOSTICS_CLUSTER_SERVER
#define MATTER_DM_PLUGIN_GENERAL_DIAGNOSTICS_SERVER
#define MATTER_DM_PLUGIN_GENERAL_DIAGNOSTICS

// Use this macro to check if the server side of the Software Diagnostics cluster is included
#define ZCL_USING_SOFTWARE_DIAGNOSTICS_CLUSTER_SERVER
#define MATTER_DM_PLUGIN_SOFTWARE_DIAGNOSTICS_SERVER
#define MATTER_DM_PLUGIN_SOFTWARE_DIAGNOSTICS

// Use this macro to check if the server side of the WiFi Network Diagnostics cluster is included
#define ZCL_USING_WIFI_NETWORK_DIAGNOSTICS_CLUSTER_SERVER
#define MATTER_DM_PLUGIN_WI_FI_NETWORK_DIAGNOSTICS_SERVER
#define MATTER_DM_PLUGIN_WI_FI_NETWORK_DIAGNOSTICS

// Use this macro to check if the server side of the Bridged Device Basic Information cluster is included
#define ZCL_USING_BRIDGED_DEVICE_BASIC_INFORMATION_CLUSTER_SERVER
#define MATTER_DM_PLUGIN_BRIDGED_DEVICE_BASIC_INFORMATION_SERVER
#define MATTER_DM_PLUGIN_BRIDGED_DEVICE_BASIC_INFORMATION

// Use this macro to check if the server side of the Switch cluster is included
#define ZCL_USING_SWITCH_CLUSTER_SERVER
#define MATTER_DM_PLUGIN_SWITCH_SERVER
#define MATTER_DM_PLUGIN_SWITCH

// Use this macro to check if the server side of the Administrator Commissioning cluster is included
#define ZCL_USING_ADMINISTRATOR_COMMISSIONING_CLUSTER_SERVER
#define MATTER_DM_PLUGIN_ADMINISTRATOR_COMMISSIONING_SERVER
#define MATTER_DM_PLUGIN_ADMINISTRATOR_COMMISSIONING

// Use this macro to check if the server side of the Operational Credentials cluster is included
#define ZCL_USING_OPERATIONAL_CREDENTIALS_CLUSTER_SERVER
#define MATTER_DM_PLUGIN_OPERATIONAL_CREDENTIALS_SERVER
#define MATTER_DM_PLUGIN_OPERATIONAL_CREDENTIALS

// Use this macro to check if the server side of the Group Key Management cluster is included
#define ZCL_USING_GROUP_KEY_MANAGEMENT_CLUSTER_SERVER
#define MATTER_DM_PLUGIN_GROUP_KEY_MANAGEMENT_SERVER
#define MATTER_DM_PLUGIN_GROUP_KEY_MANAGEMENT

// Use this macro to check if the server side of the Temperature Measurement cluster is included
#define ZCL_USING_TEMPERATURE_MEASUREMENT_CLUSTER_SERVER
#define MATTER_DM_PLUGIN_TEMPERATURE_MEASUREMENT_SERVER
#define MATTER_DM_PLUGIN_TEMPERATURE_MEASUREMENT

// Use this macro to check if the server side of the Relative Humidity Measurement cluster is included
#define ZCL_USING_RELATIVE_HUMIDITY_MEASUREMENT_CLUSTER_SERVER
#define MATTER_DM_PLUGIN_RELATIVE_HUMIDITY_MEASUREMENT_SERVER
#define MATTER_DM_PLUGIN_RELATIVE_HUMIDITY_MEASUREMENT
