/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "board/board.h"
#include "pwm/pwm_device.h"

#include <platform/CHIPDeviceLayer.h>

struct k_timer;

enum class LightingActor : uint8_t { Remote, Button };

struct LightingEvent {
	uint8_t Action;
	LightingActor Actor;
};

class AppTask {
public:
	static AppTask &Instance()
	{
		static AppTask sAppTask;
		return sAppTask;
	};

	CHIP_ERROR StartApp();

	void UpdateClusterState();
	void InitPWMDDevice();
	Nrf::PWMDevice &GetPWMDevice() { return mPWMDevice; }

private:
	CHIP_ERROR Init();

	static void LightingActionEventHandler(const LightingEvent &event);
	static void ButtonEventHandler(Nrf::ButtonState state, Nrf::ButtonMask hasChanged);

	static void ActionInitiated(Nrf::PWMDevice::Action_t action, int32_t actor);
	static void ActionCompleted(Nrf::PWMDevice::Action_t action, int32_t actor);


	Nrf::PWMDevice mPWMDevice;
};
