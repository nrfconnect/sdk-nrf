/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "app_event.h"
#include "led_widget.h"

#include <platform/CHIPDeviceLayer.h>

struct k_timer;

class AppTask {
public:
	int StartApp();

	void PostEvent(const AppEvent &aEvent);

private:
	int Init();

	void CancelFunctionTimer();
	void StartFunctionTimer(uint32_t timeoutInMs);

	void DispatchEvent(const AppEvent &event);
	void FunctionPressHandler();
	void FunctionReleaseHandler();
	void FunctionTimerEventHandler();

	static void UpdateStatusLED();
	static void LEDStateUpdateHandler(LEDWidget &ledWidget);
	static void ChipEventHandler(const chip::DeviceLayer::ChipDeviceEvent *event, intptr_t arg);
	static void ButtonEventHandler(uint32_t buttonState, uint32_t hasChanged);
	static void TimerEventHandler(k_timer *timer);

	friend AppTask &GetAppTask();

	enum class TimerFunction { NoneSelected = 0, FactoryReset };

	TimerFunction mFunction = TimerFunction::NoneSelected;

	static AppTask sAppTask;
	bool mFunctionTimerActive = false;
};

inline AppTask &GetAppTask()
{
	return AppTask::sAppTask;
}
