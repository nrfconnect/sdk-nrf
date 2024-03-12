/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task.h"

#include "bolt_lock_manager.h"

#ifdef CONFIG_THREAD_WIFI_SWITCHING
#include "thread_wifi_switch.h"
#endif

#include "app/matter_init.h"
#include "app/task_executor.h"

#ifdef CONFIG_CHIP_NUS
#include "bt_nus/bt_nus_service.h"
#endif

#ifdef CONFIG_CHIP_OTA_REQUESTOR
#include "dfu/ota/ota_util.h"
#endif

#include <app-common/zap-generated/attributes/Accessors.h>
#include <app/clusters/door-lock-server/door-lock-server.h>
#include <app/clusters/identify-server/identify-server.h>
#include <app/server/OnboardingCodesUtil.h>
#include <platform/CHIPDeviceLayer.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::app;
using namespace ::chip::DeviceLayer;

namespace
{
constexpr EndpointId kLockEndpointId = 1;

#ifdef CONFIG_CHIP_NUS
constexpr uint16_t kAdvertisingIntervalMin = 400;
constexpr uint16_t kAdvertisingIntervalMax = 500;
constexpr uint8_t kLockNUSPriority = 2;
#endif

#ifdef CONFIG_THREAD_WIFI_SWITCHING
k_timer sSwitchTransportTimer;
constexpr uint32_t kSwitchTransportTimeout = 10000;
#endif

#define APPLICATION_BUTTON_MASK DK_BTN2_MSK
#define SWITCHING_BUTTON_MASK DK_BTN3_MSK
} /* namespace */

Identify sIdentify = { kLockEndpointId, AppTask::IdentifyStartHandler, AppTask::IdentifyStopHandler,
		       Clusters::Identify::IdentifyTypeEnum::kVisibleIndicator };

void AppTask::IdentifyStartHandler(Identify *)
{
	Nrf::PostTask(
		[] { Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).Blink(Nrf::LedConsts::kIdentifyBlinkRate_ms); });
}

void AppTask::IdentifyStopHandler(Identify *)
{
	Nrf::PostTask([] { Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).Set(BoltLockMgr().IsLocked()); });
}

void AppTask::ButtonEventHandler(Nrf::ButtonState state, Nrf::ButtonMask hasChanged)
{
	if ((APPLICATION_BUTTON_MASK & hasChanged) & state) {
		Nrf::PostTask([] { LockActionEventHandler(); });
	}

#ifdef CONFIG_THREAD_WIFI_SWITCHING
	if (SWITCHING_BUTTON_MASK & hasChanged) {
		SwitchButtonAction action =
			(SWITCHING_BUTTON_MASK & state) ? SwitchButtonAction::Pressed : SwitchButtonAction::Released;
		Nrf::PostTask([action] { SwitchTransportTriggerHandler(action); });
	}
#endif
}

void AppTask::LockActionEventHandler()
{
	if (BoltLockMgr().IsLocked()) {
		BoltLockMgr().Unlock(BoltLockManager::OperationSource::kButton);
	} else {
		BoltLockMgr().Lock(BoltLockManager::OperationSource::kButton);
	}
}

#ifdef CONFIG_THREAD_WIFI_SWITCHING
void AppTask::SwitchTransportEventHandler()
{
	LOG_INF("Switching to %s", ThreadWifiSwitch::IsThreadActive() ? "Wi-Fi" : "Thread");

	ThreadWifiSwitch::SwitchTransport();
}

void AppTask::SwitchTransportTimerTimeoutCallback(k_timer *timer)
{
	Nrf::PostTask([] { SwitchTransportEventHandler(); });
}

void AppTask::SwitchTransportTriggerHandler(const SwitchButtonAction &action)
{
	if (action == SwitchButtonAction::Pressed) {
		LOG_INF("Keep button pressed for %u ms to switch to %s", kSwitchTransportTimeout,
			ThreadWifiSwitch::IsThreadActive() ? "Wi-Fi" : "Thread");
		k_timer_start(&sSwitchTransportTimer, K_MSEC(kSwitchTransportTimeout), K_NO_WAIT);
	} else {
		LOG_INF("Switching to %s cancelled", ThreadWifiSwitch::IsThreadActive() ? "Wi-Fi" : "Thread");
		k_timer_stop(&sSwitchTransportTimer);
	}
}
#endif

void AppTask::LockStateChanged(BoltLockManager::State state, BoltLockManager::OperationSource source)
{
	switch (state) {
	case BoltLockManager::State::kLockingInitiated:
		LOG_INF("Lock action initiated");
		Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).Blink(50, 50);
#ifdef CONFIG_CHIP_NUS
		Nrf::GetNUSService().SendData("locking", sizeof("locking"));
#endif
		break;
	case BoltLockManager::State::kLockingCompleted:
		LOG_INF("Lock action completed");
		Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).Set(true);
#ifdef CONFIG_CHIP_NUS
		Nrf::GetNUSService().SendData("locked", sizeof("locked"));
#endif
		break;
	case BoltLockManager::State::kUnlockingInitiated:
		LOG_INF("Unlock action initiated");
		Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).Blink(50, 50);
#ifdef CONFIG_CHIP_NUS
		Nrf::GetNUSService().SendData("unlocking", sizeof("unlocking"));
#endif
		break;
	case BoltLockManager::State::kUnlockingCompleted:
		LOG_INF("Unlock action completed");
#ifdef CONFIG_CHIP_NUS
		Nrf::GetNUSService().SendData("unlocked", sizeof("unlocked"));
#endif
		Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).Set(false);
		break;
	}

	/* Handle changing attribute state in the application */
	Instance().UpdateClusterState(state, source);
}

void AppTask::UpdateClusterState(BoltLockManager::State state, BoltLockManager::OperationSource source)
{
	DlLockState newLockState;

	switch (state) {
	case BoltLockManager::State::kLockingCompleted:
		newLockState = DlLockState::kLocked;
		break;
	case BoltLockManager::State::kUnlockingCompleted:
		newLockState = DlLockState::kUnlocked;
		break;
	default:
		newLockState = DlLockState::kNotFullyLocked;
		break;
	}

	SystemLayer().ScheduleLambda([newLockState, source] {
		chip::app::DataModel::Nullable<chip::app::Clusters::DoorLock::DlLockState> currentLockState;
		chip::app::Clusters::DoorLock::Attributes::LockState::Get(kLockEndpointId, currentLockState);

		if (currentLockState.IsNull()) {
			/* Initialize lock state with start value, but not invoke lock/unlock. */
			chip::app::Clusters::DoorLock::Attributes::LockState::Set(kLockEndpointId, newLockState);
		} else {
			LOG_INF("Updating LockState attribute");

			if (!DoorLockServer::Instance().SetLockState(kLockEndpointId, newLockState, source)) {
				LOG_ERR("Failed to update LockState attribute");
			}
		}
	});
}

#ifdef CONFIG_CHIP_NUS
void AppTask::NUSLockCallback(void *context)
{
	LOG_DBG("Received LOCK command from NUS");
	if (BoltLockMgr().mState == BoltLockManager::State::kLockingCompleted ||
	    BoltLockMgr().mState == BoltLockManager::State::kLockingInitiated) {
		LOG_INF("Device is already locked");
	} else {
		Nrf::PostTask([] { LockActionEventHandler(); });
	}
}

void AppTask::NUSUnlockCallback(void *context)
{
	LOG_DBG("Received UNLOCK command from NUS");
	if (BoltLockMgr().mState == BoltLockManager::State::kUnlockingCompleted ||
	    BoltLockMgr().mState == BoltLockManager::State::kUnlockingInitiated) {
		LOG_INF("Device is already unlocked");
	} else {
		Nrf::PostTask([] { LockActionEventHandler(); });
	}
}
#endif

#ifdef CONFIG_NCS_SAMPLE_MATTER_TEST_EVENT_TRIGGERS
CHIP_ERROR AppTask::DoorLockJammedEventCallback(Nrf::Matter::TestEventTrigger::TriggerValue)
{
	VerifyOrReturnError(DoorLockServer::Instance().SendLockAlarmEvent(kLockEndpointId, AlarmCodeEnum::kLockJammed),
			    CHIP_ERROR_INTERNAL);
	LOG_ERR("Event Trigger: Doorlock jammed.");
	return CHIP_NO_ERROR;
}
#endif

CHIP_ERROR AppTask::Init()
{
	/* Initialize Matter stack */
#ifdef CONFIG_THREAD_WIFI_SWITCHING
	/* In the Thread/Wi-Fi switchable mode, it is the ThreadWifiSwitch module that owns the NetworkCommissioning
	   instances, so offload the initialization module from controlling that by explicitly setting the
	   mNetworkingInstance parameter to nullptr. Otherwise, the initialization module instantiates the
	   NetworkCommissioning object and handles it by default internally. */
	ReturnErrorOnFailure(Nrf::Matter::PrepareServer(Nrf::Matter::InitData{ .mNetworkingInstance = nullptr }));
#else
	ReturnErrorOnFailure(Nrf::Matter::PrepareServer());
#endif

	if (!Nrf::GetBoard().Init(ButtonEventHandler)) {
		LOG_ERR("User interface initialization failed.");
		return CHIP_ERROR_INCORRECT_STATE;
	}

	/* Register Matter event handler that controls the connectivity status LED based on the captured Matter network
	 * state. */
	ReturnErrorOnFailure(Nrf::Matter::RegisterEventHandler(Nrf::Board::DefaultMatterEventHandler, 0));

#ifdef CONFIG_THREAD_WIFI_SWITCHING
	CHIP_ERROR err = ThreadWifiSwitch::StartCurrentTransport();
	if (err != CHIP_NO_ERROR) {
		LOG_ERR("ThreadWifiSwitch::StartCurrentTransport() failed: %" CHIP_ERROR_FORMAT, err.Format());
		return err;
	}

	k_timer_init(&sSwitchTransportTimer, &AppTask::SwitchTransportTimerTimeoutCallback, nullptr);
#endif

#ifdef CONFIG_CHIP_NUS
	/* Initialize Nordic UART Service for Lock purposes */
	if (!Nrf::GetNUSService().Init(kLockNUSPriority, kAdvertisingIntervalMin, kAdvertisingIntervalMax)) {
		ChipLogError(Zcl, "Cannot initialize NUS service");
	}
	Nrf::GetNUSService().RegisterCommand("Lock", sizeof("Lock"), NUSLockCallback, nullptr);
	Nrf::GetNUSService().RegisterCommand("Unlock", sizeof("Unlock"), NUSUnlockCallback, nullptr);
	if (!Nrf::GetNUSService().StartServer()) {
		LOG_ERR("GetNUSService().StartServer() failed");
	}
#endif

	/* Initialize lock manager */
	BoltLockMgr().Init(LockStateChanged);

	/* Register Door Lock test event trigger */
#ifdef CONFIG_NCS_SAMPLE_MATTER_TEST_EVENT_TRIGGERS
	ReturnErrorOnFailure(Nrf::Matter::TestEventTrigger::Instance().RegisterTestEventTrigger(
		kDoorLockJammedEventTriggerId,
		Nrf::Matter::TestEventTrigger::EventTrigger{ 0, DoorLockJammedEventCallback }));
#endif

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
