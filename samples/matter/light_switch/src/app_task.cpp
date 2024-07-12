/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task.h"

#include "light_switch.h"

#include "app/matter_init.h"
#include "app/task_executor.h"
#include "board/board.h"

#ifdef CONFIG_CHIP_OTA_REQUESTOR
#include "dfu/ota/ota_util.h"
#endif

#include <app/clusters/identify-server/identify-server.h>
#include <app/server/OnboardingCodesUtil.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::app;
using namespace ::chip::DeviceLayer;

namespace
{
constexpr uint32_t kDimmerTriggeredTimeout = 500;
constexpr uint32_t kDimmerInterval = 300;
constexpr EndpointId kLightSwitchEndpointId = 1;
constexpr EndpointId kLightEndpointId = 1;

k_timer sDimmerPressKeyTimer;
k_timer sDimmerTimer;

Identify sIdentify = { kLightEndpointId, AppTask::IdentifyStartHandler, AppTask::IdentifyStopHandler,
		       Clusters::Identify::IdentifyTypeEnum::kVisibleIndicator };
bool sWasDimmerTriggered = false;

#define APPLICATION_BUTTON_MASK DK_BTN2_MSK

#ifdef CONFIG_CHIP_ICD_UAT_SUPPORT
#define UAT_BUTTON_MASK DK_BTN3_MSK
#endif
} /* namespace */

void AppTask::DimmerTriggerEventHandler()
{
	if (!sWasDimmerTriggered) {
		LightSwitch::GetInstance().InitiateActionSwitch(LightSwitch::Action::Toggle);
	}

	Instance().CancelTimer(Timer::Dimmer);
	Instance().CancelTimer(Timer::DimmerTrigger);
	sWasDimmerTriggered = false;
}

void AppTask::TimerEventHandler(const Timer &timerType)
{
	switch (timerType) {
	case Timer::DimmerTrigger:
		LOG_INF("Dimming started...");
		sWasDimmerTriggered = true;
		LightSwitch::GetInstance().InitiateActionSwitch(LightSwitch::Action::On);
		Instance().StartTimer(Timer::Dimmer, kDimmerInterval);
		Instance().CancelTimer(Timer::DimmerTrigger);
		break;
	case Timer::Dimmer:
		LightSwitch::GetInstance().DimmerChangeBrightness();
		break;
	default:
		break;
	}
}

void AppTask::IdentifyStartHandler(Identify *)
{
	Nrf::PostTask(
		[] { Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).Blink(Nrf::LedConsts::kIdentifyBlinkRate_ms); });
}

void AppTask::IdentifyStopHandler(Identify *)
{
	Nrf::PostTask([] { Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).Set(false); });
}

void AppTask::ButtonEventHandler(Nrf::ButtonState state, Nrf::ButtonMask hasChanged)
{
	if ((APPLICATION_BUTTON_MASK & state & hasChanged)) {
		LOG_INF("Button has been pressed, keep in this state for at least 500 ms to change light sensitivity of bound lighting devices.");
		Instance().StartTimer(Timer::DimmerTrigger, kDimmerTriggeredTimeout);
	} else if ((APPLICATION_BUTTON_MASK & hasChanged)) {
		Nrf::PostTask([] { DimmerTriggerEventHandler(); });
#ifdef CONFIG_CHIP_ICD_UAT_SUPPORT
	} else if ((UAT_BUTTON_MASK & state & hasChanged)) {
		LOG_INF("ICD UserActiveMode has been triggered.");
		Server::GetInstance().GetICDManager().UpdateOperationState(ICDManager::OperationalState::ActiveMode);
#endif
	}
}

void AppTask::StartTimer(Timer timer, uint32_t timeoutMs)
{
	switch (timer) {
	case Timer::DimmerTrigger:
		k_timer_start(&sDimmerPressKeyTimer, K_MSEC(timeoutMs), K_NO_WAIT);
		break;
	case Timer::Dimmer:
		k_timer_start(&sDimmerTimer, K_MSEC(timeoutMs), K_MSEC(timeoutMs));
		break;
	default:
		break;
	}
}

void AppTask::CancelTimer(Timer timer)
{
	switch (timer) {
	case Timer::DimmerTrigger:
		k_timer_stop(&sDimmerPressKeyTimer);
		break;
	case Timer::Dimmer:
		k_timer_stop(&sDimmerTimer);
		break;
	default:
		break;
	}
}

void AppTask::UserTimerTimeoutCallback(k_timer *timer)
{
	if (!timer) {
		return;
	}
	Timer timerType;

	if (timer == &sDimmerPressKeyTimer) {
		timerType = Timer::DimmerTrigger;
	} else if (timer == &sDimmerTimer) {
		timerType = Timer::Dimmer;
	} else {
		return;
	}

	Nrf::PostTask([timerType]() { TimerEventHandler(timerType); });
}

CHIP_ERROR AppTask::Init()
{
	/* Initialize Matter stack */
	ReturnErrorOnFailure(Nrf::Matter::PrepareServer(Nrf::Matter::InitData{ .mPostServerInitClbk = [] {
		LightSwitch::GetInstance().Init(kLightSwitchEndpointId);
		return CHIP_NO_ERROR;
	} }));

	/* Initialize application timers */
	k_timer_init(&sDimmerPressKeyTimer, AppTask::UserTimerTimeoutCallback, nullptr);
	k_timer_init(&sDimmerTimer, AppTask::UserTimerTimeoutCallback, nullptr);

	if (!Nrf::GetBoard().Init(ButtonEventHandler)) {
		LOG_ERR("User interface initialization failed.");
		return CHIP_ERROR_INCORRECT_STATE;
	}

	/* Register Matter event handler that controls the connectivity status LED based on the captured Matter network
	 * state. */
	ReturnErrorOnFailure(Nrf::Matter::RegisterEventHandler(Nrf::Board::DefaultMatterEventHandler, 0));

	return Nrf::Matter::StartServer();
}

CHIP_ERROR AppTask::StartApp()
{
	ReturnErrorOnFailure(Init());

	while (true) {
		Nrf::DispatchNextTask();
	}

	return CHIP_NO_ERROR;
}
