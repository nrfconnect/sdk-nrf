/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task.h"

#include "led_widget.h"
#include "lighting_manager.h"
#include "thread_util.h"

#include <platform/CHIPDeviceLayer.h>

#include <app-common/zap-generated/attribute-id.h>
#include <app-common/zap-generated/attribute-type.h>
#include <app-common/zap-generated/attributes/Accessors.h>
#include <app-common/zap-generated/cluster-id.h>
#include <app/server/OnboardingCodesUtil.h>
#include <app/server/Server.h>
#include <credentials/DeviceAttestationCredsProvider.h>
#include <credentials/examples/DeviceAttestationCredsExample.h>
#include <lib/support/CHIPMem.h>
#include <lib/support/CodeUtils.h>
#include <system/SystemError.h>

#ifdef CONFIG_CHIP_OTA_REQUESTOR
#include "ota_util.h"
#endif

#include <dk_buttons_and_leds.h>
#include <zephyr/logging/log.h>
#include <zephyr/zephyr.h>

#include <algorithm>

using namespace ::chip;
using namespace ::chip::app;
using namespace ::chip::Credentials;
using namespace ::chip::DeviceLayer;

static const struct pwm_dt_spec sPwmDevice = PWM_DT_SPEC_GET(DT_ALIAS(pwm_led1));

LOG_MODULE_DECLARE(app, CONFIG_MATTER_LOG_LEVEL);
K_MSGQ_DEFINE(sAppEventQueue, sizeof(AppEvent), AppTask::APP_EVENT_QUEUE_SIZE, alignof(AppEvent));

namespace
{
constexpr uint32_t kFactoryResetTriggerTimeout = 3000;
constexpr uint32_t kFactoryResetCancelWindow = 3000;
constexpr EndpointId kLightEndpointId = 1;
constexpr uint8_t kDefaultMinLevel = 0;
constexpr uint8_t kDefaultMaxLevel = 254;

LEDWidget sStatusLED;
LEDWidget sLightLED;
LEDWidget sUnusedLED;
LEDWidget sUnusedLED_1;

bool sIsThreadProvisioned;
bool sIsThreadEnabled;
bool sHaveBLEConnections;

k_timer sFunctionTimer;
} /* namespace */

AppTask AppTask::sAppTask;

CHIP_ERROR AppTask::Init()
{
	/* Initialize CHIP stack */
	LOG_INF("Init CHIP stack");

	CHIP_ERROR err = chip::Platform::MemoryInit();
	if (err != CHIP_NO_ERROR) {
		LOG_ERR("Platform::MemoryInit() failed");
		return err;
	}

	err = PlatformMgr().InitChipStack();
	if (err != CHIP_NO_ERROR) {
		LOG_ERR("PlatformMgr().InitChipStack() failed");
		return err;
	}

	err = ThreadStackMgr().InitThreadStack();
	if (err != CHIP_NO_ERROR) {
		LOG_ERR("ThreadStackMgr().InitThreadStack() failed");
		return err;
	}

#ifdef CONFIG_OPENTHREAD_MTD
	err = ConnectivityMgr().SetThreadDeviceType(ConnectivityManager::kThreadDeviceType_MinimalEndDevice);
#else
	err = ConnectivityMgr().SetThreadDeviceType(ConnectivityManager::kThreadDeviceType_Router);
#endif /* CONFIG_OPENTHREAD_MTD */
	if (err != CHIP_NO_ERROR) {
		LOG_ERR("ConnectivityMgr().SetThreadDeviceType() failed");
		return err;
	}

	/* Initialize LEDs */
	LEDWidget::InitGpio();
	LEDWidget::SetStateUpdateCallback(LEDStateUpdateHandler);

	sStatusLED.Init(DK_LED1);
	sLightLED.Init(DK_LED2);
	sLightLED.Set(false);
	sUnusedLED.Init(DK_LED3);
	sUnusedLED_1.Init(DK_LED4);

	UpdateStatusLED();

	/* Initialize buttons */
	int ret = dk_buttons_init(ButtonEventHandler);
	if (ret) {
		LOG_ERR("dk_buttons_init() failed");
		return chip::System::MapErrorZephyr(ret);
	}

#ifdef CONFIG_MCUMGR_SMP_BT
	/* Initialize DFU over SMP */
	GetDFUOverSMP().Init(RequestSMPAdvertisingStart);
	GetDFUOverSMP().ConfirmNewImage();
#endif

	/* Initialize function timer */
	k_timer_init(&sFunctionTimer, &AppTask::TimerEventHandler, nullptr);
	k_timer_user_data_set(&sFunctionTimer, this);

	/* Initialize light manager */
	uint8_t minLightLevel = kDefaultMinLevel;
	Clusters::LevelControl::Attributes::MinLevel::Get(kLightEndpointId, &minLightLevel);

	uint8_t maxLightLevel = kDefaultMaxLevel;
	Clusters::LevelControl::Attributes::MaxLevel::Get(kLightEndpointId, &maxLightLevel);

	ret = LightingMgr().Init(&sPwmDevice, minLightLevel, maxLightLevel);
	if (ret) {
		return chip::System::MapErrorZephyr(ret);
	}

	LightingMgr().SetCallbacks(ActionInitiated, ActionCompleted);

	/* Initialize CHIP server */
	SetDeviceAttestationCredentialsProvider(Examples::GetExampleDACProvider());

	static chip::CommonCaseDeviceServerInitParams initParams;
	(void)initParams.InitializeStaticResourcesBeforeServerInit();

	ReturnErrorOnFailure(chip::Server::GetInstance().Init(initParams));
	ConfigurationMgr().LogDeviceConfig();
	PrintOnboardingCodes(chip::RendezvousInformationFlags(chip::RendezvousInformationFlag::kBLE));

	/*
	 * Add CHIP event handler and start CHIP thread.
	 * Note that all the initialization code should happen prior to this point
	 * to avoid data races between the main and the CHIP threads.
	 */
	PlatformMgr().AddEventHandler(ChipEventHandler, 0);

	err = PlatformMgr().StartEventLoopTask();
	if (err != CHIP_NO_ERROR) {
		LOG_ERR("PlatformMgr().StartEventLoopTask() failed");
		return err;
	}

	return CHIP_NO_ERROR;
}

CHIP_ERROR AppTask::StartApp()
{
	ReturnErrorOnFailure(Init());

	AppEvent event = {};

	while (true) {
		k_msgq_get(&sAppEventQueue, &event, K_FOREVER);
		DispatchEvent(event);
	}

	return CHIP_NO_ERROR;
}

void AppTask::PostEvent(const AppEvent &aEvent)
{
	if (k_msgq_put(&sAppEventQueue, &aEvent, K_NO_WAIT)) {
		LOG_INF("Failed to post event to app task event queue");
	}
}

void AppTask::UpdateClusterState()
{
	/* write the new on/off value */

	EmberAfStatus status = Clusters::OnOff::Attributes::OnOff::Set(kLightEndpointId, LightingMgr().IsTurnedOn());

	if (status != EMBER_ZCL_STATUS_SUCCESS) {
		LOG_ERR("Updating on/off cluster failed: %x", status);
	}

	/* write the current level */

	status = Clusters::LevelControl::Attributes::CurrentLevel::Set(kLightEndpointId, LightingMgr().GetLevel());

	if (status != EMBER_ZCL_STATUS_SUCCESS) {
		LOG_ERR("Updating level cluster failed: %x", status);
	}
}

#ifdef CONFIG_MCUMGR_SMP_BT
void AppTask::RequestSMPAdvertisingStart(void)
{
	sAppTask.PostEvent(AppEvent{ AppEvent::StartSMPAdvertising });
}
#endif

void AppTask::DispatchEvent(const AppEvent &aEvent)
{
	switch (aEvent.Type) {
	case AppEvent::On:
		LightingMgr().InitiateAction(LightingManager::Action::On, aEvent.LightEvent.Value,
					     aEvent.LightEvent.ChipInitiated);
		break;
	case AppEvent::Off:
		LightingMgr().InitiateAction(LightingManager::Action::Off, aEvent.LightEvent.Value,
					     aEvent.LightEvent.ChipInitiated);
		break;
	case AppEvent::Toggle:
		LightingMgr().InitiateAction(LightingMgr().IsTurnedOn() ? LightingManager::Action::Off :
									  LightingManager::Action::On,
					     aEvent.LightEvent.Value, aEvent.LightEvent.ChipInitiated);
		break;
	case AppEvent::Level:
		LightingMgr().InitiateAction(LightingManager::Action::Level, aEvent.LightEvent.Value,
					     aEvent.LightEvent.ChipInitiated);
		break;
	case AppEvent::FunctionPress:
		FunctionPressHandler();
		break;
	case AppEvent::FunctionRelease:
		FunctionReleaseHandler();
		break;
	case AppEvent::FunctionTimer:
		FunctionTimerEventHandler();
		break;
	case AppEvent::StartThread:
		StartThreadHandler();
		break;
	case AppEvent::StartBleAdvertising:
		StartBLEAdvertisingHandler();
		break;
	case AppEvent::UpdateLedState:
		aEvent.UpdateLedStateEvent.LedWidget->UpdateState();
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

void AppTask::ActionInitiated(LightingManager::Action aAction)
{
	if (aAction == LightingManager::Action::On) {
		LOG_INF("Turn On Action has been initiated");
	} else if (aAction == LightingManager::Action::Off) {
		LOG_INF("Turn Off Action has been initiated");
	} else if (aAction == LightingManager::Action::Level) {
		LOG_INF("Level Action has been initiated");
	}
}

void AppTask::ActionCompleted(LightingManager::Action aAction)
{
	if (aAction == LightingManager::Action::On) {
		LOG_INF("Turn On Action has been completed");
	} else if (aAction == LightingManager::Action::Off) {
		LOG_INF("Turn Off Action has been completed");
	} else if (aAction == LightingManager::Action::Level) {
		LOG_INF("Level Action has been completed");
	}

	if (!LightingMgr().IsActionChipInitiated()) {
		sAppTask.UpdateClusterState();
	}
}

void AppTask::FunctionPressHandler()
{
	sAppTask.StartFunctionTimer(kFactoryResetTriggerTimeout);
	sAppTask.mFunction = TimerFunction::SoftwareUpdate;
}

void AppTask::FunctionReleaseHandler()
{
	if (sAppTask.mFunction == TimerFunction::SoftwareUpdate) {
		sAppTask.CancelFunctionTimer();
		sAppTask.mFunction = TimerFunction::NoneSelected;

#ifdef CONFIG_MCUMGR_SMP_BT
		GetDFUOverSMP().StartServer();
#else
		LOG_INF("Software update is disabled");
#endif

	} else if (sAppTask.mFunction == TimerFunction::FactoryReset) {
		sUnusedLED_1.Set(false);
		sUnusedLED.Set(false);

		UpdateStatusLED();

		sAppTask.CancelFunctionTimer();
		sAppTask.mFunction = TimerFunction::NoneSelected;
		LOG_INF("Factory Reset has been Canceled");
	}
}

void AppTask::FunctionTimerEventHandler()
{
	if (sAppTask.mFunction == TimerFunction::SoftwareUpdate) {
		LOG_INF("Factory Reset Triggered. Release button within %ums to cancel.", kFactoryResetCancelWindow);
		sAppTask.StartFunctionTimer(kFactoryResetCancelWindow);
		sAppTask.mFunction = TimerFunction::FactoryReset;

		/* Turn off all LEDs before starting blink to make sure blink is co-ordinated. */
		sStatusLED.Set(false);
		sUnusedLED_1.Set(false);
		sUnusedLED.Set(false);

		sStatusLED.Blink(500);
		sUnusedLED.Blink(500);
		sUnusedLED_1.Blink(500);

	} else if (sAppTask.mFunction == TimerFunction::FactoryReset) {
		sAppTask.mFunction = TimerFunction::NoneSelected;
		LOG_INF("Factory Reset triggered");
		chip::Server::GetInstance().ScheduleFactoryReset();
	}
}

void AppTask::StartThreadHandler()
{
	if (!ConnectivityMgr().IsThreadProvisioned()) {
		StartDefaultThreadNetwork();
		LOG_INF("Device is not commissioned to a Thread network. Starting with the default configuration.");
	} else {
		LOG_INF("Device is commissioned to a Thread network.");
	}
}

void AppTask::StartBLEAdvertisingHandler()
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

void AppTask::LEDStateUpdateHandler(LEDWidget &ledWidget)
{
	sAppTask.PostEvent(AppEvent{ AppEvent::UpdateLedState, &ledWidget });
}

void AppTask::UpdateStatusLED()
{
	/* Update the status LED.
	 *
	 * If thread and service provisioned, keep the LED On constantly.
	 *
	 * If the system has ble connection(s) uptill the stage above, THEN blink the LED at an even
	 * rate of 100ms.
	 *
	 * Otherwise, blink the LED On for a very short time. */
	if (sIsThreadProvisioned && sIsThreadEnabled) {
		sStatusLED.Set(true);
	} else if (sHaveBLEConnections) {
		sStatusLED.Blink(100, 100);
	} else {
		sStatusLED.Blink(50, 950);
	}
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
		sHaveBLEConnections = ConnectivityMgr().NumBLEConnections() != 0;
		UpdateStatusLED();
		break;
	case DeviceEventType::kThreadStateChange:
		sIsThreadProvisioned = ConnectivityMgr().IsThreadProvisioned();
		sIsThreadEnabled = ConnectivityMgr().IsThreadEnabled();
		UpdateStatusLED();
		break;
	case DeviceEventType::kThreadConnectivityChange:
#if CONFIG_CHIP_OTA_REQUESTOR
		if (event->ThreadConnectivityChange.Result == kConnectivity_Established) {
			InitBasicOTARequestor();
		}
#endif
		break;
	default:
		break;
	}
}

void AppTask::ButtonEventHandler(uint32_t buttonState, uint32_t hasChanged)
{
	if (DK_BTN1_MSK & buttonState & hasChanged) {
		GetAppTask().PostEvent(AppEvent{ AppEvent::FunctionPress });
	} else if (DK_BTN1_MSK & hasChanged) {
		GetAppTask().PostEvent(AppEvent{ AppEvent::FunctionRelease });
	}

	if (DK_BTN2_MSK & buttonState & hasChanged) {
		GetAppTask().PostEvent(AppEvent{ AppEvent::Toggle, 0, false });
	}

	if (DK_BTN3_MSK & buttonState & hasChanged) {
		GetAppTask().PostEvent(AppEvent{ AppEvent::StartThread });
	}

	if (DK_BTN4_MSK & buttonState & hasChanged) {
		GetAppTask().PostEvent(AppEvent{ AppEvent::StartBleAdvertising });
	}
}

void AppTask::CancelFunctionTimer()
{
	k_timer_stop(&sFunctionTimer);
}

void AppTask::StartFunctionTimer(uint32_t timeoutInMs)
{
	k_timer_start(&sFunctionTimer, K_MSEC(timeoutInMs), K_NO_WAIT);
}

void AppTask::TimerEventHandler(k_timer *timer)
{
	GetAppTask().PostEvent(AppEvent{ AppEvent::FunctionTimer });
}
