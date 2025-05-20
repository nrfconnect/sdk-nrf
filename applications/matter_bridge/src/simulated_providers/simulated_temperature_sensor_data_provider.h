/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "bridged_device_data_provider.h"

#include <zephyr/kernel.h>

class SimulatedTemperatureSensorDataProvider : public Nrf::BridgedDeviceDataProvider {
public:
	SimulatedTemperatureSensorDataProvider(UpdateAttributeCallback updateCallback,
					       InvokeCommandCallback commandCallback)
		: Nrf::BridgedDeviceDataProvider(updateCallback, commandCallback)
	{
	}
	~SimulatedTemperatureSensorDataProvider() { k_timer_stop(&mTimer); }

	void Init() override;
	void NotifyUpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId, void *data,
			       size_t dataSize) override;
	CHIP_ERROR UpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId, uint8_t *buffer) override;

private:
	static constexpr uint16_t kMeasurementsIntervalMs = 10000;
	static constexpr int16_t kMinRandomTemperature = -10;
	static constexpr int16_t kMaxRandomTemperature = 10;

	static void TimerTimeoutCallback(k_timer *timer);

	k_timer mTimer;
	int16_t mTemperature = 0;
};
