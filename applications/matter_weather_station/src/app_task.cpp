/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task.h"

#include "led_widget.h"
#include <platform/CHIPDeviceLayer.h>

#include <app-common/zap-generated/attribute-id.h>
#include <app-common/zap-generated/attribute-type.h>
#include <app-common/zap-generated/cluster-id.h>
#include <app/server/OnboardingCodesUtil.h>
#include <app/server/Server.h>
#include <app/util/attribute-storage.h>
#include <credentials/DeviceAttestationCredsProvider.h>
#include <credentials/examples/DeviceAttestationCredsExample.h>

#include <dk_buttons_and_leds.h>
#include <drivers/sensor.h>
#include <logging/log.h>
#include <zephyr.h>

using namespace ::chip::Credentials;
using namespace ::chip::DeviceLayer;

LOG_MODULE_DECLARE(app);

namespace
{
enum class FunctionTimerMode { kDisabled, kFactoryResetTrigger, kFactoryResetComplete };
enum class LedState { kAlive, kAdvertisingBle, kConnectedBle, kProvisioned };

constexpr size_t kAppEventQueueSize = 10;
constexpr size_t kFactoryResetTriggerTimeoutMs = 3000;
constexpr size_t kFactoryResetCompleteTimeoutMs = 3000;
constexpr size_t kMeasurementsIntervalMs = 3000;
constexpr uint8_t kTemperatureMeasurementEndpointId = 1;
constexpr int16_t kTemperatureMeasurementAttributeMaxValue = 0x7fff;
constexpr int16_t kTemperatureMeasurementAttributeMinValue = 0x954d;
constexpr int16_t kTemperatureMeasurementAttributeInvalidValue = 0x8000;
constexpr uint8_t kHumidityMeasurementEndpointId = 2;
constexpr uint16_t kHumidityMeasurementAttributeMaxValue = 0x2710;
constexpr uint16_t kHumidityMeasurementAttributeMinValue = 0;
constexpr uint16_t kHumidityMeasurementAttributeInvalidValue = 0xffff;
constexpr uint8_t kPressureMeasurementEndpointId = 3;
constexpr int16_t kPressureMeasurementAttributeMaxValue = 0x7fff;
constexpr int16_t kPressureMeasurementAttributeMinValue = 0x8001;
constexpr int16_t kPressureMeasurementAttributeInvalidValue = 0x8000;

K_MSGQ_DEFINE(sAppEventQueue, sizeof(AppEvent), kAppEventQueueSize, alignof(AppEvent));
k_timer sFunctionTimer;
k_timer sMeasurementsTimer;
FunctionTimerMode sFunctionTimerMode = FunctionTimerMode::kDisabled;

LEDWidget sRedLED;
LEDWidget sGreenLED;
LEDWidget sBlueLED;

bool sIsThreadProvisioned;
bool sIsThreadEnabled;
bool sIsBleAdvertisingEnabled;
bool sHaveBLEConnections;

LedState sLedState = LedState::kAlive;

const device *kBme688SensorDev = device_get_binding(DT_LABEL(DT_INST(0, bosch_bme680)));
} /* namespace */

AppTask AppTask::sAppTask;

int AppTask::Init()
{
	/* Initialize RGB LED */
	LEDWidget::InitGpio();
	LEDWidget::SetStateUpdateCallback(LEDStateUpdateHandler);

	sRedLED.Init(DK_LED1);
	sGreenLED.Init(DK_LED2);
	sBlueLED.Init(DK_LED3);

	UpdateStatusLED();

	/* Initialize buttons */
	int ret = dk_buttons_init(ButtonStateHandler);
	if (ret) {
		LOG_ERR("dk_buttons_init() failed");
		return ret;
	}

	if (!kBme688SensorDev) {
		LOG_ERR("BME688 sensor init failed");
		return -1;
	}

#ifdef CONFIG_MCUMGR_SMP_BT
	GetDFUOverSMP().Init(RequestSMPAdvertisingStart);
	GetDFUOverSMP().ConfirmNewImage();
	GetDFUOverSMP().StartServer();
#endif

	/* Initialize timers */
	k_timer_init(
		&sFunctionTimer, [](k_timer *) { sAppTask.PostEvent(AppEvent{ AppEvent::FunctionTimer }); }, nullptr);
	k_timer_init(
		&sMeasurementsTimer, [](k_timer *) { sAppTask.PostEvent(AppEvent{ AppEvent::MeasurementsTimer }); },
		nullptr);
	k_timer_start(&sMeasurementsTimer, K_MSEC(kMeasurementsIntervalMs), K_MSEC(kMeasurementsIntervalMs));

	/* Init ZCL Data Model and start server */
	chip::Server::GetInstance().Init();

	/* Initialize device attestation config */
	SetDeviceAttestationCredentialsProvider(Examples::GetExampleDACProvider());
	ConfigurationMgr().LogDeviceConfig();
	PrintOnboardingCodes(chip::RendezvousInformationFlags(chip::RendezvousInformationFlag::kBLE));

#if defined(CONFIG_CHIP_NFC_COMMISSIONING)
	PlatformMgr().AddEventHandler(AppTask::ChipEventHandler, 0);
#endif

	return 0;
}

void AppTask::OpenPairingWindow()
{
	/* Don't allow on starting Matter service BLE advertising after Thread provisioning. */
	if (ConnectivityMgr().IsThreadProvisioned()) {
		LOG_INF("NFC Tag emulation and Matter service BLE advertising not started - device is commissioned to a Thread network.");
		return;
	}

	if (ConnectivityMgr().IsBLEAdvertisingEnabled()) {
		LOG_INF("BLE advertising is already enabled");
		return;
	}

	if (chip::Server::GetInstance().GetCommissioningWindowManager().OpenBasicCommissioningWindow() !=
	    CHIP_NO_ERROR) {
		LOG_ERR("OpenBasicCommissioningWindow() failed");
	}
}

int AppTask::StartApp()
{
	int ret = Init();

	if (ret) {
		LOG_ERR("AppTask.Init() failed");
		return ret;
	}

	AppEvent event = {};

	while (true) {
		k_msgq_get(&sAppEventQueue, &event, K_FOREVER);
		DispatchEvent(event);
	}
}

void AppTask::PostEvent(const AppEvent &event)
{
	if (k_msgq_put(&sAppEventQueue, &event, K_NO_WAIT)) {
		LOG_ERR("Failed to post event to app task event queue");
	}
}

#ifdef CONFIG_MCUMGR_SMP_BT
void AppTask::RequestSMPAdvertisingStart(void)
{
	sAppTask.PostEvent(AppEvent{ AppEvent::StartSMPAdvertising });
}
#endif

void AppTask::DispatchEvent(AppEvent &event)
{
	switch (event.Type) {
	case AppEvent::FunctionPress:
		ButtonPushHandler();
		break;
	case AppEvent::FunctionRelease:
		ButtonReleaseHandler();
		break;
	case AppEvent::FunctionTimer:
		FunctionTimerHandler();
		break;
	case AppEvent::MeasurementsTimer:
		MeasurementsTimerHandler();
		break;
	case AppEvent::UpdateLedState:
		event.UpdateLedStateEvent.LedWidget->UpdateState();
		break;
#ifdef CONFIG_MCUMGR_SMP_BT
	case AppEvent::StartSMPAdvertising:
		GetDFUOverSMP().StartBLEAdvertising();
		break;
#endif
	default:
		LOG_INF("Unknown event received");
		break;
	}
}

void AppTask::ButtonPushHandler()
{
	sFunctionTimerMode = FunctionTimerMode::kFactoryResetTrigger;
	k_timer_start(&sFunctionTimer, K_MSEC(kFactoryResetTriggerTimeoutMs), K_NO_WAIT);
}

void AppTask::ButtonReleaseHandler()
{
	/* If the button was released within the first kFactoryResetTriggerTimeoutMs, open the BLE pairing
	 * window. */
	if (sFunctionTimerMode == FunctionTimerMode::kFactoryResetTrigger) {
		GetAppTask().OpenPairingWindow();
	}

	sFunctionTimerMode = FunctionTimerMode::kDisabled;
	k_timer_stop(&sFunctionTimer);
}

void AppTask::ButtonStateHandler(uint32_t buttonState, uint32_t hasChanged)
{
	if (hasChanged & DK_BTN1_MSK) {
		if (buttonState & DK_BTN1_MSK)
			sAppTask.PostEvent(AppEvent{ AppEvent::FunctionPress });
		else
			sAppTask.PostEvent(AppEvent{ AppEvent::FunctionRelease });
	}
}

void AppTask::FunctionTimerHandler()
{
	switch (sFunctionTimerMode) {
	case FunctionTimerMode::kFactoryResetTrigger:
		LOG_INF("Factory Reset triggered. Release button within %ums to cancel.",
			kFactoryResetCompleteTimeoutMs);
		sFunctionTimerMode = FunctionTimerMode::kFactoryResetComplete;
		k_timer_start(&sFunctionTimer, K_MSEC(kFactoryResetCompleteTimeoutMs), K_NO_WAIT);
		break;
	case FunctionTimerMode::kFactoryResetComplete:
		ConfigurationMgr().InitiateFactoryReset();
		break;
	default:
		break;
	}
}

void AppTask::MeasurementsTimerHandler()
{
	sAppTask.UpdateClusterState();
}

void AppTask::UpdateClusterState()
{
	struct sensor_value sTemperature, sPressure, sHumidity;
	int result = sensor_sample_fetch(kBme688SensorDev);
	EmberAfStatus status;

	if (result != 0) {
		LOG_ERR("Fetching data from BME688 sensor failed with: %d", result);
		return;
	}

	result = sensor_channel_get(kBme688SensorDev, SENSOR_CHAN_AMBIENT_TEMP, &sTemperature);
	if (result == 0) {
		/* Defined by cluster temperature measured value = 100 x temperature in degC with resolution of
		 * 0.01 degC. val1 is an integer part of the value and val2 is fractional part in one-millionth
		 * parts. To achieve resolution of 0.01 degC val2 needs to be divided by 10000. */
		int16_t newValue = static_cast<int16_t>(sTemperature.val1 * 100 + sTemperature.val2 / 10000);

		if (newValue > kTemperatureMeasurementAttributeMaxValue ||
		    newValue < kTemperatureMeasurementAttributeMinValue) {
			/* Read value exceeds permitted limits, so assign invalid value code to it. */
			newValue = kTemperatureMeasurementAttributeInvalidValue;
		}

		status = emberAfWriteAttribute(kTemperatureMeasurementEndpointId, ZCL_TEMP_MEASUREMENT_CLUSTER_ID,
					       ZCL_TEMP_MEASURED_VALUE_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
					       reinterpret_cast<uint8_t *>(&newValue), ZCL_INT16S_ATTRIBUTE_TYPE);

		if (status != EMBER_ZCL_STATUS_SUCCESS) {
			LOG_ERR("Updating temperature measurement %x", status);
		}
	} else {
		LOG_ERR("Getting temperature measurement data from BME688 failed with: %d", result);
	}

	result = sensor_channel_get(kBme688SensorDev, SENSOR_CHAN_PRESS, &sPressure);
	if (result == 0) {
		/* Defined by cluster pressure measured value = 10 x pressure in kPa with resolution of 0.1 kPa.
		 * val1 is an integer part of the value and val2 is fractional part in one-millionth parts.
		 * To achieve resolution of 0.1 kPa val2 needs to be divided by 100000. */
		int16_t newValue = static_cast<int16_t>(sPressure.val1 * 10 + sPressure.val2 / 100000);

		if (newValue > kPressureMeasurementAttributeMaxValue ||
		    newValue < kPressureMeasurementAttributeMinValue) {
			/* Read value exceeds permitted limits, so assign invalid value code to it. */
			newValue = kPressureMeasurementAttributeInvalidValue;
		}

		status = emberAfWriteAttribute(kPressureMeasurementEndpointId, ZCL_PRESSURE_MEASUREMENT_CLUSTER_ID,
					       ZCL_PRESSURE_MEASURED_VALUE_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
					       reinterpret_cast<uint8_t *>(&newValue), ZCL_INT16S_ATTRIBUTE_TYPE);

		if (status != EMBER_ZCL_STATUS_SUCCESS) {
			LOG_ERR("Updating pressure measurement %x", status);
		}
	} else {
		LOG_ERR("Getting pressure measurement data from BME688 failed with: %d", result);
	}

	result = sensor_channel_get(kBme688SensorDev, SENSOR_CHAN_HUMIDITY, &sHumidity);
	if (result == 0) {
		/* Defined by cluster humidity measured value = 100 x humidity in %RH with resolution of 0.01 %.
		 * val1 is an integer part of the value and val2 is fractional part in one-millionth parts.
		 * To achieve resolution of 0.01 % val2 needs to be divided by 10000. */
		uint16_t newValue = static_cast<int16_t>(sHumidity.val1 * 100 + sHumidity.val2 / 10000);

		if (newValue > kHumidityMeasurementAttributeMaxValue ||
		    newValue < kHumidityMeasurementAttributeMinValue) {
			/* Read value exceeds permitted limits, so assign invalid value code to it. */
			newValue = kHumidityMeasurementAttributeInvalidValue;
		}

		status = emberAfWriteAttribute(kHumidityMeasurementEndpointId,
					       ZCL_RELATIVE_HUMIDITY_MEASUREMENT_CLUSTER_ID,
					       ZCL_RELATIVE_HUMIDITY_MEASURED_VALUE_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
					       reinterpret_cast<uint8_t *>(&newValue), ZCL_INT16U_ATTRIBUTE_TYPE);

		if (status != EMBER_ZCL_STATUS_SUCCESS) {
			LOG_ERR("Updating relative humidity measurement %x", status);
		}
	} else {
		LOG_ERR("Getting humidity measurement data from BME688 failed with: %d", result);
	}
}

void AppTask::UpdateStatusLED()
{
	LedState nextState;

	if (sIsThreadProvisioned && sIsThreadEnabled) {
		nextState = LedState::kProvisioned;
	} else if (sHaveBLEConnections) {
		nextState = LedState::kConnectedBle;
	} else if (sIsBleAdvertisingEnabled) {
		nextState = LedState::kAdvertisingBle;
	} else {
		nextState = LedState::kAlive;
	}

	/* In case of changing signalled state, turn off all leds to synchronize blinking */
	if (nextState != sLedState) {
		sGreenLED.Set(false);
		sBlueLED.Set(false);
		sRedLED.Set(false);
	}
	sLedState = nextState;

	switch (sLedState) {
	case LedState::kAlive:
		sGreenLED.Blink(50, 950);
		break;
	case LedState::kAdvertisingBle:
		sBlueLED.Blink(50, 950);
		break;
	case LedState::kConnectedBle:
		sBlueLED.Blink(100, 100);
		break;
	case LedState::kProvisioned:
		sBlueLED.Blink(50, 950);
		sRedLED.Blink(50, 950);
		break;
	default:
		break;
	}
}

void AppTask::LEDStateUpdateHandler(LEDWidget &ledWidget)
{
	sAppTask.PostEvent(AppEvent{ AppEvent::UpdateLedState, &ledWidget });
}

void AppTask::ChipEventHandler(const ChipDeviceEvent *event, intptr_t /* arg */)
{
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
		sIsBleAdvertisingEnabled = ConnectivityMgr().IsBLEAdvertisingEnabled();
		sHaveBLEConnections = ConnectivityMgr().NumBLEConnections() != 0;
		UpdateStatusLED();
		break;
	case DeviceEventType::kThreadStateChange:
		sIsThreadProvisioned = ConnectivityMgr().IsThreadProvisioned();
		sIsThreadEnabled = ConnectivityMgr().IsThreadEnabled();
		UpdateStatusLED();
		break;
	default:
		break;
	}
}
