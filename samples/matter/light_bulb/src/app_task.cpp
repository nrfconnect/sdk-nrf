/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task.h"

#include "led_widget.h"
#include "light_bulb_publish_service.h"
#include "lighting_manager.h"
#include "thread_util.h"

#include <platform/CHIPDeviceLayer.h>

#include <app/common/gen/attribute-id.h>
#include <app/common/gen/attribute-type.h>
#include <app/common/gen/cluster-id.h>
#include <app/server/OnboardingCodesUtil.h>
#include <app/server/Server.h>
#include <app/util/attribute-storage.h>

#include <dk_buttons_and_leds.h>
#include <logging/log.h>
#include <zephyr.h>

#include <algorithm>

using namespace ::chip::DeviceLayer;

#define PWM_LED DT_ALIAS(pwm_led1)

LOG_MODULE_DECLARE(app);
K_MSGQ_DEFINE(sAppEventQueue, sizeof(AppEvent), AppTask::APP_EVENT_QUEUE_SIZE, alignof(AppEvent));

namespace
{
static constexpr uint32_t kFactoryResetTriggerTimeout = 3000;
static constexpr uint32_t kFactoryResetCancelWindow = 3000;

LEDWidget sStatusLED;
LEDWidget sLightLED;
LEDWidget sUnusedLED;
LEDWidget sUnusedLED_1;

LightBulbPublishService sLightBulbPublishService;

bool sIsThreadProvisioned;
bool sIsThreadEnabled;
bool sHaveBLEConnections;
bool sHaveServiceConnectivity;

k_timer sFunctionTimer;
} /* namespace */

AppTask AppTask::sAppTask;

int AppTask::Init()
{
	/* Initialize LEDs */
	LEDWidget::InitGpio();

	sStatusLED.Init(DK_LED1);
	sLightLED.Init(DK_LED2);
	sLightLED.Set(false);
	sUnusedLED.Init(DK_LED3);
	sUnusedLED_1.Init(DK_LED4);

	/* Initialize buttons */
	int ret = dk_buttons_init(ButtonEventHandler);
	if (ret) {
		LOG_ERR("dk_buttons_init() failed");
		return ret;
	}

#ifdef CONFIG_MCUMGR_SMP_BT
	GetDFUOverSMP().Init(RequestSMPAdvertisingStart);
	GetDFUOverSMP().ConfirmNewImage();
#endif

	/* Initialize function timer */
	k_timer_init(&sFunctionTimer, &AppTask::TimerEventHandler, nullptr);
	k_timer_user_data_set(&sFunctionTimer, this);

	/* Initialize light manager */
	ret = LightingMgr().Init(DEVICE_DT_GET(DT_PWMS_CTLR(PWM_LED)), DT_PWMS_CHANNEL(PWM_LED));
	if (ret) {
		return ret;
	}

	LightingMgr().SetCallbacks(ActionInitiated, ActionCompleted);

	/* Init ZCL Data Model and start server */
	InitServer();
	ConfigurationMgr().LogDeviceConfig();
	PrintOnboardingCodes(chip::RendezvousInformationFlags(chip::RendezvousInformationFlag::kBLE));

#if defined(CONFIG_CHIP_NFC_COMMISSIONING)
	PlatformMgr().AddEventHandler(AppTask::ChipEventHandler, 0);
#endif

	sLightBulbPublishService.Init();
	return 0;
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
		ret = k_msgq_get(&sAppEventQueue, &event, K_MSEC(10));

		while (!ret) {
			DispatchEvent(event);
			ret = k_msgq_get(&sAppEventQueue, &event, K_NO_WAIT);
		}

		/* Collect connectivity and configuration state from the CHIP stack.  Because the
		 * CHIP event loop is being run in a separate task, the stack must be locked
		 * while these values are queried.  However we use a non-blocking lock request
		 * (TryLockChipStack()) to avoid blocking other UI activities when the CHIP
		 * task is busy (e.g. with a long crypto operation). */

		if (PlatformMgr().TryLockChipStack()) {
			sIsThreadProvisioned = ConnectivityMgr().IsThreadProvisioned();
			sIsThreadEnabled = ConnectivityMgr().IsThreadEnabled();
			sHaveBLEConnections = (ConnectivityMgr().NumBLEConnections() != 0);
			sHaveServiceConnectivity = ConnectivityMgr().HaveServiceConnectivity();
			PlatformMgr().UnlockChipStack();
		}

		/* Update the status LED.
		 *
		 * If system has "full connectivity", keep the LED On constantly.
		 *
		 * If thread and service provisioned, but not attached to the thread network yet OR no
		 * connectivity to the service OR subscriptions are not fully established
		 * THEN blink the LED Off for a short period of time.
		 *
		 * If the system has ble connection(s) uptill the stage above, THEN blink the LED at an even
		 * rate of 100ms.
		 *
		 * Otherwise, blink the LED On for a very short time. */
		if (sHaveServiceConnectivity) {
			sStatusLED.Set(true);
		} else if (sIsThreadProvisioned && sIsThreadEnabled) {
			sStatusLED.Blink(950, 50);
		} else if (sHaveBLEConnections) {
			sStatusLED.Blink(100, 100);
		} else {
			sStatusLED.Blink(50, 950);
		}

		sStatusLED.Animate();
		sUnusedLED.Animate();
		sUnusedLED_1.Animate();
	}
}

void AppTask::PostEvent(const AppEvent &aEvent)
{
	if (k_msgq_put(&sAppEventQueue, &aEvent, K_NO_WAIT)) {
		LOG_INF("Failed to post event to app task event queue");
	}
}

void AppTask::UpdateClusterState()
{
	uint8_t onOff = LightingMgr().IsTurnedOn();

	/* write the new on/off value */
	EmberAfStatus status = emberAfWriteAttribute(1, ZCL_ON_OFF_CLUSTER_ID, ZCL_ON_OFF_ATTRIBUTE_ID,
						     CLUSTER_MASK_SERVER, &onOff, ZCL_BOOLEAN_ATTRIBUTE_TYPE);

	if (status != EMBER_ZCL_STATUS_SUCCESS) {
		LOG_ERR("Updating on/off cluster failed: %x", status);
	}

	uint8_t level = LightingMgr().GetLevel();

	status = emberAfWriteAttribute(1, ZCL_LEVEL_CONTROL_CLUSTER_ID, ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
				       CLUSTER_MASK_SERVER, &level, ZCL_INT8U_ATTRIBUTE_TYPE);

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
#ifdef CONFIG_MCUMGR_SMP_BT
	case AppEvent::StartSMPAdvertising:
		GetDFUOverSMP().StartBLEAdvertising();
		break;
#endif
	case AppEvent::PublishLightBulbService:
		sLightBulbPublishService.Publish();
		break;
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
		ConfigurationMgr().InitiateFactoryReset();
	}
}

void AppTask::StartThreadHandler()
{
	if (AddTestPairing() != CHIP_NO_ERROR) {
		LOG_ERR("Failed to add test pairing");
	}

	if (!ConnectivityMgr().IsThreadProvisioned()) {
		StartDefaultThreadNetwork();
		LOG_INF("Device is not commissioned to a Thread network. Starting with the default configuration");
	} else {
		LOG_INF("Device is commissioned to a Thread network");
	}

	sLightBulbPublishService.Start(10000, 1000);
}

void AppTask::StartBLEAdvertisingHandler()
{
	/* Don't allow on starting Matter service BLE advertising after Thread provisioning. */
	if (ConnectivityMgr().IsThreadProvisioned()) {
		LOG_INF("NFC Tag emulation and Matter service BLE advertisement not started - device is commissioned to a Thread network.");
		return;
	}

	if (ConnectivityMgr().IsBLEAdvertisingEnabled()) {
		LOG_INF("BLE Advertisement is already enabled");
		return;
	}

	if (OpenDefaultPairingWindow(chip::ResetFabrics::kNo) == CHIP_NO_ERROR) {
		LOG_INF("Enabled BLE Advertisement");
	} else {
		LOG_ERR("OpenDefaultPairingWindow() failed");
	}
}

#ifdef CONFIG_CHIP_NFC_COMMISSIONING
void AppTask::ChipEventHandler(const ChipDeviceEvent *event, intptr_t /* arg */)
{
	if (event->Type != DeviceEventType::kCHIPoBLEAdvertisingChange)
		return;

    if (event->CHIPoBLEAdvertisingChange.Result == kActivity_Started)
    {
        if (NFCMgr().IsTagEmulationStarted())
        {
            LOG_INF("NFC Tag emulation is already started");
        }
        else
        {
            ShareQRCodeOverNFC(chip::RendezvousInformationFlags(chip::RendezvousInformationFlag::kBLE));
        }
    }
    else if (event->CHIPoBLEAdvertisingChange.Result == kActivity_Stopped)
    {
        NFCMgr().StopTagEmulation();
    }
}
#endif

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
