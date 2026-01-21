/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "board/board.h"

#include <platform/CHIPDeviceLayer.h>

#ifdef CONFIG_NCS_SAMPLE_MATTER_TEST_EVENT_TRIGGERS
#include "event_triggers/event_triggers.h"
#endif

struct k_timer;
struct Identify;

class AppTask {
public:
	static AppTask &Instance()
	{
		static AppTask sAppTask;
		return sAppTask;
	};

	static constexpr chip::EndpointId kSmokeCoAlarmEndpointId = 1;

	CHIP_ERROR StartApp();

	void UpdatedExpressedLedState();
	void SelfTestHandler();

private:
	CHIP_ERROR Init();

	static void ButtonEventHandler(Nrf::ButtonState state, Nrf::ButtonMask hasChanged);
	static void SelfTestLedTimerTimeoutCallback(k_timer *timer);
	static void SelfTestLedTimerEventHandler();
	static void SelfTestTimerTimeoutCallback(k_timer *timer);
	static void EndSelfTestEventHandler();

#ifdef CONFIG_NCS_SAMPLE_MATTER_TEST_EVENT_TRIGGERS
	constexpr static Nrf::Matter::TestEventTrigger::EventTriggerId kPowerSourceOnEventTriggerId =
		0xFFFF'FFFF'8000'0000;
	constexpr static Nrf::Matter::TestEventTrigger::EventTriggerId kPowerSourceOffEventTriggerId =
		0xFFFF'FFFF'8001'0000;
	static CHIP_ERROR PowerSourceOnEventCallback(Nrf::Matter::TestEventTrigger::TriggerValue);
	static CHIP_ERROR PowerSourceOffEventCallback(Nrf::Matter::TestEventTrigger::TriggerValue);
#endif

	k_timer mSelfTestLedTimer;
	k_timer mSelfTestTimer;
	chip::app::Clusters::SmokeCoAlarm::ExpressedStateEnum mCurrentState;
};
