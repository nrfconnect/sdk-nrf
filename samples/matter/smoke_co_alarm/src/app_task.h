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

#ifdef CONFIG_SAMPLE_MATTER_SYSTEM_OFF
#include <app/icd/server/ICDStateObserver.h>
#endif

struct k_timer;
struct Identify;

#ifdef CONFIG_SAMPLE_MATTER_SYSTEM_OFF
class AppTask : public chip::app::ICDStateObserver {
#else
class AppTask {
#endif
public:
	static AppTask &Instance()
	{
		static AppTask sAppTask;
		return sAppTask;
	};

	CHIP_ERROR StartApp();
	static void IdentifyStartHandler(Identify *);
	static void IdentifyStopHandler(Identify *);

#ifdef CONFIG_SAMPLE_MATTER_SYSTEM_OFF
	static void SystemOffWorkHandler(k_work *work);
#endif

	void UpdatedExpressedLedState();
	void SelfTestHandler();

	static constexpr chip::EndpointId kSmokeCoAlarmEndpointId = 1;
	static constexpr chip::EndpointId kWiredPowerSourceEndpointId = 0;
	static constexpr chip::EndpointId kBatteryPowerSourceEndpointId = 1;

private:
	CHIP_ERROR Init();

	static void ButtonEventHandler(Nrf::ButtonState state, Nrf::ButtonMask hasChanged);
	static void SelfTestLedTimerTimeoutCallback(k_timer *timer);
	static void SelfTestLedTimerEventHandler();
	static void SelfTestTimerTimeoutCallback(k_timer *timer);
	static void EndSelfTestEventHandler();

#ifdef CONFIG_SAMPLE_MATTER_SYSTEM_OFF
	/**
	 * @brief application has no action to do on this ICD event. Do nothing.
	 */
	void OnICDModeChange() override {};

	/**
	 * @brief application has no action to do on this ICD event. Do nothing.
	 */
	void OnEnterActiveMode() override {};

	/**
	 * @brief application has no action to do on this ICD event. Do nothing.
	 */
	void OnTransitionToIdle() override {};

	/**
	 * @brief when an ICD enters the idle mode, the application arms GRTC for wake up purposes and turns off the
	 * system.
	 */
	void OnEnterIdleMode() override;

	/* The device has to be active at least every ICD slow poll interval, but we have to wake up system earlier to
	 * boot, init and re-attach to network. Usually the whole procedure takes less than 2 s, but the wake up
	 * interval was set to 2.5 s to always be on the safe side.
	 */
	constexpr static auto kSystemOffWakeUpInterval = CONFIG_CHIP_ICD_SLOW_POLL_INTERVAL - 2500;

#endif

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
