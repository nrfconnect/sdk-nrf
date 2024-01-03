/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "board/board.h"

#include <platform/CHIPDeviceLayer.h>

struct Identify;

enum class TemperatureButtonAction : uint8_t { Pushed, Released };

class AppTask {
public:
	static AppTask &Instance()
	{
		static AppTask sAppTask;
		return sAppTask;
	};

	CHIP_ERROR StartApp();

	static void IdentifyStartHandler(Identify *ident);
	static void IdentifyStopHandler(Identify *ident);

private:
	CHIP_ERROR Init();

	static void MatterEventHandler(const chip::DeviceLayer::ChipDeviceEvent *event, intptr_t arg);
	static void ButtonEventHandler(ButtonState state, ButtonMask hasChanged);
	static void ThermostatHandler(const TemperatureButtonAction &action);
};
