/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "bridged_device_data_provider.h"

class OnOffLightDataProvider : public BridgedDeviceDataProvider {
public:
	static constexpr uint16_t kOnOffIntervalMs = 30000;

	OnOffLightDataProvider(UpdateAttributeCallback callback) : BridgedDeviceDataProvider(callback) {}
	~OnOffLightDataProvider() { k_timer_stop(&mTimer); }

	void Init() override;
	void NotifyUpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId, void *data,
			       size_t dataSize) override;
	CHIP_ERROR UpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId, uint8_t *buffer) override;

	static void TimerTimeoutCallback(k_timer *timer);
	static void NotifyAttributeChange(intptr_t context);

private:
	k_timer mTimer;
	bool mOnOff = false;
};
