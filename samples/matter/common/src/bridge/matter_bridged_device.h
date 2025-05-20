/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app/clusters/identify-server/identify-server.h>
#include <app/util/attribute-storage.h>
#include <platform/ConfigurationManager.h>

namespace Nrf
{

/* Definitions of  helper macros that are used across all bridged device types to create common mandatory clusters in a
 * consistent way. */
/* Declare Descriptor cluster attributes */
#define DESCRIPTOR_CLUSTER_ATTRIBUTES(descriptorAttrs)                                                                 \
	DECLARE_DYNAMIC_ATTRIBUTE_LIST_BEGIN(descriptorAttrs)                                                          \
	DECLARE_DYNAMIC_ATTRIBUTE(chip::app::Clusters::Descriptor::Attributes::DeviceTypeList::Id, ARRAY,              \
				  Nrf::MatterBridgedDevice::kDescriptorAttributeArraySize, 0), /* device list */       \
		DECLARE_DYNAMIC_ATTRIBUTE(chip::app::Clusters::Descriptor::Attributes::ServerList::Id, ARRAY,          \
					  Nrf::MatterBridgedDevice::kDescriptorAttributeArraySize, 0), /* server list  \
													*/             \
		DECLARE_DYNAMIC_ATTRIBUTE(chip::app::Clusters::Descriptor::Attributes::ClientList::Id, ARRAY,          \
					  Nrf::MatterBridgedDevice::kDescriptorAttributeArraySize, 0), /* client list  \
													*/             \
		DECLARE_DYNAMIC_ATTRIBUTE(chip::app::Clusters::Descriptor::Attributes::PartsList::Id, ARRAY,           \
					  Nrf::MatterBridgedDevice::kDescriptorAttributeArraySize, 0), /* parts list   \
													*/             \
		DECLARE_DYNAMIC_ATTRIBUTE(chip::app::Clusters::Descriptor::Attributes::FeatureMap::Id, BITMAP32, 4,    \
					  0), /* feature map */                                                        \
		DECLARE_DYNAMIC_ATTRIBUTE_LIST_END();

/* Declare Bridged Device Basic Information cluster attributes */
#define BRIDGED_DEVICE_BASIC_INFORMATION_CLUSTER_ATTRIBUTES(bridgedDeviceBasicAttrs)                                   \
	DECLARE_DYNAMIC_ATTRIBUTE_LIST_BEGIN(bridgedDeviceBasicAttrs)                                                  \
	DECLARE_DYNAMIC_ATTRIBUTE(chip::app::Clusters::BridgedDeviceBasicInformation::Attributes::NodeLabel::Id,       \
				  CHAR_STRING, Nrf::MatterBridgedDevice::kNodeLabelSize,                               \
				  ZAP_ATTRIBUTE_MASK(WRITABLE)), /* NodeLabel */                                       \
		DECLARE_DYNAMIC_ATTRIBUTE(                                                                             \
			chip::app::Clusters::BridgedDeviceBasicInformation::Attributes::Reachable::Id, BOOLEAN, 1,     \
			0), /* Reachable */                                                                            \
		DECLARE_DYNAMIC_ATTRIBUTE(                                                                             \
			chip::app::Clusters::BridgedDeviceBasicInformation::Attributes::UniqueID::Id, CHAR_STRING, 32, \
			0), /* UniqueID */                                                                             \
		DECLARE_DYNAMIC_ATTRIBUTE(                                                                             \
			chip::app::Clusters::BridgedDeviceBasicInformation::Attributes::FeatureMap::Id, BITMAP32, 4,   \
			0), /* feature map */                                                                          \
		DECLARE_DYNAMIC_ATTRIBUTE_LIST_END();

/* Declare Bridged Device Identify cluster attributes */

constexpr chip::CommandId sIdentifyIncomingCommands[] = {
	chip::app::Clusters::Identify::Commands::Identify::Id,
	chip::kInvalidCommandId,
};

#define IDENTIFY_CLUSTER_ATTRIBUTES(identifyAttrs)                                                                     \
	DECLARE_DYNAMIC_ATTRIBUTE_LIST_BEGIN(identifyAttrs)                                                            \
	DECLARE_DYNAMIC_ATTRIBUTE(chip::app::Clusters::Identify::Attributes::IdentifyTime::Id, INT16U, 2,              \
				  ZAP_ATTRIBUTE_MASK(WRITABLE)), /* identify time*/                                    \
		DECLARE_DYNAMIC_ATTRIBUTE(chip::app::Clusters::Identify::Attributes::IdentifyType::Id, ENUM8, 1,       \
					  0), /* identify type */                                                      \
		DECLARE_DYNAMIC_ATTRIBUTE(chip::app::Clusters::Identify::Attributes::FeatureMap::Id, BITMAP32, 4,      \
					  0), /* feature map */                                                        \
		DECLARE_DYNAMIC_ATTRIBUTE_LIST_END();

inline void IdentifyStartDefaultCb(Identify *identify)
{
	VerifyOrReturn(identify);
	ChipLogProgress(DeviceLayer, "Starting bridged device identify on endpoint %d", identify->mEndpoint);
}

inline void IdentifyStopDefaultCb(Identify *identify)
{
	VerifyOrReturn(identify);
	ChipLogProgress(DeviceLayer, "Stopping bridged device identify on endpoint %d", identify->mEndpoint);
}

class MatterBridgedDevice {
public:
	enum DeviceType : uint16_t {
		BridgedNode = 0x0013,
		OnOffLight = 0x0100,
		OnOffLightSwitch = 0x0103,
		TemperatureSensor = 0x0302,
		HumiditySensor = 0x0307,
		GenericSwitch = 0x000F
	};
	using IdentifyType = chip::app::Clusters::Identify::IdentifyTypeEnum;
	static constexpr uint8_t kDefaultDynamicEndpointVersion = 3;
	static constexpr uint8_t kNodeLabelSize = 32;
	static constexpr uint8_t kUniqueIDSize = chip::DeviceLayer::ConfigurationManager::kMaxUniqueIDLength;
	static constexpr uint8_t kDescriptorAttributeArraySize = 254;

	explicit MatterBridgedDevice(const char *uniqueID, const char *nodeLabel)
		: mIdentifyServer(mEndpointId, IdentifyStartDefaultCb, IdentifyStopDefaultCb,
				  IdentifyType::kVisibleIndicator)
	{
		if (uniqueID) {
			memcpy(mUniqueID, uniqueID, strlen(uniqueID));
		}
		if (nodeLabel) {
			memcpy(mNodeLabel, nodeLabel, strlen(nodeLabel));
		}
	}
	virtual ~MatterBridgedDevice() { chip::Platform::MemoryFree(mDataVersion); }

	void Init(chip::EndpointId endpoint)
	{
		mEndpointId = endpoint;
		mIdentifyServer.mEndpoint = mEndpointId;
		ConfigureIdentifyServer(&mIdentifyServer);
	}
	chip::EndpointId GetEndpointId() const { return mEndpointId; }

	virtual uint16_t GetDeviceType() const = 0;
	virtual CHIP_ERROR HandleRead(chip::ClusterId clusterId, chip::AttributeId attributeId, uint8_t *buffer,
				      uint16_t maxReadLength) = 0;
	virtual CHIP_ERROR HandleWrite(chip::ClusterId clusterId, chip::AttributeId attributeId, uint8_t *buffer,
				       size_t size) = 0;
	virtual CHIP_ERROR HandleAttributeChange(chip::ClusterId clusterId, chip::AttributeId attributeId, void *data,
						 size_t dataSize) = 0;

	virtual void ConfigureIdentifyServer(Identify *identifyServer) {};
	CHIP_ERROR CopyAttribute(const void *attribute, size_t attributeSize, void *buffer, uint16_t maxBufferSize);
	CHIP_ERROR HandleWriteDeviceBasicInformation(chip::ClusterId clusterId, chip::AttributeId attributeId,
						     void *data, size_t dataSize);
	CHIP_ERROR HandleReadBridgedDeviceBasicInformation(chip::AttributeId attributeId, uint8_t *buffer,
							   uint16_t maxReadLength);

	CHIP_ERROR HandleReadIdentify(chip::AttributeId attributeId, uint8_t *buffer, uint16_t maxReadLength);
	CHIP_ERROR HandleWriteIdentify(chip::AttributeId attributeId, void *data, size_t dataSize);

	bool GetIsReachable() const { return mIsReachable; }
	const char *GetUniqueID() const { return mUniqueID; }
	const char *GetNodeLabel() const { return mNodeLabel; }
	static constexpr uint16_t GetBridgedDeviceBasicInformationClusterRevision() { return 4; }
	static constexpr uint32_t GetBridgedDeviceBasicInformationFeatureMap() { return 0; }
	static constexpr uint16_t GetIdentifyClusterRevision() { return 5; }
	static constexpr uint32_t GetIdentifyClusterFeatureMap() { return 0; }

	EmberAfEndpointType *mEp;
	const EmberAfDeviceType *mDeviceTypeList;
	size_t mDeviceTypeListSize;
	chip::DataVersion *mDataVersion;
	size_t mDataVersionSize;

protected:
	void SetIsReachable(bool isReachable) { mIsReachable = isReachable; }
	void SetNodeLabel(void *data, size_t size);

	chip::EndpointId mEndpointId{};

private:
	bool mIsReachable = true;
	char mUniqueID[kUniqueIDSize] = "";
	char mNodeLabel[kNodeLabelSize] = "";
	Identify mIdentifyServer;
	uint16_t mIdentifyTime{};
};

} /* namespace Nrf */
