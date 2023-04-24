/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "app_event.h"
#include "led_widget.h"
#include "window_covering.h"

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

class AppTask {
public:
	static AppTask &Instance()
	{
		static AppTask sAppTask;
		return sAppTask;
	};

	CHIP_ERROR StartApp();

	static void IdentifyStartHandler(Identify *);
	static void IdentifyStopHandler(Identify *);

private:
	CHIP_ERROR Init();
	void ToggleMoveType();

	static void CancelTimer();
	static void StartTimer(uint32_t timeoutMs);

	static void PostEvent(const AppEvent &event);
	static void DispatchEvent(const AppEvent &event);
	static void FunctionTimerEventHandler(const AppEvent &event);
	static void FunctionHandler(const AppEvent &event);
	static void UpdateLedStateEventHandler(const AppEvent &event);
	static void StartBLEAdvertisementHandler(const AppEvent &event);
	static void MovementTimerEventHandler(const AppEvent &event);
	static void OpenHandler(const AppEvent &event);
	static void CloseHandler(const AppEvent &event);

	static void TimerTimeoutCallback(k_timer *timer);
	static void FunctionTimerTimeoutCallback(k_timer *timer);
	static void LEDStateUpdateHandler(LEDWidget &ledWidget);

	static void ChipEventHandler(const chip::DeviceLayer::ChipDeviceEvent *event, intptr_t arg);
	static void ButtonEventHandler(uint32_t buttonState, uint32_t hasChanged);
	static void UpdateStatusLED();

	FunctionEvent mFunction{ FunctionEvent::NoneSelected };
	OperationalState mMoveType{ OperationalState::MovingUpOrOpen };
	bool mFunctionTimerActive{ false };
	bool mMovementTimerActive{ false };
	bool mOpenButtonIsPressed{ false };
	bool mCloseButtonIsPressed{ false };
	bool mMoveTypeRecentlyChanged{ false };

#if CONFIG_CHIP_FACTORY_DATA
	chip::DeviceLayer::FactoryDataProvider<chip::DeviceLayer::InternalFlashFactoryData> mFactoryDataProvider;
#endif
};
