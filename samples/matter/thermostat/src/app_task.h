/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "board.h"

#include <platform/CHIPDeviceLayer.h>

#include <app/clusters/identify-server/identify-server.h>
#include <platform/nrfconnect/DeviceInstanceInfoProviderImpl.h>

#if CONFIG_CHIP_FACTORY_DATA
#include <platform/nrfconnect/FactoryDataProvider.h>
#else
#include <platform/nrfconnect/DeviceInstanceInfoProviderImpl.h>
#endif

#ifdef CONFIG_MCUMGR_TRANSPORT_BT
#include "dfu_over_smp.h"
#endif

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

	static void ChipEventHandler(const chip::DeviceLayer::ChipDeviceEvent *event, intptr_t arg);
	static void ButtonEventHandler(ButtonState state, ButtonMask hasChanged);
	static void ThermostatHandler(const TemperatureButtonAction &action);

#if CONFIG_CHIP_FACTORY_DATA
	chip::DeviceLayer::FactoryDataProvider<chip::DeviceLayer::InternalFlashFactoryData> mFactoryDataProvider;
#endif
};
