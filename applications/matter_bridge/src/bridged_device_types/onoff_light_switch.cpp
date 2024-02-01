/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "onoff_light_switch.h"

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

DECLARE_DYNAMIC_ATTRIBUTE_LIST_BEGIN(onOffClientAttrs)
DECLARE_DYNAMIC_ATTRIBUTE(Clusters::OnOff::Attributes::FeatureMap::Id, BITMAP32, 4, 0),
	DECLARE_DYNAMIC_ATTRIBUTE_LIST_END();

constexpr CommandId onOffOutgoingCommands[] = {
	app::Clusters::OnOff::Commands::Off::Id,
	app::Clusters::OnOff::Commands::On::Id,
	app::Clusters::OnOff::Commands::Toggle::Id,
	kInvalidCommandId,
};

DECLARE_DYNAMIC_ATTRIBUTE_LIST_BEGIN(bindingAttrs)
DECLARE_DYNAMIC_ATTRIBUTE(Clusters::Binding::Attributes::Binding::Id, ARRAY,
			  sizeof(Clusters::Binding::Structs::TargetStruct::Type), ZAP_ATTRIBUTE_MASK(WRITABLE)),
	DECLARE_DYNAMIC_ATTRIBUTE(Clusters::Binding::Attributes::FeatureMap::Id, BITMAP32, 4, 0),
	DECLARE_DYNAMIC_ATTRIBUTE_LIST_END();

DECLARE_DYNAMIC_CLUSTER_LIST_BEGIN(bridgedLightSwitchClusters)
DECLARE_DYNAMIC_CLUSTER(Clusters::OnOff::Id, onOffClientAttrs, ZAP_CLUSTER_MASK(CLIENT), nullptr,
			onOffOutgoingCommands),
	DECLARE_DYNAMIC_CLUSTER(Clusters::Descriptor::Id, descriptorAttrs, ZAP_CLUSTER_MASK(SERVER), nullptr, nullptr),
	DECLARE_DYNAMIC_CLUSTER(Clusters::BridgedDeviceBasicInformation::Id, bridgedDeviceBasicAttrs,
				ZAP_CLUSTER_MASK(SERVER), nullptr, nullptr),
	DECLARE_DYNAMIC_CLUSTER(Clusters::Identify::Id, identifyAttrs, ZAP_CLUSTER_MASK(SERVER),
				sIdentifyIncomingCommands, nullptr),
	DECLARE_DYNAMIC_CLUSTER(Clusters::Binding::Id, bindingAttrs, ZAP_CLUSTER_MASK(SERVER), nullptr,
				nullptr) DECLARE_DYNAMIC_CLUSTER_LIST_END;

DECLARE_DYNAMIC_ENDPOINT(bridgedLightSwitchEndpoint, bridgedLightSwitchClusters);

static constexpr uint8_t kBridgedOnOffLightSwitchEndpointVersion = 2;

static constexpr EmberAfDeviceType kBridgedLightSwitchDeviceTypes[] = {
	{ static_cast<chip::DeviceTypeId>(MatterBridgedDevice::DeviceType::OnOffLightSwitch),
	  kBridgedOnOffLightSwitchEndpointVersion },
	{ static_cast<chip::DeviceTypeId>(MatterBridgedDevice::DeviceType::BridgedNode),
	  MatterBridgedDevice::kDefaultDynamicEndpointVersion }
};

static constexpr uint8_t kLightSwitchDataVersionSize = ArraySize(bridgedLightSwitchClusters);

OnOffLightSwitchDevice::OnOffLightSwitchDevice(const char *nodeLabel) : MatterBridgedDevice(nodeLabel)
{
	mDataVersionSize = kLightSwitchDataVersionSize;
	mEp = &bridgedLightSwitchEndpoint;
	mDeviceTypeList = kBridgedLightSwitchDeviceTypes;
	mDeviceTypeListSize = ARRAY_SIZE(kBridgedLightSwitchDeviceTypes);
	mDataVersion = static_cast<DataVersion *>(chip::Platform::MemoryAlloc(sizeof(DataVersion) * mDataVersionSize));
}

CHIP_ERROR OnOffLightSwitchDevice::HandleRead(ClusterId clusterId, AttributeId attributeId, uint8_t *buffer,
					      uint16_t maxReadLength)
{
	switch (clusterId) {
	case Clusters::OnOff::Id:
		return HandleReadOnOff(attributeId, buffer, maxReadLength);
	case Clusters::Identify::Id:
		return HandleReadIdentify(attributeId, buffer, maxReadLength);
	default:
		return CHIP_ERROR_INVALID_ARGUMENT;
	}
}

CHIP_ERROR OnOffLightSwitchDevice::HandleReadOnOff(AttributeId attributeId, uint8_t *buffer, uint16_t maxReadLength)
{
	switch (attributeId) {
	case Clusters::OnOff::Attributes::ClusterRevision::Id: {
		uint16_t clusterRevision = GetOnOffClusterRevision();
		return CopyAttribute(&clusterRevision, sizeof(clusterRevision), buffer, maxReadLength);
	}
	case Clusters::OnOff::Attributes::FeatureMap::Id: {
		uint32_t featureMap = GetOnOffFeatureMap();
		return CopyAttribute(&featureMap, sizeof(featureMap), buffer, maxReadLength);
	}
	default:
		return CHIP_ERROR_INVALID_ARGUMENT;
	}
}
