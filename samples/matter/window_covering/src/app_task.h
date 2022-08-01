/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "window_covering.h"
#include <platform/CHIPDeviceLayer.h>

#if CONFIG_CHIP_FACTORY_DATA
#include <platform/nrfconnect/FactoryDataProvider.h>
#endif

#ifdef CONFIG_MCUMGR_SMP_BT
#include "dfu_over_smp.h"
#endif

struct k_timer;
class AppEvent;
class LEDWidget;

class AppTask {
public:
	static AppTask &Instance(void)
	{
		static AppTask sAppTask;
		return sAppTask;
	};
	CHIP_ERROR StartApp();

private:
	enum class OperatingMode : uint8_t { Normal, FactoryReset, FirmwareUpdate, MoveSelection, Movement, Invalid };
	CHIP_ERROR Init();
	void DispatchEvent(AppEvent *aEvent);
	void ToggleMoveType();

	/* statics needed to interact with zephyr C API */
	static void CancelTimer();
	static void StartTimer(uint32_t aTimeoutInMs);
	static void FunctionTimerEventHandler(AppEvent *aEvent);
	static void MovementTimerEventHandler(AppEvent *aEvent);
	static void FunctionHandler(AppEvent *aEvent);
	static void ButtonEventHandler(uint32_t aButtonsState, uint32_t aHasChanged);
	static void TimerTimeoutCallback(k_timer *aTimer);
	static void FunctionTimerTimeoutCallback(k_timer *aTimer);
	static void PostEvent(AppEvent *aEvent);
	static void UpdateStatusLED();
	static void LEDStateUpdateHandler(LEDWidget &aLedWidget);
	static void UpdateLedStateEventHandler(AppEvent *aEvent);
	static void StartBLEAdvertisementHandler(AppEvent *aEvent);
	static void ChipEventHandler(const chip::DeviceLayer::ChipDeviceEvent *aEvent, intptr_t aArg);
	static void OpenHandler(AppEvent *aEvent);
	static void CloseHandler(AppEvent *aEvent);
#ifdef CONFIG_MCUMGR_SMP_BT
	static void RequestSMPAdvertisingStart(void);
#endif

	OperatingMode mMode{ OperatingMode::Normal };
	OperationalState mMoveType{ OperationalState::MovingUpOrOpen };
	bool mFunctionTimerActive{ false };
	bool mMovementTimerActive{ false };
	bool mIsThreadProvisioned{ false };
	bool mIsThreadEnabled{ false };
	bool mHaveBLEConnections{ false };
	bool mOpenButtonIsPressed{ false };
	bool mCloseButtonIsPressed{ false };
	bool mMoveTypeRecentlyChanged{ false };

#if CONFIG_CHIP_FACTORY_DATA
	chip::DeviceLayer::FactoryDataProvider<chip::DeviceLayer::InternalFlashFactoryData> mFactoryDataProvider;
#endif
};
