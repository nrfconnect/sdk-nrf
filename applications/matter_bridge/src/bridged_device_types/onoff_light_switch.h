/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "matter_bridged_device.h"

class OnOffLightSwitchDevice : public Nrf::MatterBridgedDevice {
public:
	OnOffLightSwitchDevice(const char *nodeLabel);

	uint16_t GetDeviceType() const override { return Nrf::MatterBridgedDevice::DeviceType::OnOffLightSwitch; }
	CHIP_ERROR HandleRead(chip::ClusterId clusterId, chip::AttributeId attributeId, uint8_t *buffer,
			      uint16_t maxReadLength) override;
	CHIP_ERROR HandleReadOnOff(chip::AttributeId attributeId, uint8_t *buffer, uint16_t maxReadLength);
	CHIP_ERROR HandleReadBinding(chip::AttributeId attributeId, uint8_t *buffer, uint16_t maxReadLength);
	CHIP_ERROR HandleWrite(chip::ClusterId clusterId, chip::AttributeId attributeId, uint8_t *buffer) override
	{
		return CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE;
	}
	CHIP_ERROR HandleAttributeChange(chip::ClusterId clusterId, chip::AttributeId attributeId, void *data,
					 size_t dataSize) override
	{
		return CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE;
	}

	static constexpr uint16_t GetOnOffClusterRevision() { return 4; }
	static constexpr uint32_t GetOnOffFeatureMap() { return 1; }
	static constexpr uint16_t GetBindingClusterRevision() { return 1; }
	static constexpr uint32_t GetBindingFeatureMap() { return 0; }
};
