/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "bridged_device.h"
#include <lib/support/ZclString.h>

using namespace ::chip;
using namespace ::chip::app;

CHIP_ERROR BridgedDevice::CopyAttribute(void *attribute, size_t attributeSize, void *buffer, uint16_t maxBufferSize)
{
	if (maxBufferSize < attributeSize) {
		return CHIP_ERROR_INVALID_ARGUMENT;
	}

	memcpy(buffer, attribute, attributeSize);

	return CHIP_NO_ERROR;
}

CHIP_ERROR BridgedDevice::HandleWriteDeviceBasicInformation(chip::ClusterId clusterId, chip::AttributeId attributeId,
							    void *data, size_t dataSize)
{
	switch (attributeId) {
	case Clusters::BridgedDeviceBasicInformation::Attributes::Reachable::Id:
		if (data && dataSize == sizeof(bool)) {
			SetIsReachable(*reinterpret_cast<bool*>(data));
			return CHIP_NO_ERROR;
		}
		default:
		return CHIP_ERROR_INVALID_ARGUMENT;
	}
}

CHIP_ERROR BridgedDevice::HandleReadBridgedDeviceBasicInformation(AttributeId attributeId, uint8_t *buffer,
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

CHIP_ERROR BridgedDevice::HandleReadDescriptor(AttributeId attributeId, uint8_t *buffer, uint16_t maxReadLength)
{
	switch (attributeId) {
	case Clusters::Descriptor::Attributes::ClusterRevision::Id: {
		uint16_t clusterRevision = GetDescriptorClusterRevision();
		return CopyAttribute(&clusterRevision, sizeof(clusterRevision), buffer, maxReadLength);
	}
	case Clusters::Descriptor::Attributes::FeatureMap::Id: {
		uint32_t featureMap = GetDescriptorFeatureMap();
		return CopyAttribute(&featureMap, sizeof(featureMap), buffer, maxReadLength);
	}
	default:
		return CHIP_ERROR_INVALID_ARGUMENT;
	}
}
