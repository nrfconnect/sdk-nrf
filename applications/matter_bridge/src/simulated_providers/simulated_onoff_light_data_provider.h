/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "bridged_device_data_provider.h"

class SimulatedOnOffLightDataProvider : public BridgedDeviceDataProvider {
public:
	SimulatedOnOffLightDataProvider(UpdateAttributeCallback updateCallback, InvokeCommandCallback commandCallback) : BridgedDeviceDataProvider(updateCallback, commandCallback) {}
	~SimulatedOnOffLightDataProvider()
	{
#ifdef CONFIG_BRIDGED_DEVICE_SIMULATED_ONOFF_AUTOMATIC
		k_timer_stop(&mTimer);
#endif
	}

	void Init() override;
	void NotifyUpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId, void *data,
			       size_t dataSize) override;
	CHIP_ERROR UpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId, uint8_t *buffer) override;

private:
	static void NotifyAttributeChange(intptr_t context);

#ifdef CONFIG_BRIDGED_DEVICE_SIMULATED_ONOFF_AUTOMATIC
	static void TimerTimeoutCallback(k_timer *timer);
	static constexpr uint16_t kOnOffIntervalMs = 30000;
	k_timer mTimer;
#endif
	bool mOnOff = false;
};
