/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "app_event.h"
#include "bolt_lock_manager.h"

#include <platform/CHIPDeviceLayer.h>

struct k_timer;

class AppTask {
public:
	int StartApp();

	void PostEvent(const AppEvent &aEvent);
	void UpdateClusterState();

private:
	int Init();

	void CancelFunctionTimer();
	void StartFunctionTimer(uint32_t timeoutInMs);

	void DispatchEvent(const AppEvent &event);
	void LockActionHandler(BoltLockManager::Action action, bool chipInitiated);
	void CompleteLockActionHandler();
	void FunctionPressHandler();
	void FunctionReleaseHandler();
	void FunctionTimerEventHandler();
	void StartThreadHandler();
	void StartBLEAdvertisingHandler();

#ifdef CONFIG_CHIP_NFC_COMMISSIONING
	int StartNFCTag();
#endif

	static void ButtonEventHandler(uint32_t buttonState, uint32_t hasChanged);
	static void TimerEventHandler(k_timer *timer);
	static void ThreadProvisioningHandler(const chip::DeviceLayer::ChipDeviceEvent *event, intptr_t arg);
	static int SoftwareUpdateConfirmationHandler(uint32_t offset, uint32_t size, void *arg);

	friend AppTask &GetAppTask();

	enum class TimerFunction { NoneSelected = 0, SoftwareUpdate, FactoryReset };

	TimerFunction mFunction = TimerFunction::NoneSelected;

	static AppTask sAppTask;
	bool mFunctionTimerActive = false;
	bool mSoftwareUpdateEnabled = false;
};

inline AppTask &GetAppTask()
{
	return AppTask::sAppTask;
}
