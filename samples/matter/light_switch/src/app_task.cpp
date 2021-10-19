/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task.h"

#include "led_widget.h"
#include "light_switch.h"
#include "thread_util.h"

#include <lib/support/CHIPMem.h>
#include <lib/support/SafeInt.h>
#include <platform/CHIPDeviceLayer.h>

#include <dk_buttons_and_leds.h>
#include <logging/log.h>
#include <zephyr.h>

using namespace ::chip::DeviceLayer;

LOG_MODULE_DECLARE(app);

namespace
{
static constexpr size_t kAppEventQueueSize = 10;

K_MSGQ_DEFINE(sAppEventQueue, sizeof(AppEvent), kAppEventQueueSize, alignof(AppEvent));
LightSwitch sLightSwitch;
LEDWidget sStatusLED;
uint8_t sLightLevel = 255; /* light brightness 0-255 */
k_timer sLightLevelTimer; /* timer for increasing the light brightness while holding the button */
k_timer sDiscoveryTimeout; /* timer for stopping light bulb discovery */
} /* namespace */

AppTask AppTask::sAppTask;

CHIP_ERROR AppTask::Init()
{
	/* Initialize CHIP */

	CHIP_ERROR err = chip::Platform::MemoryInit();

	if (err != CHIP_NO_ERROR) {
		LOG_ERR("Platform::MemoryInit() failed");
		return err;
	}

	err = sLightSwitch.Init();

	if (err != CHIP_NO_ERROR)
		return err;

	err = ThreadStackMgr().InitThreadStack();

	if (err != CHIP_NO_ERROR) {
		LOG_ERR("ThreadStackMgr().InitThreadStack() failed: %s", chip::ErrorStr(err));
		return err;
	}

	err = ConnectivityMgr().SetThreadDeviceType(ConnectivityManager::kThreadDeviceType_Router);

	if (err != CHIP_NO_ERROR) {
		LOG_ERR("ConnectivityMgr().SetThreadDeviceType() failed: %s", chip::ErrorStr(err));
		return err;
	}

	/* Initialize UI components */

	LEDWidget::InitGpio();
	LEDWidget::SetStateUpdateCallback(LEDStateUpdateHandler);
	sStatusLED.Init(DK_LED1);

	int ret = dk_buttons_init(ButtonEventHandler);

	if (ret) {
		LOG_ERR("dk_buttons_init() failed");
		return chip::System::MapErrorZephyr(ret);
	}

	k_timer_init(&sLightLevelTimer, AppTask::LightLevelTimerHandler, nullptr);
	k_timer_init(&sDiscoveryTimeout, AppTask::DiscoveryTimeoutHandler, nullptr);
	return CHIP_NO_ERROR;
}

CHIP_ERROR AppTask::StartApp()
{
	CHIP_ERROR err = Init();

	if (err != CHIP_NO_ERROR) {
		LOG_ERR("AppTask.Init() failed");
		return err;
	}

	AppEvent event = {};

	while (true) {
		k_msgq_get(&sAppEventQueue, &event, K_FOREVER);
		DispatchEvent(event);
	}

	return CHIP_NO_ERROR;
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
	case AppEvent::FactoryReset:
		LOG_INF("Factory reset triggered");
		ConfigurationMgr().InitiateFactoryReset();
		break;
	case AppEvent::StartDiscovery:
		StartDiscoveryHandler();
		break;
	case AppEvent::StopDiscovery:
		sLightSwitch.StopDiscovery();
		sStatusLED.Set(false);
		break;
	case AppEvent::ToggleLight:
		sLightSwitch.ToggleLight();
		break;
	case AppEvent::SetLevel:
		sLightSwitch.SetLightLevel(event.LightLevelEvent.Level);
		break;
	case AppEvent::LightFound:
		k_timer_stop(&sDiscoveryTimeout);
		sLightSwitch.StopDiscovery();
		sStatusLED.Set(sLightSwitch.Pair(event.LightFoundEvent.PeerAddress) == CHIP_NO_ERROR);
		break;
	case AppEvent::UpdateLedState:
		event.UpdateLedStateEvent.LedWidget->UpdateState();
		break;
	default:
		LOG_INF("Unknown event received");
		break;
	}
}

void AppTask::StartDiscoveryHandler()
{
	if (!ConnectivityMgr().IsThreadProvisioned()) {
		StartDefaultThreadNetwork(/* dataset timestamp */ 1);
		LOG_INF("Device is not commissioned to a Thread network. Starting with the default configuration");
	}

	sLightSwitch.StartDiscovery();
	sStatusLED.Blink(100, 900);
	k_timer_start(&sDiscoveryTimeout, K_SECONDS(10), K_NO_WAIT);
}

void AppTask::LEDStateUpdateHandler(LEDWidget &ledWidget)
{
	sAppTask.PostEvent(AppEvent{ AppEvent::UpdateLedState, &ledWidget });
}

void AppTask::ButtonEventHandler(uint32_t buttonsState, uint32_t hasChanged)
{
	if (DK_BTN1_MSK & buttonsState & hasChanged) {
		GetAppTask().PostEvent(AppEvent(AppEvent::FactoryReset));
	}

	if (DK_BTN2_MSK & buttonsState & hasChanged) {
		GetAppTask().PostEvent(AppEvent(AppEvent::ToggleLight));
	}

	if (DK_BTN3_MSK & buttonsState & hasChanged) {
		GetAppTask().PostEvent(AppEvent(AppEvent::StartDiscovery));
	}

	if (DK_BTN4_MSK & hasChanged & buttonsState) {
		k_timer_start(&sLightLevelTimer, K_MSEC(150), K_MSEC(150));
	} else if (DK_BTN4_MSK & hasChanged) {
		k_timer_stop(&sLightLevelTimer);
	}
}

void AppTask::LightLevelTimerHandler(k_timer *)
{
	/* The exponential formula below is used, since brightness differences become less and less
	 * visible for a human eye as the level increases. */
	const size_t newLevel = (sLightLevel + 1) * 5 / 4;
	sLightLevel = chip::CanCastTo<uint8_t>(newLevel) ? static_cast<uint8_t>(newLevel) : 0;
	GetAppTask().PostEvent(AppEvent(AppEvent::SetLevel, sLightLevel));
}

void AppTask::DiscoveryTimeoutHandler(k_timer *)
{
	GetAppTask().PostEvent(AppEvent(AppEvent::StopDiscovery));
}
