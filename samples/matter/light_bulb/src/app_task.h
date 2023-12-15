/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "board.h"
#include "pwm_device.h"

#include <platform/CHIPDeviceLayer.h>

#if CONFIG_CHIP_FACTORY_DATA
#include <platform/nrfconnect/FactoryDataProvider.h>
#else
#include <platform/nrfconnect/DeviceInstanceInfoProviderImpl.h>
#endif

#ifdef CONFIG_MCUMGR_TRANSPORT_BT
#include "dfu_over_smp.h"
#endif

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

	static void ChipEventHandler(const chip::DeviceLayer::ChipDeviceEvent *event, intptr_t arg);

	static void ActionInitiated(PWMDevice::Action_t action, int32_t actor);
	static void ActionCompleted(PWMDevice::Action_t action, int32_t actor);

	PWMDevice mPWMDevice;
#if CONFIG_CHIP_FACTORY_DATA
	chip::DeviceLayer::FactoryDataProvider<chip::DeviceLayer::InternalFlashFactoryData> mFactoryDataProvider;
#endif
};
