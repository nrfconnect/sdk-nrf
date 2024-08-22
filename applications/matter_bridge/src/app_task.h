/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "board/board.h"
#include <platform/CHIPDeviceLayer.h>

class AppTask {
public:
	static AppTask &Instance()
	{
		static AppTask sAppTask;
		return sAppTask;
	};

	CHIP_ERROR StartApp();
#ifdef CONFIG_BRIDGE_SMART_PLUG_SUPPORT
	static constexpr chip::EndpointId kSmartplugEndpointId = 2;
	static void ButtonEventHandler(Nrf::ButtonState state, Nrf::ButtonMask hasChanged);
	static void SmartplugOnOffEventHandler();
#endif /* CONFIG_BRIDGE_SMART_PLUG_SUPPORT */

private:
	CHIP_ERROR Init();

	static CHIP_ERROR RestoreBridgedDevices();
};
