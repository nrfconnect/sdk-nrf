/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "onoff_light.h"

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
using namespace Nrf;

constexpr CommandId identifyIncomingCommands[] = {
	app::Clusters::Identify::Commands::Identify::Id,
	app::Clusters::Identify::Commands::TriggerEffect::Id,
	kInvalidCommandId,
};

DECLARE_DYNAMIC_ATTRIBUTE_LIST_BEGIN(onOffAttrs)
DECLARE_DYNAMIC_ATTRIBUTE(Clusters::OnOff::Attributes::OnOff::Id, BOOLEAN, 1, 0),
	DECLARE_DYNAMIC_ATTRIBUTE(Clusters::OnOff::Attributes::FeatureMap::Id, BITMAP32, 4, 0),
	DECLARE_DYNAMIC_ATTRIBUTE(Clusters::OnOff::Attributes::GlobalSceneControl::Id, BOOLEAN, 1, 0),
	DECLARE_DYNAMIC_ATTRIBUTE(Clusters::OnOff::Attributes::OnTime::Id, INT16U, 2, ZAP_ATTRIBUTE_MASK(WRITABLE)),
	DECLARE_DYNAMIC_ATTRIBUTE(Clusters::OnOff::Attributes::OffWaitTime::Id, INT16U, 2,
				  ZAP_ATTRIBUTE_MASK(WRITABLE)),
	DECLARE_DYNAMIC_ATTRIBUTE(Clusters::OnOff::Attributes::StartUpOnOff::Id, ENUM8, 1,
				  ZAP_ATTRIBUTE_MASK(WRITABLE)),
	DECLARE_DYNAMIC_ATTRIBUTE_LIST_END();

constexpr CommandId onOffIncomingCommands[] = {
	app::Clusters::OnOff::Commands::Off::Id,
	app::Clusters::OnOff::Commands::On::Id,
	app::Clusters::OnOff::Commands::Toggle::Id,
	app::Clusters::OnOff::Commands::OffWithEffect::Id,
	app::Clusters::OnOff::Commands::OnWithRecallGlobalScene::Id,
	app::Clusters::OnOff::Commands::OnWithTimedOff::Id,
	kInvalidCommandId,
};

DECLARE_DYNAMIC_ATTRIBUTE_LIST_BEGIN(groupsAttrs)
DECLARE_DYNAMIC_ATTRIBUTE(Clusters::Groups::Attributes::NameSupport::Id, BITMAP8, 1, 0),
	DECLARE_DYNAMIC_ATTRIBUTE(Clusters::Groups::Attributes::FeatureMap::Id, BITMAP32, 4, 1),
	DECLARE_DYNAMIC_ATTRIBUTE_LIST_END();

constexpr CommandId groupsIncomingCommands[] = {
	app::Clusters::Groups::Commands::AddGroup::Id,
	app::Clusters::Groups::Commands::ViewGroup::Id,
	app::Clusters::Groups::Commands::GetGroupMembership::Id,
	app::Clusters::Groups::Commands::RemoveGroup::Id,
	app::Clusters::Groups::Commands::RemoveAllGroups::Id,
	app::Clusters::Groups::Commands::AddGroupIfIdentifying::Id,
	kInvalidCommandId,
};

constexpr CommandId groupsGeneratedCommands[] = {
	app::Clusters::Groups::Commands::AddGroupResponse::Id,
	app::Clusters::Groups::Commands::RemoveGroupResponse::Id,
	app::Clusters::Groups::Commands::ViewGroupResponse::Id,
	app::Clusters::Groups::Commands::GetGroupMembershipResponse::Id,
	kInvalidCommandId,
};

DECLARE_DYNAMIC_CLUSTER_LIST_BEGIN(bridgedLightClusters)
DECLARE_DYNAMIC_CLUSTER(Clusters::OnOff::Id, onOffAttrs, ZAP_CLUSTER_MASK(SERVER), onOffIncomingCommands, nullptr),
	DECLARE_DYNAMIC_CLUSTER(Clusters::Descriptor::Id, descriptorAttrs, ZAP_CLUSTER_MASK(SERVER), nullptr, nullptr),
	DECLARE_DYNAMIC_CLUSTER(Clusters::Groups::Id, groupsAttrs, ZAP_CLUSTER_MASK(SERVER), groupsIncomingCommands,
				groupsGeneratedCommands),
	DECLARE_DYNAMIC_CLUSTER(Clusters::BridgedDeviceBasicInformation::Id, bridgedDeviceBasicAttrs,
				ZAP_CLUSTER_MASK(SERVER), nullptr, nullptr),
	DECLARE_DYNAMIC_CLUSTER(Clusters::Identify::Id, identifyAttrs, ZAP_CLUSTER_MASK(SERVER),
				identifyIncomingCommands, nullptr) DECLARE_DYNAMIC_CLUSTER_LIST_END;

DECLARE_DYNAMIC_ENDPOINT(bridgedLightEndpoint, bridgedLightClusters);

static constexpr uint8_t kBridgedOnOffEndpointVersion = 2;

static constexpr EmberAfDeviceType kBridgedOnOffDeviceTypes[] = {
	{ static_cast<chip::DeviceTypeId>(MatterBridgedDevice::DeviceType::OnOffLight), kBridgedOnOffEndpointVersion },
	{ static_cast<chip::DeviceTypeId>(MatterBridgedDevice::DeviceType::BridgedNode),
	  MatterBridgedDevice::kDefaultDynamicEndpointVersion }
};

static constexpr uint8_t kLightDataVersionSize = ArraySize(bridgedLightClusters);

OnOffLightDevice::OnOffLightDevice(const char *nodeLabel) : MatterBridgedDevice(nodeLabel)
{
	mDataVersionSize = kLightDataVersionSize;
	mEp = &bridgedLightEndpoint;
	mDeviceTypeList = kBridgedOnOffDeviceTypes;
	mDeviceTypeListSize = ARRAY_SIZE(kBridgedOnOffDeviceTypes);
	mDataVersion = static_cast<DataVersion *>(chip::Platform::MemoryAlloc(sizeof(DataVersion) * mDataVersionSize));
}

CHIP_ERROR OnOffLightDevice::HandleRead(ClusterId clusterId, AttributeId attributeId, uint8_t *buffer,
					uint16_t maxReadLength)
{
	switch (clusterId) {
	case Clusters::OnOff::Id:
		return HandleReadOnOff(attributeId, buffer, maxReadLength);
	case Clusters::Groups::Id:
		return HandleReadGroups(attributeId, buffer, maxReadLength);
	default:
		return CHIP_ERROR_INVALID_ARGUMENT;
	}
}

CHIP_ERROR OnOffLightDevice::HandleReadOnOff(AttributeId attributeId, uint8_t *buffer, uint16_t maxReadLength)
{
	switch (attributeId) {
	case Clusters::OnOff::Attributes::OnOff::Id: {
		bool onOff = GetOnOff();
		return CopyAttribute(&onOff, sizeof(onOff), buffer, maxReadLength);
	}
	case Clusters::OnOff::Attributes::ClusterRevision::Id: {
		uint16_t clusterRevision = GetOnOffClusterRevision();
		return CopyAttribute(&clusterRevision, sizeof(clusterRevision), buffer, maxReadLength);
	}
	case Clusters::OnOff::Attributes::FeatureMap::Id: {
		uint32_t featureMap = GetOnOffFeatureMap();
		return CopyAttribute(&featureMap, sizeof(featureMap), buffer, maxReadLength);
	}
	case Clusters::OnOff::Attributes::OnTime::Id: {
		return CopyAttribute(&mOnTime, sizeof(mOnTime), buffer, maxReadLength);
	}
	case Clusters::OnOff::Attributes::OffWaitTime::Id: {
		return CopyAttribute(&mOffWaitTime, sizeof(mOffWaitTime), buffer, maxReadLength);
	}
	case Clusters::OnOff::Attributes::StartUpOnOff::Id: {
		return CopyAttribute(&mStartUpOnOff, sizeof(mStartUpOnOff), buffer, maxReadLength);
	}
	case Clusters::OnOff::Attributes::GlobalSceneControl::Id: {
		return CopyAttribute(&mGlobalSceneControl, sizeof(mGlobalSceneControl), buffer, maxReadLength);
	}
	default:
		return CHIP_ERROR_INVALID_ARGUMENT;
	}
}

CHIP_ERROR OnOffLightDevice::HandleReadGroups(AttributeId attributeId, uint8_t *buffer, uint16_t maxReadLength)
{
	switch (attributeId) {
	case Clusters::Groups::Attributes::NameSupport::Id: {
		uint8_t nameSupportMap = GetGroupsNameSupportMap();
		return CopyAttribute(&nameSupportMap, sizeof(nameSupportMap), buffer, maxReadLength);
	}
	case Clusters::Groups::Attributes::ClusterRevision::Id: {
		uint16_t clusterRevision = GetGroupsClusterRevision();
		return CopyAttribute(&clusterRevision, sizeof(clusterRevision), buffer, maxReadLength);
	}
	case Clusters::Groups::Attributes::FeatureMap::Id: {
		uint32_t featureMap = GetGroupsFeatureMap();
		return CopyAttribute(&featureMap, sizeof(featureMap), buffer, maxReadLength);
	}
	default:
		return CHIP_ERROR_INVALID_ARGUMENT;
	}
}

CHIP_ERROR OnOffLightDevice::HandleWrite(ClusterId clusterId, AttributeId attributeId, uint8_t *buffer, size_t size)
{
	if (clusterId != Clusters::OnOff::Id && clusterId != Clusters::BridgedDeviceBasicInformation::Id) {
		return CHIP_ERROR_INVALID_ARGUMENT;
	}

	switch (attributeId) {
	case Clusters::OnOff::Attributes::OnOff::Id:
		mOnOff = *buffer;
		break;
	case Clusters::OnOff::Attributes::OnTime::Id:
		mOnTime = *buffer;
		break;
	case Clusters::OnOff::Attributes::OffWaitTime::Id:
		mOffWaitTime = *buffer;
		break;
	case Clusters::OnOff::Attributes::StartUpOnOff::Id:
		mStartUpOnOff = static_cast<Clusters::OnOff::StartUpOnOffEnum>(*buffer);
		break;
	case Clusters::BridgedDeviceBasicInformation::Attributes::NodeLabel::Id:
		return HandleWriteDeviceBasicInformation(clusterId, attributeId, buffer, size);
	default:
		return CHIP_ERROR_INVALID_ARGUMENT;
	}
	return CHIP_NO_ERROR;
}

CHIP_ERROR OnOffLightDevice::HandleAttributeChange(chip::ClusterId clusterId, chip::AttributeId attributeId, void *data,
						   size_t dataSize)
{
	CHIP_ERROR err = CHIP_NO_ERROR;
	if (!data) {
		return CHIP_ERROR_INVALID_ARGUMENT;
	}

	switch (clusterId) {
	case Clusters::BridgedDeviceBasicInformation::Id:
		return HandleWriteDeviceBasicInformation(clusterId, attributeId, data, dataSize);
	case Clusters::Identify::Id:
		return HandleWriteIdentify(attributeId, data, dataSize);
	case Clusters::OnOff::Id: {
		switch (attributeId) {
		case Clusters::OnOff::Attributes::OnOff::Id: {
			bool value;

			err = CopyAttribute(data, dataSize, &value, sizeof(value));

			if (err != CHIP_NO_ERROR) {
				return err;
			}
			mOnOff = value;
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
