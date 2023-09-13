/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <app/util/attribute-storage.h>

/* Definitions of  helper macros that are used across all bridged device types to create common mandatory clusters in a
 * consistent way. */
/* Declare Descriptor cluster attributes */
#define DESCRIPTOR_CLUSTER_ATTRIBUTES(descriptorAttrs)                                                                 \
	DECLARE_DYNAMIC_ATTRIBUTE_LIST_BEGIN(descriptorAttrs)                                                          \
	DECLARE_DYNAMIC_ATTRIBUTE(chip::app::Clusters::Descriptor::Attributes::DeviceTypeList::Id, ARRAY,              \
				  MatterBridgedDevice::kDescriptorAttributeArraySize, 0), /* device list */            \
		DECLARE_DYNAMIC_ATTRIBUTE(chip::app::Clusters::Descriptor::Attributes::ServerList::Id, ARRAY,          \
					  MatterBridgedDevice::kDescriptorAttributeArraySize, 0), /* server list */    \
		DECLARE_DYNAMIC_ATTRIBUTE(chip::app::Clusters::Descriptor::Attributes::ClientList::Id, ARRAY,          \
					  MatterBridgedDevice::kDescriptorAttributeArraySize, 0), /* client list */    \
		DECLARE_DYNAMIC_ATTRIBUTE(chip::app::Clusters::Descriptor::Attributes::PartsList::Id, ARRAY,           \
					  MatterBridgedDevice::kDescriptorAttributeArraySize, 0), /* parts list */     \
		DECLARE_DYNAMIC_ATTRIBUTE_LIST_END();

/* Declare Bridged Device Basic Information cluster attributes */
#define BRIDGED_DEVICE_BASIC_INFORMATION_CLUSTER_ATTRIBUTES(bridgedDeviceBasicAttrs)                                   \
	DECLARE_DYNAMIC_ATTRIBUTE_LIST_BEGIN(bridgedDeviceBasicAttrs)                                                  \
	DECLARE_DYNAMIC_ATTRIBUTE(chip::app::Clusters::BridgedDeviceBasicInformation::Attributes::NodeLabel::Id,       \
				  CHAR_STRING, MatterBridgedDevice::kNodeLabelSize, 0), /* NodeLabel */                \
		DECLARE_DYNAMIC_ATTRIBUTE(                                                                             \
			chip::app::Clusters::BridgedDeviceBasicInformation::Attributes::Reachable::Id, BOOLEAN, 1,     \
			0), /* Reachable */                                                                            \
		DECLARE_DYNAMIC_ATTRIBUTE(                                                                             \
			chip::app::Clusters::BridgedDeviceBasicInformation::Attributes::FeatureMap::Id, BITMAP32, 4,   \
			0), /* feature map */                                                                          \
		DECLARE_DYNAMIC_ATTRIBUTE_LIST_END();

class MatterBridgedDevice {
public:
	enum DeviceType : uint16_t {
		BridgedNode = 0x0013,
		OnOffLight = 0x0100,
		TemperatureSensor = 0x0302,
		HumiditySensor = 0x0307
	};

	static constexpr uint8_t kDefaultDynamicEndpointVersion = 1;
	static constexpr uint8_t kNodeLabelSize = 32;
	static constexpr uint8_t kDescriptorAttributeArraySize = 254;

	explicit MatterBridgedDevice(const char *nodeLabel)
	{
		if (nodeLabel) {
			memcpy(mNodeLabel, nodeLabel, strlen(nodeLabel));
		}
	}
	virtual ~MatterBridgedDevice() { chip::Platform::MemoryFree(mDataVersion); }

	void Init(chip::EndpointId endpoint) { mEndpointId = endpoint; }
	chip::EndpointId GetEndpointId() const { return mEndpointId; }

	virtual DeviceType GetDeviceType() const = 0;
	virtual CHIP_ERROR HandleRead(chip::ClusterId clusterId, chip::AttributeId attributeId, uint8_t *buffer,
				      uint16_t maxReadLength) = 0;
	virtual CHIP_ERROR HandleWrite(chip::ClusterId clusterId, chip::AttributeId attributeId, uint8_t *buffer) = 0;
	virtual CHIP_ERROR HandleAttributeChange(chip::ClusterId clusterId, chip::AttributeId attributeId, void *data,
						 size_t dataSize) = 0;
	CHIP_ERROR CopyAttribute(void *attribute, size_t attributeSize, void *buffer, uint16_t maxBufferSize);
	CHIP_ERROR HandleWriteDeviceBasicInformation(chip::ClusterId clusterId, chip::AttributeId attributeId,
						     void *data, size_t dataSize);
	CHIP_ERROR HandleReadBridgedDeviceBasicInformation(chip::AttributeId attributeId, uint8_t *buffer,
							   uint16_t maxReadLength);
	CHIP_ERROR HandleReadDescriptor(chip::AttributeId attributeId, uint8_t *buffer, uint16_t maxReadLength);

	bool GetIsReachable() const { return mIsReachable; }
	const char *GetNodeLabel() const { return mNodeLabel; }
	uint16_t GetBridgedDeviceBasicInformationClusterRevision()
	{
		return kBridgedDeviceBasicInformationClusterRevision;
	}
	uint32_t GetBridgedDeviceBasicInformationFeatureMap() { return kBridgedDeviceBasicInformationFeatureMap; }
	uint16_t GetDescriptorClusterRevision() { return kDescriptorClusterRevision; }
	uint32_t GetDescriptorFeatureMap() { return kDescrtiptorFeatureMap; }

	static void NotifyAttributeChange(intptr_t context);

	EmberAfEndpointType *mEp;
	const EmberAfDeviceType *mDeviceTypeList;
	size_t mDeviceTypeListSize;
	chip::DataVersion *mDataVersion;
	size_t mDataVersionSize;

protected:
	void SetIsReachable(bool isReachable) { mIsReachable = isReachable; }

	chip::EndpointId mEndpointId;

private:
	static constexpr uint16_t kBridgedDeviceBasicInformationClusterRevision = 1;
	static constexpr uint32_t kBridgedDeviceBasicInformationFeatureMap = 0;
	static constexpr uint16_t kDescriptorClusterRevision = 1;
	static constexpr uint32_t kDescrtiptorFeatureMap = 0;

	bool mIsReachable = true;
	char mNodeLabel[kNodeLabelSize] = "";
};
