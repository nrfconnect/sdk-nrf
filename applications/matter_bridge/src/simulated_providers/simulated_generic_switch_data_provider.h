/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "bridged_device_data_provider.h"

class SimulatedGenericSwitchDataProvider : public Nrf::BridgedDeviceDataProvider {
public:
	SimulatedGenericSwitchDataProvider(UpdateAttributeCallback updateCallback, InvokeCommandCallback commandCallback) : Nrf::BridgedDeviceDataProvider(updateCallback, commandCallback) {}
	~SimulatedGenericSwitchDataProvider() { k_timer_stop(&mTimer); }

	void Init() override;
	void NotifyUpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId, void *data,
			       size_t dataSize) override;
	CHIP_ERROR UpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId, uint8_t *buffer) override;

private:
	static void NotifyAttributeChange(intptr_t context);

	static void TimerTimeoutCallback(k_timer *timer);
	static constexpr uint16_t kSwitchChangePositionIntervalMs = 30000;
	k_timer mTimer;

	bool mCurrentSwitchPosition = false;
};
