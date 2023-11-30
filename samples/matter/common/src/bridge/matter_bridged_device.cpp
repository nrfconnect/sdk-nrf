/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "matter_bridged_device.h"
#include <app-common/zap-generated/callback.h>
#include <lib/support/ZclString.h>

using namespace ::chip;
using namespace ::chip::app;

CHIP_ERROR MatterBridgedDevice::CopyAttribute(const void *attribute, size_t attributeSize, void *buffer,
					      uint16_t maxBufferSize)
{
	if (maxBufferSize < attributeSize) {
		return CHIP_ERROR_INVALID_ARGUMENT;
	}

	memcpy(buffer, attribute, attributeSize);

	return CHIP_NO_ERROR;
}

CHIP_ERROR MatterBridgedDevice::HandleWriteDeviceBasicInformation(chip::ClusterId clusterId,
								  chip::AttributeId attributeId, void *data,
								  size_t dataSize)
{
	switch (attributeId) {
	case Clusters::BridgedDeviceBasicInformation::Attributes::Reachable::Id:
		if (data && dataSize == sizeof(bool)) {
			SetIsReachable(*reinterpret_cast<bool *>(data));
			return CHIP_NO_ERROR;
		}
	default:
		return CHIP_ERROR_INVALID_ARGUMENT;
	}
}

CHIP_ERROR MatterBridgedDevice::HandleReadBridgedDeviceBasicInformation(AttributeId attributeId, uint8_t *buffer,
									uint16_t maxReadLength)
{
	switch (attributeId) {
	case Clusters::BridgedDeviceBasicInformation::Attributes::Reachable::Id: {
		bool isReachable = GetIsReachable();
		return CopyAttribute(&isReachable, sizeof(isReachable), buffer, maxReadLength);
	}
	case Clusters::BridgedDeviceBasicInformation::Attributes::NodeLabel::Id: {
		MutableByteSpan zclNodeLabelSpan(buffer, maxReadLength);
		return MakeZclCharString(zclNodeLabelSpan, GetNodeLabel());
	}
	case Clusters::BridgedDeviceBasicInformation::Attributes::ClusterRevision::Id: {
		uint16_t clusterRevision = GetBridgedDeviceBasicInformationClusterRevision();
		return CopyAttribute(&clusterRevision, sizeof(clusterRevision), buffer, maxReadLength);
	}
	case Clusters::BridgedDeviceBasicInformation::Attributes::FeatureMap::Id: {
		uint32_t featureMap = GetBridgedDeviceBasicInformationFeatureMap();
		return CopyAttribute(&featureMap, sizeof(featureMap), buffer, maxReadLength);
	}
	default:
		return CHIP_ERROR_INVALID_ARGUMENT;
	}
}

CHIP_ERROR MatterBridgedDevice::HandleWriteIdentify(chip::AttributeId attributeId, void *data, size_t dataSize)
{
	switch (attributeId) {
	case Clusters::Identify::Attributes::IdentifyTime::Id:
		if (data && dataSize == sizeof(mIdentifyTime)) {
			memcpy(&mIdentifyTime, data, sizeof(mIdentifyTime));
			/* Externally stored attribute was updated, now we need to notify identify-server to
			   leverage the Identify cluster implementation */
			app::ConcreteAttributePath attributePath{ mEndpointId, Clusters::Identify::Id, attributeId };
			MatterIdentifyClusterServerAttributeChangedCallback(attributePath);
			return CHIP_NO_ERROR;
		}
	default:
		return CHIP_ERROR_INVALID_ARGUMENT;
	}
}

CHIP_ERROR MatterBridgedDevice::HandleReadIdentify(chip::AttributeId attributeId, uint8_t *buffer,
						   uint16_t maxReadLength)
{
	switch (attributeId) {
	case Clusters::Identify::Attributes::ClusterRevision::Id: {
		uint16_t clusterRevision = GetIdentifyClusterRevision();
		return CopyAttribute(&clusterRevision, sizeof(clusterRevision), buffer, maxReadLength);
	}
	case Clusters::Identify::Attributes::FeatureMap::Id: {
		uint32_t featureMap = GetIdentifyClusterFeatureMap();
		return CopyAttribute(&featureMap, sizeof(featureMap), buffer, maxReadLength);
	}
	case Clusters::Identify::Attributes::IdentifyType::Id: {
		MatterBridgedDevice::IdentifyType type = mIdentifyServer.mIdentifyType;
		return CopyAttribute(&type, sizeof(type), buffer, maxReadLength);
	}
	case Clusters::Identify::Attributes::IdentifyTime::Id: {
		return CopyAttribute(&mIdentifyTime, sizeof(mIdentifyTime), buffer, maxReadLength);
	}
	default:
		return CHIP_ERROR_INVALID_ARGUMENT;
	}
}
