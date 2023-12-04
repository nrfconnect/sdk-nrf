/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "generic_switch.h"

#include <zephyr/logging/log.h>

#include <app-common/zap-generated/ids/Commands.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

namespace
{
DESCRIPTOR_CLUSTER_ATTRIBUTES(descriptorAttrs);
BRIDGED_DEVICE_BASIC_INFORMATION_CLUSTER_ATTRIBUTES(bridgedDeviceBasicAttrs);
IDENTIFY_CLUSTER_ATTRIBUTES(identifyAttrs);
}; /* namespace */

using namespace ::chip;
using namespace ::chip::app;

DECLARE_DYNAMIC_ATTRIBUTE_LIST_BEGIN(switchAttr)
DECLARE_DYNAMIC_ATTRIBUTE(Clusters::Switch::Attributes::NumberOfPositions::Id, INT8U, 1, 0),
	DECLARE_DYNAMIC_ATTRIBUTE(Clusters::Switch::Attributes::CurrentPosition::Id, INT8U, 1, 0),
	DECLARE_DYNAMIC_ATTRIBUTE_LIST_END();

DECLARE_DYNAMIC_CLUSTER_LIST_BEGIN(genericSwitchClusters)
DECLARE_DYNAMIC_CLUSTER(Clusters::Switch::Id, switchAttr, nullptr, nullptr),
	DECLARE_DYNAMIC_CLUSTER(Clusters::Descriptor::Id, descriptorAttrs, nullptr, nullptr),
	DECLARE_DYNAMIC_CLUSTER(Clusters::BridgedDeviceBasicInformation::Id, bridgedDeviceBasicAttrs, nullptr, nullptr),
	DECLARE_DYNAMIC_CLUSTER(Clusters::Identify::Id, identifyAttrs, sIdentifyIncomingCommands,
				nullptr) DECLARE_DYNAMIC_CLUSTER_LIST_END;

DECLARE_DYNAMIC_ENDPOINT(bridgedGenericSwitchEndpoint, genericSwitchClusters);

static constexpr EmberAfDeviceType kBridgedGenericSwitchDeviceTypes[] = {
	{ static_cast<chip::DeviceTypeId>(MatterBridgedDevice::DeviceType::GenericSwitch),
	  MatterBridgedDevice::kDefaultDynamicEndpointVersion },
	{ static_cast<chip::DeviceTypeId>(MatterBridgedDevice::DeviceType::BridgedNode),
	  MatterBridgedDevice::kDefaultDynamicEndpointVersion }
};

static constexpr uint8_t kSwitchDataVersionSize = ArraySize(genericSwitchClusters);

GenericSwitchDevice::GenericSwitchDevice(const char *nodeLabel) : MatterBridgedDevice(nodeLabel)
{
	mDataVersionSize = kSwitchDataVersionSize;
	mEp = &bridgedGenericSwitchEndpoint;
	mDeviceTypeList = kBridgedGenericSwitchDeviceTypes;
	mDeviceTypeListSize = ARRAY_SIZE(kBridgedGenericSwitchDeviceTypes);
	mDataVersion = static_cast<DataVersion *>(chip::Platform::MemoryAlloc(sizeof(DataVersion) * mDataVersionSize));
}

CHIP_ERROR GenericSwitchDevice::HandleRead(ClusterId clusterId, AttributeId attributeId, uint8_t *buffer,
				     uint16_t maxReadLength)
{
	switch (clusterId) {
	case Clusters::Switch::Id:
		return HandleReadSwitch(attributeId, buffer, maxReadLength);
	case Clusters::Identify::Id:
		return HandleReadIdentify(attributeId, buffer, maxReadLength);
	default:
		return CHIP_ERROR_INVALID_ARGUMENT;
	}
}

CHIP_ERROR GenericSwitchDevice::HandleReadSwitch(AttributeId attributeId, uint8_t *buffer, uint16_t maxReadLength)
{
	switch (attributeId) {
	case Clusters::Switch::Attributes::NumberOfPositions::Id: {
		uint8_t numberOfPositions = GetSwitchClusterNumberOfPositions();
		return CopyAttribute(&numberOfPositions, sizeof(numberOfPositions), buffer, maxReadLength);
	}
	case Clusters::Switch::Attributes::CurrentPosition::Id: {
		return CopyAttribute(&mCurrentPosition, sizeof(mCurrentPosition), buffer, maxReadLength);
	}
	case Clusters::Switch::Attributes::ClusterRevision::Id: {
		uint16_t clusterRevision = GetSwitchClusterRevision();
		return CopyAttribute(&clusterRevision, sizeof(clusterRevision), buffer, maxReadLength);
	}
	case Clusters::Switch::Attributes::FeatureMap::Id: {
		uint32_t featureMap = GetSwitchClusterFeatureMap();
		return CopyAttribute(&featureMap, sizeof(featureMap), buffer, maxReadLength);
	}
	default:
		return CHIP_ERROR_INVALID_ARGUMENT;
	}
}

CHIP_ERROR GenericSwitchDevice::HandleAttributeChange(chip::ClusterId clusterId, chip::AttributeId attributeId, void *data,
						size_t dataSize)
{
	VerifyOrReturnError(data, CHIP_ERROR_INVALID_ARGUMENT);

	CHIP_ERROR err = CHIP_NO_ERROR;

	switch (clusterId) {
	case Clusters::BridgedDeviceBasicInformation::Id:
		return HandleWriteDeviceBasicInformation(clusterId, attributeId, data, dataSize);
	case Clusters::Identify::Id:
		return HandleWriteIdentify(attributeId, data, dataSize);
	case Clusters::Switch::Id: {
		switch (attributeId) {
		case Clusters::Switch::Attributes::CurrentPosition::Id: {
			err = CopyAttribute(data, dataSize, &mCurrentPosition, sizeof(mCurrentPosition));
			VerifyOrReturnError(err == CHIP_NO_ERROR, err);
			break;
		}
		default:
			return CHIP_ERROR_INVALID_ARGUMENT;
		}
		break;
	}
	default:
		return CHIP_ERROR_INVALID_ARGUMENT;
	}

	return err;
}
