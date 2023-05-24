/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "app_event.h"
#include "led_widget.h"

#include <app/clusters/identify-server/identify-server.h>
#include <platform/CHIPDeviceLayer.h>

#ifdef CONFIG_MCUMGR_TRANSPORT_BT
#include "dfu_over_smp.h"
#endif

#if CONFIG_CHIP_FACTORY_DATA
#include <platform/nrfconnect/FactoryDataProvider.h>
#endif

#ifdef CONFIG_CHIP_ICD_SUBSCRIPTION_HANDLING
#include "icd_util.h"
#endif

struct k_timer;

class AppTask {
public:
	CHIP_ERROR StartApp();

	void PostEvent(const AppEvent &aEvent);
	void UpdateClustersState();
	static void OnIdentifyStart(Identify *);
	static void OnIdentifyStop(Identify *);

private:
	friend AppTask &GetAppTask();

	CHIP_ERROR Init();

	void OpenPairingWindow();
	void DispatchEvent(AppEvent &event);
	void UpdateTemperatureClusterState();
	void UpdatePressureClusterState();
	void UpdateRelativeHumidityClusterState();
	void UpdatePowerSourceClusterState();

	static void ButtonStateHandler(uint32_t buttonState, uint32_t hasChanged);
	static void ButtonPushHandler();
	static void ButtonReleaseHandler();
	static void FunctionTimerHandler();
	static void MeasurementsTimerHandler();
	static void IdentifyTimerHandler();
	static void UpdateStatusLED();
	static void LEDStateUpdateHandler(LEDWidget &ledWidget);
	static void ChipEventHandler(const chip::DeviceLayer::ChipDeviceEvent *event, intptr_t arg);

	static AppTask sAppTask;

#if CONFIG_CHIP_FACTORY_DATA
	chip::DeviceLayer::FactoryDataProvider<chip::DeviceLayer::InternalFlashFactoryData> mFactoryDataProvider;
#endif
};

inline AppTask &GetAppTask()
{
	return AppTask::sAppTask;
}
