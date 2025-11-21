/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task.h"
#include "bolt_lock_manager.h"
#include "clusters/identify.h"

#ifdef CONFIG_THREAD_WIFI_SWITCHING
#include "thread_wifi_switch.h"
#endif

#include "app/matter_init.h"
#include "app/task_executor.h"

#ifdef CONFIG_CHIP_NUS
#include "bt_nus/bt_nus_service.h"
#endif

#include <app-common/zap-generated/attributes/Accessors.h>
#include <app/clusters/door-lock-server/door-lock-server.h>
#include <platform/CHIPDeviceLayer.h>
#include <setup_payload/OnboardingCodesUtil.h>

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

#ifndef CONFIG_CHIP_FACTORY_RESET_ERASE_SETTINGS
void AppFactoryResetHandler(const ChipDeviceEvent *event, intptr_t /* unused */)
{
	switch (event->Type) {
	case DeviceEventType::kFactoryReset:
		BoltLockMgr().FactoryReset();
		break;
	default:
		break;
	}
}
#endif

Nrf::Matter::IdentifyCluster sIdentifyCluster(kLockEndpointId, false, []() {
	Nrf::PostTask([] { Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).Set(BoltLockMgr().IsLocked()); });
});
} /* namespace */

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

void AppTask::LockStateChanged(const BoltLockManager::StateData &stateData)
{
	switch (stateData.mState) {
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
	Instance().UpdateClusterState(stateData);
}

void AppTask::UpdateClusterState(const BoltLockManager::StateData &stateData)
{
	BoltLockManager::StateData *stateDataCopy = Platform::New<BoltLockManager::StateData>(stateData);

	if (stateDataCopy == nullptr) {
		LOG_ERR("Failed to allocate memory for BoltLockManager::StateData");
		return;
	}

	CHIP_ERROR err = SystemLayer().ScheduleLambda([stateDataCopy]() {
		UpdateClusterStateHandler(*stateDataCopy);
		Platform::Delete(stateDataCopy);
	});

	if (err != CHIP_NO_ERROR) {
		LOG_ERR("Failed to schedule lambda: %" CHIP_ERROR_FORMAT, err.Format());
		Platform::Delete(stateDataCopy);
	}
}

void AppTask::UpdateClusterStateHandler(const BoltLockManager::StateData &stateData)
{
	using namespace chip::app::Clusters::DoorLock::Attributes;

	DlLockState newLockState;

	switch (stateData.mState) {
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

	Nullable<DlLockState> currentLockState;
	LockState::Get(kLockEndpointId, currentLockState);

	if (currentLockState.IsNull()) {
		/* Initialize lock state with start value, but not invoke lock/unlock. */
		LockState::Set(kLockEndpointId, newLockState);
	} else {
		LOG_INF("Updating LockState attribute");

		Nullable<uint16_t> userId;
		Nullable<List<const LockOpCredentials>> credentials;
#ifdef CONFIG_LOCK_PASS_CREDENTIALS_TO_SET_LOCK_STATE
		List<const LockOpCredentials> credentialList;
#endif

		if (!stateData.mValidatePINResult.IsNull()) {
			userId = { stateData.mValidatePINResult.Value().mUserId };

#ifdef CONFIG_LOCK_PASS_CREDENTIALS_TO_SET_LOCK_STATE
			/* `DoorLockServer::SetLockState` exptects list of `LockOpCredentials`,
			   however in case of PIN validation it makes no sense to have more than one
			   credential corresponding to validation result. For simplicity we wrap single
			   credential in list here. */
			credentialList = { &stateData.mValidatePINResult.Value().mCredential, 1 };
			credentials = { credentialList };
#endif
		}

		if (!DoorLockServer::Instance().SetLockState(kLockEndpointId, newLockState, stateData.mSource, userId,
							     credentials, stateData.mFabricIdx, stateData.mNodeId)) {
			LOG_ERR("Failed to update LockState attribute");
		}
	}
}

#ifdef CONFIG_CHIP_NUS
void AppTask::NUSLockCallback(void *context)
{
	LOG_DBG("Received LOCK command from NUS");
	if (BoltLockMgr().GetState().mState == BoltLockManager::State::kLockingCompleted ||
	    BoltLockMgr().GetState().mState == BoltLockManager::State::kLockingInitiated) {
		LOG_INF("Device is already locked");
	} else {
		Nrf::PostTask([] { LockActionEventHandler(); });
	}
}

void AppTask::NUSUnlockCallback(void *context)
{
	LOG_DBG("Received UNLOCK command from NUS");
	if (BoltLockMgr().GetState().mState == BoltLockManager::State::kUnlockingCompleted ||
	    BoltLockMgr().GetState().mState == BoltLockManager::State::kUnlockingInitiated) {
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

#ifndef CONFIG_CHIP_FACTORY_RESET_ERASE_SETTINGS
	/* Register factory reset event handler.
	 * With this configuration we have to manually clean up the storage,
	 * as whole settings partition won't be erased.
	 * */
	ReturnErrorOnFailure(Nrf::Matter::RegisterEventHandler(AppFactoryResetHandler, 0));
#endif

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

	ReturnErrorOnFailure(sIdentifyCluster.Init());

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
