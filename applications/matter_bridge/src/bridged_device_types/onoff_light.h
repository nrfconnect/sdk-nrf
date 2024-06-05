/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "matter_bridged_device.h"

class OnOffLightDevice : public Nrf::MatterBridgedDevice {
public:
	OnOffLightDevice(const char *nodeLabel);

	bool GetOnOff() { return mOnOff; }
	void Toggle() { mOnOff = !mOnOff; }

	uint16_t GetDeviceType() const override { return Nrf::MatterBridgedDevice::DeviceType::OnOffLight; }
	CHIP_ERROR HandleRead(chip::ClusterId clusterId, chip::AttributeId attributeId, uint8_t *buffer,
			      uint16_t maxReadLength) override;
	CHIP_ERROR HandleReadOnOff(chip::AttributeId attributeId, uint8_t *buffer, uint16_t maxReadLength);
	CHIP_ERROR HandleReadGroups(chip::AttributeId attributeId, uint8_t *buffer, uint16_t maxReadLength);
	CHIP_ERROR HandleWrite(chip::ClusterId clusterId, chip::AttributeId attributeId, uint8_t *buffer, size_t size) override;
	CHIP_ERROR HandleAttributeChange(chip::ClusterId clusterId, chip::AttributeId attributeId, void *data,
					 size_t dataSize) override;

	static constexpr uint16_t GetOnOffClusterRevision() { return 5; }
	static constexpr uint32_t GetOnOffFeatureMap() { return 1; }
	static constexpr uint16_t GetGroupsClusterRevision() { return 4; }
	static constexpr uint32_t GetGroupsFeatureMap() { return 1; }
	static constexpr uint8_t GetGroupsNameSupportMap() { return 0; }

private:
	bool mOnOff{ false };
	bool mGlobalSceneControl{ false };
	int16_t mOnTime{ 0 };
	int16_t mOffWaitTime{ 0 };
	chip::app::Clusters::OnOff::StartUpOnOffEnum mStartUpOnOff{
		chip::app::Clusters::OnOff::StartUpOnOffEnum::kUnknownEnumValue
	};
};
