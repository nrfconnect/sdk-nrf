/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task.h"
#include "led_widget.h"

#include <platform/CHIPDeviceLayer.h>

#include <app/server/OnboardingCodesUtil.h>
#include <app/server/Server.h>

#include <dk_buttons_and_leds.h>
#include <logging/log.h>
#include <zephyr.h>

using namespace ::chip::DeviceLayer;

LOG_MODULE_DECLARE(app);

namespace
{
static constexpr size_t kAppEventQueueSize = 10;
static constexpr uint32_t kFactoryResetTriggerTimeout = 6000;

K_MSGQ_DEFINE(sAppEventQueue, sizeof(AppEvent), kAppEventQueueSize, alignof(AppEvent));
LEDWidget sStatusLED;
LEDWidget sUnusedLED;
LEDWidget sUnusedLED_1;
LEDWidget sUnusedLED_2;

bool sIsThreadProvisioned;
bool sIsThreadEnabled;
bool sHaveBLEConnections;
bool sHaveServiceConnectivity;

k_timer sFunctionTimer;
} /* namespace */

AppTask AppTask::sAppTask;

int AppTask::Init()
{
	/* Initialize LEDs */
	LEDWidget::InitGpio();

	sStatusLED.Init(DK_LED1);
	sUnusedLED.Init(DK_LED2);
	sUnusedLED_1.Init(DK_LED3);
	sUnusedLED_2.Init(DK_LED4);

	/* Initialize buttons */
	int ret = dk_buttons_init(ButtonEventHandler);
	if (ret) {
		LOG_ERR("dk_buttons_init() failed");
		return ret;
	}

	/* Initialize function timer */
	k_timer_init(&sFunctionTimer, &AppTask::TimerEventHandler, nullptr);
	k_timer_user_data_set(&sFunctionTimer, this);

	/* Init ZCL Data Model and start server */
	InitServer();
	ConfigurationMgr().LogDeviceConfig();
	PrintOnboardingCodes(chip::RendezvousInformationFlags(chip::RendezvousInformationFlag::kBLE));

	return 0;
}

int AppTask::StartApp()
{
	int ret = Init();

	if (ret) {
		LOG_ERR("AppTask.Init() failed");
		return ret;
	}

	AppEvent event = {};
	
	while (true) {
		ret = k_msgq_get(&sAppEventQueue, &event, K_MSEC(10));

		while (!ret) {
			DispatchEvent(event);
			ret = k_msgq_get(&sAppEventQueue, &event, K_NO_WAIT);
		}

		/* Collect connectivity and configuration state from the CHIP stack.  Because the
		 * CHIP event loop is being run in a separate task, the stack must be locked
		 * while these values are queried.  However we use a non-blocking lock request
		 * (TryLockChipStack()) to avoid blocking other UI activities when the CHIP
		 * task is busy (e.g. with a long crypto operation). */

		if (PlatformMgr().TryLockChipStack()) {
			sIsThreadProvisioned = ConnectivityMgr().IsThreadProvisioned();
			sIsThreadEnabled = ConnectivityMgr().IsThreadEnabled();
			sHaveBLEConnections = (ConnectivityMgr().NumBLEConnections() != 0);
			sHaveServiceConnectivity = ConnectivityMgr().HaveServiceConnectivity();
			PlatformMgr().UnlockChipStack();
		}

		/* Update the status LED.
		 *
		 * If system has "full connectivity", keep the LED On constantly.
		 *
		 * If thread and service provisioned, but not attached to the thread network yet OR no
		 * connectivity to the service OR subscriptions are not fully established
		 * THEN blink the LED Off for a short period of time.
		 *
		 * If the system has ble connection(s) uptill the stage above, THEN blink the LED at an even
		 * rate of 100ms.
		 *
		 * Otherwise, blink the LED On for a very short time. */
		if (sHaveServiceConnectivity) {
			sStatusLED.Set(true);
		} else if (sIsThreadProvisioned && sIsThreadEnabled) {
			sStatusLED.Blink(950, 50);
		} else if (sHaveBLEConnections) {
			sStatusLED.Blink(100, 100);
		} else {
			sStatusLED.Blink(50, 950);
		}

		sStatusLED.Animate();
		sUnusedLED.Animate();
		sUnusedLED_1.Animate();
		sUnusedLED_2.Animate();
	}
}

void AppTask::PostEvent(const AppEvent &event)
{
	if (k_msgq_put(&sAppEventQueue, &event, K_NO_WAIT)) {
		LOG_INF("Failed to post event to app task event queue");
	}
}

void AppTask::DispatchEvent(const AppEvent &event)
{
	switch (event.Type) {
	case AppEvent::FunctionPress:
		FunctionPressHandler();
		break;
	case AppEvent::FunctionRelease:
		FunctionReleaseHandler();
		break;
	case AppEvent::FunctionTimer:
		FunctionTimerEventHandler();
		break;
	default:
		LOG_INF("Unknown event received");
		break;
	}
}

void AppTask::FunctionPressHandler()
{
	sAppTask.StartFunctionTimer(kFactoryResetTriggerTimeout);
	sAppTask.mFunction = TimerFunction::FactoryReset;
}

void AppTask::FunctionReleaseHandler()
{
	if (sAppTask.mFunction == TimerFunction::FactoryReset) {
		sUnusedLED_2.Set(false);
		sUnusedLED_1.Set(false);
		sUnusedLED.Set(false);

		sAppTask.CancelFunctionTimer();
		sAppTask.mFunction = TimerFunction::NoneSelected;
		LOG_INF("Factory Reset has been Canceled");
	}
}

void AppTask::FunctionTimerEventHandler()
{
	if (sAppTask.mFunction == TimerFunction::FactoryReset) {
		sAppTask.mFunction = TimerFunction::NoneSelected;
		LOG_INF("Factory Reset triggered");
		
		sStatusLED.Set(true);
		sUnusedLED.Set(true);
		sUnusedLED_1.Set(true);
		sUnusedLED_2.Set(true);

		ConfigurationMgr().InitiateFactoryReset();
	}
}

void AppTask::ButtonEventHandler(uint32_t buttonState, uint32_t hasChanged)
{
	if (DK_BTN1_MSK & buttonState & hasChanged) {
		GetAppTask().PostEvent(AppEvent{ AppEvent::FunctionPress });
	} else if (DK_BTN1_MSK & hasChanged) {
		GetAppTask().PostEvent(AppEvent{ AppEvent::FunctionRelease });
	}
}

void AppTask::CancelFunctionTimer()
{
	k_timer_stop(&sFunctionTimer);
}

void AppTask::StartFunctionTimer(uint32_t timeoutInMs)
{
	k_timer_start(&sFunctionTimer, K_MSEC(timeoutInMs), K_NO_WAIT);
}

void AppTask::TimerEventHandler(k_timer *timer)
{
	GetAppTask().PostEvent(AppEvent{ AppEvent::FunctionTimer });
}
