/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "bridged_device_data_provider.h"

class SimulatedOnOffLightSwitchDataProvider : public Nrf::BridgedDeviceDataProvider {
public:
	SimulatedOnOffLightSwitchDataProvider(UpdateAttributeCallback updateCallback, InvokeCommandCallback commandCallback) : Nrf::BridgedDeviceDataProvider(updateCallback, commandCallback) {}
	~SimulatedOnOffLightSwitchDataProvider() {}

	void Init() override;
	void NotifyUpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId, void *data,
			       size_t dataSize) override;
	CHIP_ERROR UpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId, uint8_t *buffer) override;

private:
};
