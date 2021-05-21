/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "app_event.h"
#include "lighting_manager.h"

#include <platform/CHIPDeviceLayer.h>

struct k_timer;

class AppTask {
public:
	static constexpr size_t APP_EVENT_QUEUE_SIZE = 10;

	int StartApp();

	void PostEvent(const AppEvent &aEvent);
	void UpdateClusterState();

private:
	int Init();

	void DispatchEvent(const AppEvent &event);
	void CompleteLockActionHandler();
	void StartThreadHandler();
	void StartBLEAdvertisingHandler();

#ifdef CONFIG_CHIP_NFC_COMMISSIONING
	int StartNFCTag();
#endif

	static void ActionInitiated(LightingManager::Action aAction);
	static void ActionCompleted(LightingManager::Action aAction);
	static void ButtonEventHandler(uint32_t buttonState, uint32_t hasChanged);
	static void ThreadProvisioningHandler(const chip::DeviceLayer::ChipDeviceEvent *event, intptr_t arg);

	friend AppTask &GetAppTask();

	static AppTask sAppTask;
};

inline AppTask &GetAppTask()
{
	return AppTask::sAppTask;
}
