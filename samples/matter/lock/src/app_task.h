/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "app_event.h"
#include "bolt_lock_manager.h"
#include "led_widget.h"

#include <platform/CHIPDeviceLayer.h>

#if CONFIG_CHIP_FACTORY_DATA
#include <platform/nrfconnect/FactoryDataProvider.h>
#else
#include <platform/nrfconnect/DeviceInstanceInfoProviderImpl.h>
#endif

#ifdef CONFIG_MCUMGR_TRANSPORT_BT
#include "dfu_over_smp.h"
#endif

#ifdef CONFIG_CHIP_ICD_SUBSCRIPTION_HANDLING
#include "icd_util.h"
#endif

struct k_timer;
struct Identify;
class AppFabricTableDelegate;

class AppTask {
public:
	friend class AppFabricTableDelegate;

	static AppTask &Instance()
	{
		static AppTask sAppTask;
		return sAppTask;
	};

	CHIP_ERROR StartApp();

	void UpdateClusterState(BoltLockManager::State state, BoltLockManager::OperationSource source);

	static void PostEvent(const AppEvent &event);
	static void IdentifyStartHandler(Identify *);
	static void IdentifyStopHandler(Identify *);

private:
	CHIP_ERROR Init();

	void CancelTimer();
	void StartTimer(uint32_t timeoutInMs);

	static void DispatchEvent(const AppEvent &event);
	static void FunctionTimerEventHandler(const AppEvent &event);
	static void FunctionHandler(const AppEvent &event);
	static void StartBLEAdvertisementAndLockActionEventHandler(const AppEvent &event);
	static void LockActionEventHandler(const AppEvent &event);
	static void StartBLEAdvertisementHandler(const AppEvent &event);
	static void UpdateLedStateEventHandler(const AppEvent &event);

	static void ChipEventHandler(const chip::DeviceLayer::ChipDeviceEvent *event, intptr_t arg);
	static void ButtonEventHandler(uint32_t buttonState, uint32_t hasChanged);
	static void LEDStateUpdateHandler(LEDWidget &ledWidget);
	static void FunctionTimerTimeoutCallback(k_timer *timer);
	static void UpdateStatusLED();

	static void LockStateChanged(BoltLockManager::State state, BoltLockManager::OperationSource source);

#ifdef CONFIG_THREAD_WIFI_SWITCHING
	static void SwitchImagesDone();
	static void SwitchImagesTriggerHandler(const AppEvent &event);
	static void SwitchImagesTimerTimeoutCallback(k_timer *timer);
	static void SwitchImagesEventHandler(const AppEvent &event);

	bool mSwitchImagesTimerActive = false;
#endif

#ifdef CONFIG_CHIP_NUS
	static void NUSLockCallback(void *context);
	static void NUSUnlockCallback(void *context);
#endif

#ifdef CONFIG_THREAD_WIFI_SWITCHING_CLI_SUPPORT
	static void RegisterSwitchCliCommand();
#endif

	FunctionEvent mFunction = FunctionEvent::NoneSelected;
	bool mFunctionTimerActive = false;

#if CONFIG_CHIP_FACTORY_DATA
	chip::DeviceLayer::FactoryDataProvider<chip::DeviceLayer::InternalFlashFactoryData> mFactoryDataProvider;
#endif
};
