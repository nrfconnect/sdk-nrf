/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "matter_bridged_device.h"

class GenericSwitchDevice : public Nrf::MatterBridgedDevice {
public:
	GenericSwitchDevice(const char *uniqueID, const char *nodeLabel);

	uint16_t GetDeviceType() const override { return Nrf::MatterBridgedDevice::DeviceType::GenericSwitch; }
	CHIP_ERROR HandleRead(chip::ClusterId clusterId, chip::AttributeId attributeId, uint8_t *buffer,
			      uint16_t maxReadLength) override;
	CHIP_ERROR HandleAttributeChange(chip::ClusterId clusterId, chip::AttributeId attributeId, void *data,
					 size_t dataSize) override;
	CHIP_ERROR HandleWrite(chip::ClusterId clusterId, chip::AttributeId attributeId, uint8_t *buffer,
			       size_t size) override
	{
		if (clusterId != chip::app::Clusters::BridgedDeviceBasicInformation::Id) {
			return CHIP_ERROR_INVALID_ARGUMENT;
		}

		switch (attributeId) {
		case chip::app::Clusters::BridgedDeviceBasicInformation::Attributes::NodeLabel::Id:
			return HandleWriteDeviceBasicInformation(clusterId, attributeId, buffer, size);
		default:
			return CHIP_ERROR_INVALID_ARGUMENT;
		}
		return CHIP_NO_ERROR;
	};

private:
	CHIP_ERROR HandleReadSwitch(chip::AttributeId attributeId, uint8_t *buffer, uint16_t maxReadLength);

	static constexpr uint16_t GetSwitchClusterRevision() { return 2; }
	/* According to the Matter 1.2 specification: Bit 1 -> MomentarySwitch in the Switch Cluster section. */
	static constexpr uint32_t GetSwitchClusterFeatureMap() { return 2; }
	static constexpr uint32_t GetSwitchClusterNumberOfPositions() { return 2; }

	uint8_t mCurrentPosition{};
};
