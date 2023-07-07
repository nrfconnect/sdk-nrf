/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "bridged_device_data_provider.h"

#include <zephyr/kernel.h>

class TemperatureSensorDataProvider : public BridgedDeviceDataProvider {
public:
	static constexpr uint16_t kMeasurementsIntervalMs = 10000;
	static constexpr int16_t kMinRandomTemperature = -10;
	static constexpr int16_t kMaxRandomTemperature = 10;

	TemperatureSensorDataProvider(UpdateAttributeCallback callback) : BridgedDeviceDataProvider(callback) {}

	void Init() override;
	void NotifyUpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId, void *data,
			       size_t dataSize) override;
	CHIP_ERROR UpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId, uint8_t *buffer) override;

	static void TimerTimeoutCallback(k_timer *timer);
	static void NotifyAttributeChange(intptr_t context);

private:
	k_timer mTimer;
	int16_t mTemperature = 0;
};
