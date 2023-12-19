/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "board.h"

#include <platform/CHIPDeviceLayer.h>

class AppTask {
public:

	static AppTask &Instance()
	{
		static AppTask sAppTask;
		return sAppTask;
	};

	CHIP_ERROR StartApp();

private:
	CHIP_ERROR Init();

	static void MatterEventHandler(const chip::DeviceLayer::ChipDeviceEvent *event, intptr_t arg);
	static CHIP_ERROR RestoreBridgedDevices();
};
