/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "window_covering.h"

#include "board/led_widget.h"

#include <platform/CHIPDeviceLayer.h>

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

	static void MatterEventHandler(const chip::DeviceLayer::ChipDeviceEvent *event, intptr_t arg);
	static void ButtonEventHandler(Nrf::ButtonState state, Nrf::ButtonMask hasChanged);

	OperationalState mMoveType{ OperationalState::MovingUpOrOpen };
	bool mMovementTimerActive{ false };
	bool mOpenButtonIsPressed{ false };
	bool mCloseButtonIsPressed{ false };
	bool mMoveTypeRecentlyChanged{ false };
};
