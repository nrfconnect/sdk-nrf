/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "board.h"
#include "pwm_device.h"

#include <platform/CHIPDeviceLayer.h>

struct k_timer;
struct Identify;

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
	PWMDevice &GetPWMDevice() { return mPWMDevice; }

	static void IdentifyStartHandler(Identify *);
	static void IdentifyStopHandler(Identify *);
	static void TriggerIdentifyEffectHandler(Identify *);
	static void TriggerEffectTimerTimeoutCallback(k_timer *timer);

private:
	CHIP_ERROR Init();

	static void LightingActionEventHandler(const LightingEvent &event);
	static void ButtonEventHandler(ButtonState state, ButtonMask hasChanged);

	static void MatterEventHandler(const chip::DeviceLayer::ChipDeviceEvent *event, intptr_t arg);

	static void ActionInitiated(PWMDevice::Action_t action, int32_t actor);
	static void ActionCompleted(PWMDevice::Action_t action, int32_t actor);

#ifdef CONFIG_AWS_IOT_INTEGRATION
	static bool AWSIntegrationCallback(struct aws_iot_integration_cb_data *data);
#endif

	PWMDevice mPWMDevice;
};
