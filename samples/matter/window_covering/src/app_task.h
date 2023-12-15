/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "led_widget.h"
#include "window_covering.h"

#include <platform/CHIPDeviceLayer.h>

#if CONFIG_CHIP_FACTORY_DATA
#include <platform/nrfconnect/FactoryDataProvider.h>
#else
#include <platform/nrfconnect/DeviceInstanceInfoProviderImpl.h>
#endif

#ifdef CONFIG_MCUMGR_TRANSPORT_BT
#include "dfu_over_smp.h"
#endif

struct Identify;

enum class WindowButtonAction : uint8_t { Pressed, Released };

class AppTask {
public:
	static AppTask &Instance()
	{
		static AppTask sAppTask;
		return sAppTask;
	};

	CHIP_ERROR StartApp();

	static void IdentifyStartHandler(Identify *);
	static void IdentifyStopHandler(Identify *);

private:
	CHIP_ERROR Init();
	void ToggleMoveType();

	static void OpenHandler(const WindowButtonAction &action);
	static void CloseHandler(const WindowButtonAction &action);

	static void ChipEventHandler(const chip::DeviceLayer::ChipDeviceEvent *event, intptr_t arg);
	static void ButtonEventHandler(ButtonState state, ButtonMask hasChanged);

	OperationalState mMoveType{ OperationalState::MovingUpOrOpen };
	bool mMovementTimerActive{ false };
	bool mOpenButtonIsPressed{ false };
	bool mCloseButtonIsPressed{ false };
	bool mMoveTypeRecentlyChanged{ false };

#if CONFIG_CHIP_FACTORY_DATA
	chip::DeviceLayer::FactoryDataProvider<chip::DeviceLayer::InternalFlashFactoryData> mFactoryDataProvider;
#endif
};
