/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "matter_bridged_device.h"

class HumiditySensorDevice : public Nrf::MatterBridgedDevice {
public:
	HumiditySensorDevice(const char *uniqueID, const char *nodeLabel);

	uint16_t GetMeasuredValue() { return mMeasuredValue; }
	uint16_t GetMinMeasuredValue() { return mMinMeasuredValue; }
	uint16_t GetMaxMeasuredValue() { return mMaxMeasuredValue; }

	uint16_t GetDeviceType() const override { return Nrf::MatterBridgedDevice::DeviceType::HumiditySensor; }
	CHIP_ERROR HandleRead(chip::ClusterId clusterId, chip::AttributeId attributeId, uint8_t *buffer,
			      uint16_t maxReadLength) override;
	CHIP_ERROR HandleReadRelativeHumidityMeasurement(chip::AttributeId attributeId, uint8_t *buffer,
							 uint16_t maxReadLength);
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
	}
	CHIP_ERROR HandleAttributeChange(chip::ClusterId clusterId, chip::AttributeId attributeId, void *data,
					 size_t dataSize) override;

	static constexpr uint16_t GetRelativeHumidityMeasurementClusterRevision() { return 3; }
	static constexpr uint32_t GetRelativeHumidityMeasurementFeatureMap() { return 0; }

private:
	void SetMeasuredValue(int16_t value) { mMeasuredValue = value; }
	void SetMinMeasuredValue(int16_t value) { mMinMeasuredValue = value; }
	void SetMaxMeasuredValue(int16_t value) { mMaxMeasuredValue = value; }

	uint16_t mMeasuredValue = 0;
	uint16_t mMinMeasuredValue = CONFIG_BRIDGE_HUMIDITY_SENSOR_MIN_MEASURED_VALUE;
	uint16_t mMaxMeasuredValue = CONFIG_BRIDGE_HUMIDITY_SENSOR_MAX_MEASURED_VALUE;
};
