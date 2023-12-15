/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "board.h"
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

struct k_timer;
struct Identify;

#ifdef CONFIG_THREAD_WIFI_SWITCHING
enum class SwitchButtonAction : uint8_t { Pressed, Released };
#endif

class AppTask {
public:
	static AppTask &Instance()
	{
		static AppTask sAppTask;
		return sAppTask;
	};

	CHIP_ERROR StartApp();

	void UpdateClusterState(BoltLockManager::State state, BoltLockManager::OperationSource source);
	static void IdentifyStartHandler(Identify *);
	static void IdentifyStopHandler(Identify *);

private:
	CHIP_ERROR Init();

	static void LockActionEventHandler();
	static void ButtonEventHandler(ButtonState state, ButtonMask hasChanged);
	static void ChipEventHandler(const chip::DeviceLayer::ChipDeviceEvent *event, intptr_t arg);
	static void LockStateChanged(BoltLockManager::State state, BoltLockManager::OperationSource source);

#ifdef CONFIG_THREAD_WIFI_SWITCHING
	static void SwitchTransportEventHandler();
	static void SwitchTransportTimerTimeoutCallback(k_timer *timer);
	static void SwitchTransportTriggerHandler(const SwitchButtonAction &action);
#endif

#ifdef CONFIG_CHIP_NUS
	static void NUSLockCallback(void *context);
	static void NUSUnlockCallback(void *context);
#endif

#if CONFIG_CHIP_FACTORY_DATA
	chip::DeviceLayer::FactoryDataProvider<chip::DeviceLayer::InternalFlashFactoryData> mFactoryDataProvider;
#endif
};
