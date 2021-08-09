/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "app_event.h"

#include <platform/CHIPDeviceLayer.h>

#ifdef CONFIG_MCUMGR_SMP_BT
#include "dfu_over_smp.h"
#endif

struct k_timer;

class AppTask {
public:
	int StartApp();

	void PostEvent(const AppEvent *aEvent);
	void PostEvent(AppEvent::Type type, AppEvent::Handler handler);
	void UpdateClusterState();
	void UpdateLedState();

private:
	friend AppTask &GetAppTask();

	int Init();
	void OpenPairingWindow();
	void DispatchEvent(AppEvent *event);

#ifdef CONFIG_MCUMGR_SMP_BT
	static void RequestSMPAdvertisingStart(void);
#endif
	static void ButtonStateHandler(uint32_t buttonState, uint32_t hasChanged);
	static void ButtonPushHandler(AppEvent *event);
	static void ButtonReleaseHandler(AppEvent *event);
	static void FunctionTimerHandler(AppEvent *event);
	static void MeasurementsTimerHandler(AppEvent *event);
	static void ChipEventHandler(const chip::DeviceLayer::ChipDeviceEvent *event, intptr_t arg);

	static AppTask sAppTask;
};

inline AppTask &GetAppTask()
{
	return AppTask::sAppTask;
}
