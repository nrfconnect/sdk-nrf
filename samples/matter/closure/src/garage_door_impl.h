/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "physical_device.h"
#include "physical_device_observer.h"
#include "pwm/pwm_device.h"
#include <platform/CHIPDeviceLayer.h>

/**
 * @class GarageDoorImpl
 * @brief Implementation of IPhysicalDevice using an LED
 */
class GarageDoorImpl : public IPhysicalDevice {
public:
	explicit GarageDoorImpl(const pwm_dt_spec *spec);

	CHIP_ERROR Init() override;
	CHIP_ERROR MoveTo(uint16_t position, uint16_t speed) override;
	CHIP_ERROR Stop() override;

private:
	static void TimerTimeoutCallback(chip::System::Layer *systemLayer, void *appState);
	void HandleTimer();
	Nrf::PWMDevice mPhysicalIndicator;
	const pwm_dt_spec *mSpec;
	bool mJustStarted = false;
	uint16_t mSpeed;
	uint16_t mCurrentPosition;
	uint16_t mTargetPosition;
};
