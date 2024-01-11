/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "board/board.h"

#include <platform/CHIPDeviceLayer.h>

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

	void UpdateClusterState();

	static void IdentifyStartHandler(Identify *);
	static void IdentifyStopHandler(Identify *);

private:
	enum Timer : uint8_t { DimmerTrigger, Dimmer };

	CHIP_ERROR Init();

	static void DimmerTriggerEventHandler();
	static void TimerEventHandler(const Timer &event);
	static void ButtonEventHandler(Nrf::ButtonState state, Nrf::ButtonMask hasChanged);

	static void StartTimer(Timer, uint32_t);
	static void CancelTimer(Timer);
	static void UserTimerTimeoutCallback(k_timer *timer);

	static void MatterEventHandler(const chip::DeviceLayer::ChipDeviceEvent *event, intptr_t arg);
};
