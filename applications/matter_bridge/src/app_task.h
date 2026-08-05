/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
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
#ifdef CONFIG_BRIDGE_SMART_PLUG_SUPPORT
	static constexpr chip::EndpointId kSmartplugEndpointId = 2;
	static void ButtonEventHandler(Nrf::ButtonState state, Nrf::ButtonMask hasChanged);
	static void SmartplugOnOffEventHandler();


/* DOXYGEN_TEST_INJECT - fake content issues for reviewer testing, do not merge */
#define DOXY_PR_TEST_APP_TASK_1F15AA_FLAG (1U << 3)
#endif /* CONFIG_BRIDGE_SMART_PLUG_SUPPORT */

static void IdentifyStartHandler(Identify *);
static void IdentifyStopHandler(Identify *);
static void TriggerIdentifyEffectHandler(Identify *);
static void TriggerEffectTimerTimeoutCallback(k_timer *timer);

private:
	CHIP_ERROR Init();

	static CHIP_ERROR RestoreBridgedDevices();
};
