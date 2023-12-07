/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "matter_bridged_device.h"

class OnOffLightSwitchDevice : public MatterBridgedDevice {
public:
	static constexpr uint16_t kOnOffClusterRevision = 4;
	static constexpr uint32_t kOnOffFeatureMap = 1;
	static constexpr uint16_t kBindingClusterRevision = 1;
	static constexpr uint32_t kBindingFeatureMap = 0;

	OnOffLightSwitchDevice(const char *nodeLabel);

	uint16_t GetOnOffClusterRevision() { return kOnOffClusterRevision; }
	uint32_t GetOnOffFeatureMap() { return kOnOffFeatureMap; }
	uint16_t GetBindingClusterRevision() { return kBindingClusterRevision; }
	uint32_t GetBindingFeatureMap() { return kBindingFeatureMap; }

	MatterBridgedDevice::DeviceType GetDeviceType() const override
	{
		return MatterBridgedDevice::DeviceType::OnOffLightSwitch;
	}
	CHIP_ERROR HandleRead(chip::ClusterId clusterId, chip::AttributeId attributeId, uint8_t *buffer,
			      uint16_t maxReadLength) override;
	CHIP_ERROR HandleReadOnOff(chip::AttributeId attributeId, uint8_t *buffer, uint16_t maxReadLength);
	CHIP_ERROR HandleWrite(chip::ClusterId clusterId, chip::AttributeId attributeId, uint8_t *buffer) override
	{
		return CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE;
	}
	CHIP_ERROR HandleAttributeChange(chip::ClusterId clusterId, chip::AttributeId attributeId, void *data,
					 size_t dataSize) override
	{
		return CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE;
	}

private:
};
