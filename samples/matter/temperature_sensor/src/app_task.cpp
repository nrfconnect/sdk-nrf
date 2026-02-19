/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task.h"

#include "app/matter_init.h"
#include "app/task_executor.h"
#include "board/board.h"
#include "clusters/identify.h"
#include "lib/core/CHIPError.h"

#include <app-common/zap-generated/attributes/Accessors.h>

#include <zephyr/logging/log.h>

#ifdef CONFIG_BME680
#include <zephyr/drivers/sensor.h>

const device *sBme688SensorDev = DEVICE_DT_GET_ONE(bosch_bme680);
#endif

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::app;
using namespace ::chip::DeviceLayer;

namespace
{
constexpr chip::EndpointId kTemperatureSensorEndpointId = 1;

Nrf::Matter::IdentifyCluster sIdentifyCluster(kTemperatureSensorEndpointId);

#ifdef CONFIG_CHIP_ICD_UAT_SUPPORT
#ifndef CONFIG_NCS_SAMPLE_MATTER_USE_DEFAULT_BUTTON_HANDLER
#define UAT_BUTTON_MASK DK_BTN3_MSK
#endif
#endif

#ifndef CONFIG_NCS_SAMPLE_MATTER_USE_DEFAULT_BUTTON_HANDLER
constexpr int sSoftwareUpdateTimeout = 1500;
constexpr int sUatTimeout = 1500;
constexpr int sFactoryResetTimeout = 3000;
constexpr int sUatBlinkPeriod = 200;
ButtonState sBtnState = ButtonState::None;
k_timer sBtn1Timer;
#endif

} /* namespace */

void HandleUAT()
{
#ifdef CONFIG_CHIP_ICD_UAT_SUPPORT
	LOG_INF("ICD UserActiveMode has been triggered.");
	Server::GetInstance().GetICDManager().OnNetworkActivity();
#endif
}
/* nRF54T15 TAG has only a single button,
 * therefore the default handler for factory data and software update had to be overriden.
 * On other DK's only UAT is handled by the application.
 */
void AppTask::ButtonEventHandler(Nrf::ButtonState state, Nrf::ButtonMask hasChanged)
{
#ifdef CONFIG_NCS_SAMPLE_MATTER_USE_DEFAULT_BUTTON_HANDLER
	if (UAT_BUTTON_MASK & state & hasChanged) {
		HandleUAT();
	}
#else
	if (DK_BTN1_MSK & hasChanged) {
		if (DK_BTN1_MSK & state) {
			LOG_INF("Release the button within %ums to trigger Software Update", sSoftwareUpdateTimeout);
			k_timer_start(&sBtn1Timer, K_MSEC(sUatTimeout), K_NO_WAIT);
			sBtnState = ButtonState::SoftwareUpdate;
		} else {
			if (sBtnState == ButtonState::SoftwareUpdate) {
#ifndef CONFIG_NCS_SAMPLE_MATTER_CUSTOM_BLUETOOTH_ADVERTISING
				if (Nrf::GetBoard().GetDeviceState() == Nrf::DeviceState::DeviceProvisioned) {
/* In this case we need to run only Bluetooth LE SMP advertising if it is available */
#ifdef CONFIG_MCUMGR_TRANSPORT_BT
					GetDFUOverSMP().StartServer();
#else
					LOG_INF("Software update is disabled");
#endif
				} else {
					/* In this case we start both Bluetooth LE SMP and Matter advertising at the
					 * same time */
					Nrf::GetBoard().StartBLEAdvertisement();
				}
#endif
			} else if (sBtnState == ButtonState::UAT) {
				HandleUAT();
			}
			/* Restore LED's state and cancel the timer
			 */
			k_timer_stop(&sBtn1Timer);
			sBtnState = ButtonState::None;
			Nrf::GetBoard().RestoreAllLedsState();
			Nrf::GetBoard().RunLedStateHandler();
		}
	}
#endif
}
#ifndef CONFIG_NCS_SAMPLE_MATTER_USE_DEFAULT_BUTTON_HANDLER
void ButtonTimerEventHandler()
{
	if (sBtnState == ButtonState::SoftwareUpdate) {
		LOG_INF("Release the button within %ums to trigger UAT", sUatTimeout);
		k_timer_start(&sBtn1Timer, K_MSEC(sUatTimeout), K_NO_WAIT);
		sBtnState = ButtonState::UAT;
		/* Turn off all LEDs before starting blink to make sure blink is coordinated. */
		Nrf::GetBoard().ResetAllLeds();

		Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED1).Blink(sUatBlinkPeriod);
		Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).Blink(sUatBlinkPeriod);
#if NUMBER_OF_LEDS >= 3
		Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED3).Blink(sUatBlinkPeriod);
#endif
#if NUMBER_OF_LEDS >= 4
		Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED4).Blink(sUatBlinkPeriod);
#endif
	} else if (sBtnState == ButtonState::UAT) {
		LOG_INF("Factory reset has been triggered. Release button within %ums to cancel.",
			sFactoryResetTimeout);

		/* Start timer for sFactoryResetTimeout to allow user to cancel, if required. */
		k_timer_start(&sBtn1Timer, K_MSEC(sFactoryResetTimeout), K_NO_WAIT);
		sBtnState = ButtonState::None;

		Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED1).Blink(Nrf::LedConsts::kBlinkRate_ms);
		Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED2).Blink(Nrf::LedConsts::kBlinkRate_ms);
#if NUMBER_OF_LEDS >= 3
		Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED3).Blink(Nrf::LedConsts::kBlinkRate_ms);
#endif
#if NUMBER_OF_LEDS >= 4
		Nrf::GetBoard().GetLED(Nrf::DeviceLeds::LED4).Blink(Nrf::LedConsts::kBlinkRate_ms);
#endif
		/* If we reached here, the button was held past FactoryResetTriggerTimeout, initiate factory reset */
	} else if (sBtnState == ButtonState::None) {
		/* Actually trigger Factory Reset */
		chip::Server::GetInstance().ScheduleFactoryReset();
	}
}

void ButtonTimerTimeoutCallback(k_timer *timer)
{
	Nrf::PostTask([] { ButtonTimerEventHandler(); });
}

#endif

void AppTask::UpdateTemperatureMeasurement()
{
#ifdef CONFIG_BME680
	/* Real data from the onboard sensor*/
	int result = sensor_sample_fetch(sBme688SensorDev);
	if (result == 0) {
		sensor_value temperature;
		int result = sensor_channel_get(sBme688SensorDev, SENSOR_CHAN_AMBIENT_TEMP, &temperature);
		if (result == 0) {
			/* Defined by cluster temperature measured value = 100 x temperature in degC with resolution of
			 * 0.01 degC. val1 is an integer part of the value and val2 is fractional part in one-millionth
			 * parts. To achieve resolution of 0.01 degC val2 needs to be divided by 10000. */
			mCurrentTemperature = static_cast<int16_t>(temperature.val1 * 100 + temperature.val2 / 10000);
			LOG_DBG("New temperature measurement %d.%d *C", temperature.val1, temperature.val2);
		} else {
			LOG_ERR("Getting temperature measurement data from BME688 failed with: %d", result);
		}
	} else {
		LOG_ERR("Fetching data from BME688 sensor failed with: %d", result);
	}
#else
	/* Linear temperature increase that is wrapped around to min value after reaching the max value. */
	if (mCurrentTemperature < mTemperatureSensorMaxValue) {
		mCurrentTemperature += kTemperatureMeasurementStep;
	} else {
		mCurrentTemperature = mTemperatureSensorMinValue;
	}
#endif
}

void AppTask::UpdateTemperatureTimeoutCallback(k_timer *timer)
{
	if (!timer || !timer->user_data) {
		return;
	}

	DeviceLayer::PlatformMgr().ScheduleWork(
		[](intptr_t p) {
			AppTask::Instance().UpdateTemperatureMeasurement();

			Protocols::InteractionModel::Status status =
				Clusters::TemperatureMeasurement::Attributes::MeasuredValue::Set(
					kTemperatureSensorEndpointId, AppTask::Instance().GetCurrentTemperature());

			if (status != Protocols::InteractionModel::Status::Success) {
				LOG_ERR("Updating temperature measurement failed %x", to_underlying(status));
			}
		},
		reinterpret_cast<intptr_t>(timer->user_data));
}

CHIP_ERROR AppTask::Init()
{
	/* Initialize Matter stack */
	ReturnErrorOnFailure(Nrf::Matter::PrepareServer());

	if (!Nrf::GetBoard().Init(ButtonEventHandler)) {
		LOG_ERR("User interface initialization failed.");
		return CHIP_ERROR_INCORRECT_STATE;
	}

	/* Register Matter event handler that controls the connectivity status LED based on the captured Matter network
	 * state. */
	ReturnErrorOnFailure(Nrf::Matter::RegisterEventHandler(Nrf::Board::DefaultMatterEventHandler, 0));
#ifdef CONFIG_BME680
	if (!device_is_ready(sBme688SensorDev)) {
		LOG_ERR("BME688 sensor device not ready");
		return chip::System::MapErrorZephyr(-ENODEV);
	}
#endif

	ReturnErrorOnFailure(sIdentifyCluster.Init());

	return Nrf::Matter::StartServer();
}

CHIP_ERROR AppTask::StartApp()
{
	ReturnErrorOnFailure(Init());

	DataModel::Nullable<int16_t> val;
	Protocols::InteractionModel::Status status =
		Clusters::TemperatureMeasurement::Attributes::MinMeasuredValue::Get(kTemperatureSensorEndpointId, val);

	if (status != Protocols::InteractionModel::Status::Success || val.IsNull()) {
		LOG_ERR("Failed to get temperature measurement min value %x", to_underlying(status));
		return CHIP_ERROR_INCORRECT_STATE;
	}

	mTemperatureSensorMinValue = val.Value();

	status = Clusters::TemperatureMeasurement::Attributes::MaxMeasuredValue::Get(kTemperatureSensorEndpointId, val);

	if (status != Protocols::InteractionModel::Status::Success || val.IsNull()) {
		LOG_ERR("Failed to get temperature measurement max value %x", to_underlying(status));
		return CHIP_ERROR_INCORRECT_STATE;
	}

	mTemperatureSensorMaxValue = val.Value();

	k_timer_init(&mTimer, AppTask::UpdateTemperatureTimeoutCallback, nullptr);
	k_timer_user_data_set(&mTimer, this);
	k_timer_start(&mTimer, K_MSEC(kTemperatureMeasurementIntervalMs), K_MSEC(kTemperatureMeasurementIntervalMs));
#ifndef CONFIG_NCS_SAMPLE_MATTER_USE_DEFAULT_BUTTON_HANDLER
	k_timer_init(&sBtn1Timer, &ButtonTimerTimeoutCallback, nullptr);
#endif
	while (true) {
		Nrf::DispatchNextTask();
	}

	return CHIP_NO_ERROR;
}
