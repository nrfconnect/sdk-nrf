/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "app_event.h"
#include "led_widget.h"

#include <platform/CHIPDeviceLayer.h>

#if CONFIG_CHIP_FACTORY_DATA
#include <platform/nrfconnect/FactoryDataProvider.h>
#else
#include <platform/nrfconnect/DeviceInstanceInfoProviderImpl.h>
#endif

#ifdef CONFIG_MCUMGR_TRANSPORT_BT
#include "dfu_over_smp.h"
#endif

#ifdef CONFIG_CHIP_ICD_SUBSCRIPTION_HANDLING
#include "icd_util.h"
#endif

struct k_timer;
struct Identify;
class AppFabricTableDelegate;

class AppTask {
public:
	friend class AppFabricTableDelegate;

	static AppTask &Instance()
	{
		static AppTask sAppTask;
		return sAppTask;
	};

	CHIP_ERROR StartApp();

	void UpdateClusterState();

	static void IdentifyStartHandler(Identify *);
	static void IdentifyStopHandler(Identify *);

private:
	enum class Timer : uint8_t { Function, DimmerTrigger, Dimmer };
	enum class Button : uint8_t {
		Function,
		Dimmer,
	};

	CHIP_ERROR Init();

	static void PostEvent(const AppEvent &event);
	static void DispatchEvent(const AppEvent &event);
	static void ButtonPushHandler(const AppEvent &event);
	static void ButtonReleaseHandler(const AppEvent &event);
	static void TimerEventHandler(const AppEvent &event);
	static void StartBLEAdvertisementHandler(const AppEvent &event);
	static void UpdateLedStateEventHandler(const AppEvent &event);

	static void ChipEventHandler(const chip::DeviceLayer::ChipDeviceEvent *event, intptr_t arg);
	static void ButtonEventHandler(uint32_t buttonState, uint32_t hasChanged);
	static void LEDStateUpdateHandler(LEDWidget &ledWidget);
	static void FunctionTimerTimeoutCallback(k_timer *timer);
	static void UpdateStatusLED();

	static void StartTimer(Timer, uint32_t);
	static void CancelTimer(Timer);

	FunctionEvent mFunction = FunctionEvent::NoneSelected;

#if CONFIG_CHIP_FACTORY_DATA
	chip::DeviceLayer::FactoryDataProvider<chip::DeviceLayer::InternalFlashFactoryData> mFactoryDataProvider;
#endif
};
