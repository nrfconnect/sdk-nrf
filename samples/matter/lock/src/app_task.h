/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "app_event.h"
#include "bolt_lock_manager.h"
#include "led_widget.h"

#include <platform/CHIPDeviceLayer.h>

#ifdef CONFIG_MCUMGR_SMP_BT
#include "dfu_over_smp.h"
#endif

struct k_timer;

class AppTask {
public:
	CHIP_ERROR StartApp();

	void PostEvent(const AppEvent &aEvent);
	void UpdateClusterState(BoltLockManager::State state, BoltLockManager::OperationSource source);

private:
	CHIP_ERROR Init();

	void CancelFunctionTimer();
	void StartFunctionTimer(uint32_t timeoutInMs);

	void DispatchEvent(const AppEvent &event);
	void FunctionPressHandler();
	void FunctionReleaseHandler();
	void FunctionTimerEventHandler();
	void StartThreadHandler();
	void StartBLEAdvertisingHandler();

	static void LockStateChanged(BoltLockManager::State state, BoltLockManager::OperationSource source);
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
	bool mFunctionTimerActive = false;
};

inline AppTask &GetAppTask()
{
	return AppTask::sAppTask;
}
