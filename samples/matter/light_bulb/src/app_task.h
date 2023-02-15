/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "app_event.h"
#include "led_widget.h"
#include "pwm_device.h"

#include <platform/CHIPDeviceLayer.h>

#if CONFIG_CHIP_FACTORY_DATA
#include <platform/nrfconnect/FactoryDataProvider.h>
#endif

#ifdef CONFIG_MCUMGR_TRANSPORT_BT
#include "dfu_over_smp.h"
#endif

struct k_timer;
struct Identify;

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

private:
	CHIP_ERROR Init();

	void CancelTimer();
	void StartTimer(uint32_t timeoutInMs);

	static void PostEvent(const AppEvent &event);
	static void DispatchEvent(const AppEvent &event);
	static void FunctionTimerEventHandler(const AppEvent &event);
	static void LightingActionEventHandler(const AppEvent &event);
	static void StartBLEAdvertisementHandler(const AppEvent &event);
	static void UpdateLedStateEventHandler(const AppEvent &event);

	static void ChipEventHandler(const chip::DeviceLayer::ChipDeviceEvent *event, intptr_t arg);
	static void ButtonEventHandler(uint32_t buttonState, uint32_t hasChanged);
	static void FunctionTimerTimeoutCallback(k_timer *timer);

	static void ActionInitiated(PWMDevice::Action_t action, int32_t actor);
	static void ActionCompleted(PWMDevice::Action_t action, int32_t actor);
	static void UpdateStatusLED();
	static void LEDStateUpdateHandler(LEDWidget &ledWidget);
	static void FunctionHandler(const AppEvent &event);
	static void StartBLEAdvertisementAndLightActionEventHandler(const AppEvent &event);

	FunctionEvent mFunction = FunctionEvent::NoneSelected;
	bool mFunctionTimerActive = false;
	PWMDevice mPWMDevice;
#if CONFIG_CHIP_FACTORY_DATA
	chip::DeviceLayer::FactoryDataProvider<chip::DeviceLayer::InternalFlashFactoryData> mFactoryDataProvider;
#endif
};
