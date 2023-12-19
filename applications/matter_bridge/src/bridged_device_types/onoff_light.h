/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "matter_bridged_device.h"
class OnOffLightDevice : public MatterBridgedDevice {
public:
	static constexpr uint16_t kOnOffClusterRevision = 4;
	static constexpr uint32_t kOnOffFeatureMap = 1;
	static constexpr uint16_t kGroupsClusterRevision = 4;
	static constexpr uint32_t kGroupsFeatureMap = 0;
	static constexpr uint8_t kGroupsNameSupportMap = 0;

	OnOffLightDevice(const char *nodeLabel);

	bool GetOnOff() { return mOnOff; }
	void Toggle() { mOnOff = !mOnOff; }
	uint16_t GetOnOffClusterRevision() { return kOnOffClusterRevision; }
	uint32_t GetOnOffFeatureMap() { return kOnOffFeatureMap; }
	uint16_t GetGroupsClusterRevision() { return kGroupsClusterRevision; }
	uint32_t GetGroupsFeatureMap() { return kGroupsFeatureMap; }
	uint8_t GetGroupsNameSupportMap() { return kGroupsNameSupportMap; }

	uint16_t GetDeviceType() const override
	{
		return MatterBridgedDevice::DeviceType::OnOffLight;
	}
	CHIP_ERROR HandleRead(chip::ClusterId clusterId, chip::AttributeId attributeId, uint8_t *buffer,
			      uint16_t maxReadLength) override;
	CHIP_ERROR HandleReadOnOff(chip::AttributeId attributeId, uint8_t *buffer, uint16_t maxReadLength);
	CHIP_ERROR HandleReadGroups(chip::AttributeId attributeId, uint8_t *buffer, uint16_t maxReadLength);
	CHIP_ERROR HandleWrite(chip::ClusterId clusterId, chip::AttributeId attributeId, uint8_t *buffer) override;
	CHIP_ERROR HandleAttributeChange(chip::ClusterId clusterId, chip::AttributeId attributeId, void *data,
					 size_t dataSize) override;

private:
	void SetOnOff(bool onOff) { mOnOff = onOff; }

	bool mOnOff = false;
};
