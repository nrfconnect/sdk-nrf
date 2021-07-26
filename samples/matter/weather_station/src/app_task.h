/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "app_event.h"

#include <platform/CHIPDeviceLayer.h>

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

	static int SoftwareUpdateConfirmationHandler(uint32_t offset, uint32_t size, void *arg);
	static void ButtonStateHandler(uint32_t buttonState, uint32_t hasChanged);
	static void ButtonPushHandler(AppEvent *event);
	static void ButtonReleaseHandler(AppEvent *event);
	static void FunctionTimerHandler(AppEvent *event);
	static void ChipEventHandler(const chip::DeviceLayer::ChipDeviceEvent *event, intptr_t arg);

	static AppTask sAppTask;
};

inline AppTask &GetAppTask()
{
	return AppTask::sAppTask;
}
