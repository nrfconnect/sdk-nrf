/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task.h"

#include "temp_sensor_manager.h"
#include "temperature_manager.h"

#include "app/matter_init.h"
#include "app/task_executor.h"

#ifdef CONFIG_CHIP_OTA_REQUESTOR
#include "dfu/ota/ota_util.h"
#endif

#include <app-common/zap-generated/attributes/Accessors.h>
#include <app/clusters/identify-server/identify-server.h>
#include <app/server/OnboardingCodesUtil.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_MATTER_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::app;
using namespace ::chip::DeviceLayer;
namespace
{
constexpr EndpointId kThermostatEndpointId = 1;

Identify sIdentify = { kThermostatEndpointId, AppTask::IdentifyStartHandler, AppTask::IdentifyStopHandler,
		       Clusters::Identify::IdentifyTypeEnum::kVisibleIndicator };

#define TEMPERATURE_BUTTON_MASK DK_BTN2_MSK
} // namespace

void AppTask::IdentifyStartHandler(Identify *)
{
	Nrf::PostTask([] { Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).Blink(Nrf::LedConsts::kIdentifyBlinkRate_ms); });
}

void AppTask::IdentifyStopHandler(Identify *)
{
	Nrf::PostTask([] { Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).Set(false); });
}

void AppTask::ButtonEventHandler(Nrf::ButtonState state, Nrf::ButtonMask hasChanged)
{
	if (TEMPERATURE_BUTTON_MASK & hasChanged) {
		TemperatureButtonAction action = (TEMPERATURE_BUTTON_MASK & state) ? TemperatureButtonAction::Pushed :
										     TemperatureButtonAction::Released;
		Nrf::PostTask([action] { ThermostatHandler(action); });
	}
}

void AppTask::ThermostatHandler(const TemperatureButtonAction &action)
{
	if (action == TemperatureButtonAction::Pushed) {
		TemperatureManager::Instance().LogThermostatStatus();
	}
}

void AppTask::MatterEventHandler(const ChipDeviceEvent *event, intptr_t /* arg */)
{
	bool isNetworkProvisioned = false;

	switch (event->Type) {
	case DeviceEventType::kCHIPoBLEAdvertisingChange:
#ifdef CONFIG_CHIP_NFC_COMMISSIONING
		if (event->CHIPoBLEAdvertisingChange.Result == kActivity_Started) {
			if (NFCMgr().IsTagEmulationStarted()) {
				LOG_INF("NFC Tag emulation is already started");
			} else {
				ShareQRCodeOverNFC(
					chip::RendezvousInformationFlags(chip::RendezvousInformationFlag::kBLE));
			}
		} else if (event->CHIPoBLEAdvertisingChange.Result == kActivity_Stopped) {
			NFCMgr().StopTagEmulation();
		}
#endif
		if (ConnectivityMgr().NumBLEConnections() != 0) {
			Nrf::GetBoard().UpdateDeviceState(Nrf::DeviceState::DeviceConnectedBLE);
		}
		break;
#if defined(CONFIG_NET_L2_OPENTHREAD)
	case DeviceEventType::kDnssdInitialized:
#if CONFIG_CHIP_OTA_REQUESTOR
		Nrf::Matter::InitBasicOTARequestor();
#endif /* CONFIG_CHIP_OTA_REQUESTOR */
		break;
	case DeviceEventType::kThreadStateChange:
		isNetworkProvisioned = ConnectivityMgr().IsThreadProvisioned() && ConnectivityMgr().IsThreadEnabled();
#elif defined(CONFIG_CHIP_WIFI)
	case DeviceEventType::kWiFiConnectivityChange:
		isNetworkProvisioned =
			ConnectivityMgr().IsWiFiStationProvisioned() && ConnectivityMgr().IsWiFiStationEnabled();
#if CONFIG_CHIP_OTA_REQUESTOR
		if (event->WiFiConnectivityChange.Result == kConnectivity_Established) {
			Nrf::Matter::InitBasicOTARequestor();
		}
#endif /* CONFIG_CHIP_OTA_REQUESTOR */
#endif
		if (isNetworkProvisioned) {
			Nrf::GetBoard().UpdateDeviceState(Nrf::DeviceState::DeviceProvisioned);
		} else {
			Nrf::GetBoard().UpdateDeviceState(Nrf::DeviceState::DeviceDisconnected);
		}
		break;
	default:
		break;
	}
}

CHIP_ERROR AppTask::Init()
{
	/* Initialize Matter stack */
	ReturnErrorOnFailure(
		Nrf::Matter::PrepareServer(MatterEventHandler, Nrf::Matter::InitData{ .mPostServerInitClbk = [] {
						      CHIP_ERROR err = TemperatureManager::Instance().Init();
						      if (err != CHIP_NO_ERROR) {
							      LOG_ERR("TempMgr Init fail");
						      }
						      return err;
					      } }));

	if (!Nrf::GetBoard().Init(ButtonEventHandler)) {
		LOG_ERR("User interface initialization failed.");
		return CHIP_ERROR_INCORRECT_STATE;
	}

	CHIP_ERROR err = TempSensorManager::Instance().Init();
	if (err != CHIP_NO_ERROR) {
		LOG_ERR("TempSensorManager Init fail");
		return err;
	}
	return Nrf::Matter::StartServer();
}

CHIP_ERROR AppTask::StartApp()
{
	ReturnErrorOnFailure(Init());

	while (true) {
		Nrf::DispatchNextTask();
	}

	return CHIP_NO_ERROR;
}
