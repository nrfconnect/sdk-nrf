/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "app_event.h"

#include <cstdint>

struct k_timer;

class AppTask {
public:
	int StartApp();
	void PostEvent(const AppEvent &event);

private:
	int Init();

	void DispatchEvent(const AppEvent &event);
	void StartDiscoveryHandler();

	static void ButtonEventHandler(uint32_t buttonsState, uint32_t hasChanged);
	static void LightLevelTimerHandler(k_timer *);
	static void DiscoveryTimeoutHandler(k_timer *);

	friend AppTask &GetAppTask();
	static AppTask sAppTask;
};

inline AppTask &GetAppTask()
{
	return AppTask::sAppTask;
}
