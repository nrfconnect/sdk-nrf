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

#include <app-common/zap-generated/callback.h>
#include <app-common/zap-generated/cluster-id.h>
#include <lib/support/Span.h>
#include <protocols/interaction_model/Constants.h>

using namespace chip;

// Cluster Init Functions
void emberAfClusterInitCallback(EndpointId endpoint, ClusterId clusterId)
{
	switch (clusterId) {
	case ZCL_ACCESS_CONTROL_CLUSTER_ID:
		emberAfAccessControlClusterInitCallback(endpoint);
		break;
	case ZCL_ADMINISTRATOR_COMMISSIONING_CLUSTER_ID:
		emberAfAdministratorCommissioningClusterInitCallback(endpoint);
		break;
	case ZCL_BASIC_CLUSTER_ID:
		emberAfBasicClusterInitCallback(endpoint);
		break;
	case ZCL_DESCRIPTOR_CLUSTER_ID:
		emberAfDescriptorClusterInitCallback(endpoint);
		break;
	case ZCL_DOOR_LOCK_CLUSTER_ID:
		emberAfDoorLockClusterInitCallback(endpoint);
		break;
	case ZCL_GENERAL_COMMISSIONING_CLUSTER_ID:
		emberAfGeneralCommissioningClusterInitCallback(endpoint);
		break;
	case ZCL_GENERAL_DIAGNOSTICS_CLUSTER_ID:
		emberAfGeneralDiagnosticsClusterInitCallback(endpoint);
		break;
	case ZCL_GROUP_KEY_MANAGEMENT_CLUSTER_ID:
		emberAfGroupKeyManagementClusterInitCallback(endpoint);
		break;
	case ZCL_IDENTIFY_CLUSTER_ID:
		emberAfIdentifyClusterInitCallback(endpoint);
		break;
	case ZCL_NETWORK_COMMISSIONING_CLUSTER_ID:
		emberAfNetworkCommissioningClusterInitCallback(endpoint);
		break;
	case ZCL_OTA_PROVIDER_CLUSTER_ID:
		emberAfOtaSoftwareUpdateProviderClusterInitCallback(endpoint);
		break;
	case ZCL_OTA_REQUESTOR_CLUSTER_ID:
		emberAfOtaSoftwareUpdateRequestorClusterInitCallback(endpoint);
		break;
	case ZCL_OPERATIONAL_CREDENTIALS_CLUSTER_ID:
		emberAfOperationalCredentialsClusterInitCallback(endpoint);
		break;
	case ZCL_SOFTWARE_DIAGNOSTICS_CLUSTER_ID:
		emberAfSoftwareDiagnosticsClusterInitCallback(endpoint);
		break;
	case ZCL_THREAD_NETWORK_DIAGNOSTICS_CLUSTER_ID:
		emberAfThreadNetworkDiagnosticsClusterInitCallback(endpoint);
		break;
	case ZCL_WIFI_NETWORK_DIAGNOSTICS_CLUSTER_ID:
		emberAfWiFiNetworkDiagnosticsClusterInitCallback(endpoint);
		break;
	default:
		// Unrecognized cluster ID
		break;
	}
}

void __attribute__((weak)) emberAfAccessControlClusterInitCallback(EndpointId endpoint)
{
	// To prevent warning
	(void)endpoint;
}
void __attribute__((weak)) emberAfAdministratorCommissioningClusterInitCallback(EndpointId endpoint)
{
	// To prevent warning
	(void)endpoint;
}
void __attribute__((weak)) emberAfBasicClusterInitCallback(EndpointId endpoint)
{
	// To prevent warning
	(void)endpoint;
}
void __attribute__((weak)) emberAfDescriptorClusterInitCallback(EndpointId endpoint)
{
	// To prevent warning
	(void)endpoint;
}
void __attribute__((weak)) emberAfDoorLockClusterInitCallback(EndpointId endpoint)
{
	// To prevent warning
	(void)endpoint;
}
void __attribute__((weak)) emberAfGeneralCommissioningClusterInitCallback(EndpointId endpoint)
{
	// To prevent warning
	(void)endpoint;
}
void __attribute__((weak)) emberAfGeneralDiagnosticsClusterInitCallback(EndpointId endpoint)
{
	// To prevent warning
	(void)endpoint;
}
void __attribute__((weak)) emberAfGroupKeyManagementClusterInitCallback(EndpointId endpoint)
{
	// To prevent warning
	(void)endpoint;
}
void __attribute__((weak)) emberAfIdentifyClusterInitCallback(EndpointId endpoint)
{
	// To prevent warning
	(void)endpoint;
}
void __attribute__((weak)) emberAfNetworkCommissioningClusterInitCallback(EndpointId endpoint)
{
	// To prevent warning
	(void)endpoint;
}
void __attribute__((weak)) emberAfOtaSoftwareUpdateProviderClusterInitCallback(EndpointId endpoint)
{
	// To prevent warning
	(void)endpoint;
}
void __attribute__((weak)) emberAfOtaSoftwareUpdateRequestorClusterInitCallback(EndpointId endpoint)
{
	// To prevent warning
	(void)endpoint;
}
void __attribute__((weak)) emberAfOperationalCredentialsClusterInitCallback(EndpointId endpoint)
{
	// To prevent warning
	(void)endpoint;
}
void __attribute__((weak)) emberAfSoftwareDiagnosticsClusterInitCallback(EndpointId endpoint)
{
	// To prevent warning
	(void)endpoint;
}
void __attribute__((weak)) emberAfThreadNetworkDiagnosticsClusterInitCallback(EndpointId endpoint)
{
	// To prevent warning
	(void)endpoint;
}
void __attribute__((weak)) emberAfWiFiNetworkDiagnosticsClusterInitCallback(EndpointId endpoint)
{
	// To prevent warning
	(void)endpoint;
}

//
// Non-Cluster Related Callbacks
//

void __attribute__((weak)) emberAfAddToCurrentAppTasksCallback(EmberAfApplicationTask tasks) {}

void __attribute__((weak)) emberAfRemoveFromCurrentAppTasksCallback(EmberAfApplicationTask tasks) {}

EmberAfAttributeWritePermission __attribute__((weak))
emberAfAllowNetworkWriteAttributeCallback(EndpointId endpoint, ClusterId clusterId, AttributeId attributeId,
					  uint8_t *value, uint8_t type)
{
	return EMBER_ZCL_ATTRIBUTE_WRITE_PERMISSION_ALLOW_WRITE_NORMAL; // Default
}

bool __attribute__((weak))
emberAfAttributeReadAccessCallback(EndpointId endpoint, ClusterId clusterId, AttributeId attributeId)
{
	return true;
}

bool __attribute__((weak))
emberAfAttributeWriteAccessCallback(EndpointId endpoint, ClusterId clusterId, AttributeId attributeId)
{
	return true;
}

bool __attribute__((weak))
emberAfDefaultResponseCallback(ClusterId clusterId, CommandId commandId, EmberAfStatus status)
{
	return false;
}

bool __attribute__((weak)) emberAfPreMessageSendCallback(EmberAfMessageStruct *messageStruct, EmberStatus *status)
{
	return false;
}

bool __attribute__((weak))
emberAfMessageSentCallback(const MessageSendDestination &destination, EmberApsFrame *apsFrame, uint16_t msgLen,
			   uint8_t *message, EmberStatus status)
{
	return false;
}

EmberAfStatus __attribute__((weak))
emberAfExternalAttributeReadCallback(EndpointId endpoint, ClusterId clusterId,
				     const EmberAfAttributeMetadata *attributeMetadata, uint8_t *buffer,
				     uint16_t maxReadLength)
{
	return EMBER_ZCL_STATUS_FAILURE;
}

EmberAfStatus __attribute__((weak))
emberAfExternalAttributeWriteCallback(EndpointId endpoint, ClusterId clusterId,
				      const EmberAfAttributeMetadata *attributeMetadata, uint8_t *buffer)
{
	return EMBER_ZCL_STATUS_FAILURE;
}

uint32_t __attribute__((weak)) emberAfGetCurrentTimeCallback()
{
	return 0;
}

bool __attribute__((weak)) emberAfGetEndpointInfoCallback(EndpointId endpoint, uint8_t *returnNetworkIndex,
							  EmberAfEndpointInfoStruct *returnEndpointInfo)
{
	return false;
}

void __attribute__((weak)) emberAfRegistrationAbortCallback() {}

EmberStatus __attribute__((weak))
emberAfInterpanSendMessageCallback(EmberAfInterpanHeader *header, uint16_t messageLength, uint8_t *message)
{
	return EMBER_LIBRARY_NOT_PRESENT;
}

bool __attribute__((weak)) emberAfStartMoveCallback()
{
	return false;
}

chip::Protocols::InteractionModel::Status __attribute__((weak))
MatterPreAttributeChangeCallback(const chip::app::ConcreteAttributePath &attributePath, uint8_t type, uint16_t size,
				 uint8_t *value)
{
	return chip::Protocols::InteractionModel::Status::Success;
}

void __attribute__((weak)) MatterPostAttributeChangeCallback(const chip::app::ConcreteAttributePath &attributePath,
							     uint8_t type, uint16_t size, uint8_t *value)
{
}
