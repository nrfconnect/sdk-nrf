/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "bridged_device_data_provider.h"

class SimulatedHumiditySensorDataProvider : public BridgedDeviceDataProvider {
public:
	static constexpr uint16_t kMeasurementsIntervalMs = 10000;
	static constexpr uint16_t kMinRandomTemperature = 30;
	static constexpr uint16_t kMaxRandomTemperature = 50;

	SimulatedHumiditySensorDataProvider(UpdateAttributeCallback callback) : BridgedDeviceDataProvider(callback) {}
	~SimulatedHumiditySensorDataProvider() { k_timer_stop(&mTimer); }

	void Init() override;
	void NotifyUpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId, void *data,
			       size_t dataSize) override;
	CHIP_ERROR UpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId, uint8_t *buffer) override;

	static void TimerTimeoutCallback(k_timer *timer);
	static void NotifyAttributeChange(intptr_t context);

private:
	k_timer mTimer;
	uint16_t mHumidity = 0;
};
