/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "app_event.h"
#include "led_widget.h"
#include "lighting_manager.h"

#include <platform/CHIPDeviceLayer.h>

#if CONFIG_CHIP_FACTORY_DATA
#include <platform/nrfconnect/FactoryDataProvider.h>
#endif

#ifdef CONFIG_MCUMGR_SMP_BT
#include "dfu_over_smp.h"
#endif

struct k_timer;

class AppTask {
public:
	static constexpr size_t APP_EVENT_QUEUE_SIZE = 10;

	CHIP_ERROR StartApp();

	void PostEvent(const AppEvent &aEvent);
	void UpdateClusterState();

private:
	CHIP_ERROR Init();

	void CancelFunctionTimer();
	void StartFunctionTimer(uint32_t timeoutInMs);

	void DispatchEvent(const AppEvent &event);
	void FunctionPressHandler();
	void FunctionReleaseHandler();
	void FunctionTimerEventHandler();
	void StartBLEAdvertisingHandler();

	static void ActionInitiated(LightingManager::Action aAction);
	static void ActionCompleted(LightingManager::Action aAction);
	static void UpdateStatusLED();
	static void ButtonEventHandler(uint32_t buttonState, uint32_t hasChanged);
	static void TimerEventHandler(k_timer *timer);
	static void LEDStateUpdateHandler(LEDWidget &ledWidget);
	static void ChipEventHandler(const chip::DeviceLayer::ChipDeviceEvent *event, intptr_t arg);
#ifdef CONFIG_MCUMGR_SMP_BT
	static void RequestSMPAdvertisingStart(void);
#endif

	friend AppTask &GetAppTask();

	enum class TimerFunction { NoneSelected = 0, SoftwareUpdate, FactoryReset };

	TimerFunction mFunction = TimerFunction::NoneSelected;

	static AppTask sAppTask;

#if CONFIG_CHIP_FACTORY_DATA
	chip::DeviceLayer::FactoryDataProvider<chip::DeviceLayer::InternalFlashFactoryData> mFactoryDataProvider;
#endif
};

inline AppTask &GetAppTask()
{
	return AppTask::sAppTask;
}
