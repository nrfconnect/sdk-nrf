/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "board/board.h"

#include <platform/CHIPDeviceLayer.h>

struct Identify;

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

	static constexpr chip::EndpointId kContactSensorEndpointId = 1;

private:
	CHIP_ERROR Init();

	static void ButtonEventHandler(Nrf::ButtonState state, Nrf::ButtonMask hasChanged);
};
