/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task.h"
#include "app_config.h"
#include "app_event.h"
#include "led_widget.h"
#include "window_covering.h"
#include "thread_util.h"

#include <app/server/OnboardingCodesUtil.h>
#include <app/server/Server.h>

#include <app-common/zap-generated/attribute-id.h>
#include <app-common/zap-generated/attribute-type.h>
#include <app-common/zap-generated/cluster-id.h>
#include <app/util/attribute-storage.h>

#include <credentials/DeviceAttestationCredsProvider.h>
#include <credentials/examples/DeviceAttestationCredsExample.h>

#include <dk_buttons_and_leds.h>
#include <logging/log.h>
#include <zephyr.h>

#define FACTORY_RESET_TRIGGER_TIMEOUT 3000
#define FACTORY_RESET_CANCEL_WINDOW_TIMEOUT 3000
#define MOVEMENT_START_WINDOW_TIMEOUT 2000
#define APP_EVENT_QUEUE_SIZE 10
#define BUTTON_PUSH_EVENT 1
#define BUTTON_RELEASE_EVENT 0

LOG_MODULE_DECLARE(app, CONFIG_MATTER_LOG_LEVEL);
K_MSGQ_DEFINE(sAppEventQueue, sizeof(AppEvent), APP_EVENT_QUEUE_SIZE, alignof(AppEvent));

#ifdef CONFIG_CHIP_OTA_REQUESTOR
#include "ota_util.h"
#endif

static LEDWidget sStatusLED;
static LEDWidget sUnusedLED;

static k_timer sFunctionTimer;

namespace LedConsts
{
constexpr uint32_t kBlinkRate_ms{ 500 };
namespace StatusLed
{
	namespace Unprovisioned
	{
		constexpr uint32_t kOn_ms{ 100 };
		constexpr uint32_t kOff_ms{ kOn_ms };
	} /* namespace Unprovisioned */
	namespace Provisioned
	{
		constexpr uint32_t kOn_ms{ 50 };
		constexpr uint32_t kOff_ms{ 950 };
	} /* namespace Provisioned */

} /* namespace StatusLed */
} /* namespace LedConsts */

using namespace ::chip;
using namespace ::chip::Credentials;
using namespace ::chip::DeviceLayer;

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

#if CONFIG_CHIP_THREAD_SSED
	err = ConnectivityMgr().SetThreadDeviceType(ConnectivityManager::kThreadDeviceType_SynchronizedSleepyEndDevice);
#elif CONFIG_OPENTHREAD_MTD_SED
	err = ConnectivityMgr().SetThreadDeviceType(ConnectivityManager::kThreadDeviceType_SleepyEndDevice);
#else
	err = ConnectivityMgr().SetThreadDeviceType(ConnectivityManager::kThreadDeviceType_MinimalEndDevice);
#endif
	if (err != CHIP_NO_ERROR) {
		LOG_ERR("ConnectivityMgr().SetThreadDeviceType() failed");
		return err;
	}

#ifdef CONFIG_OPENTHREAD_DEFAULT_TX_POWER
	err = SetDefaultThreadOutputPower();
	if (err != CHIP_NO_ERROR) {
		LOG_ERR("Cannot set default Thread output power");
		return err;
	}
#endif

	/* Initialize LEDs */
	LEDWidget::InitGpio();
	LEDWidget::SetStateUpdateCallback(LEDStateUpdateHandler);

	sStatusLED.Init(SYSTEM_STATE_LED);
	sUnusedLED.Init(DK_LED4);

	UpdateStatusLED();

	/* Initialize buttons */
	auto ret = dk_buttons_init(ButtonEventHandler);
	if (ret) {
		LOG_ERR("dk_buttons_init() failed");
		return chip::System::MapErrorZephyr(ret);
	}

#ifdef CONFIG_MCUMGR_SMP_BT
	/* Initialize DFU over SMP */
	GetDFUOverSMP().Init(RequestSMPAdvertisingStart);
	GetDFUOverSMP().ConfirmNewImage();
#endif

	/* Initialize function timer user data */
	k_timer_init(&sFunctionTimer, &AppTask::FunctionTimerTimeoutCallback, nullptr);
	k_timer_user_data_set(&sFunctionTimer, this);

	/* Initialize CHIP server */
	SetDeviceAttestationCredentialsProvider(Examples::GetExampleDACProvider());
	static chip::CommonCaseDeviceServerInitParams initParams;
	(void)initParams.InitializeStaticResourcesBeforeServerInit();
	ReturnErrorOnFailure(chip::Server::GetInstance().Init(initParams));

#if CONFIG_CHIP_OTA_REQUESTOR
	InitBasicOTARequestor();
#endif

	ConfigurationMgr().LogDeviceConfig();
	PrintOnboardingCodes(chip::RendezvousInformationFlag(chip::RendezvousInformationFlag::kBLE));

	/* Add CHIP event handler and start CHIP thread. */
	/* Note that all the initialization code should happen prior to this point to avoid data races */
	/* between the main and the CHIP threads */
	PlatformMgr().AddEventHandler(ChipEventHandler, 0);
	err = PlatformMgr().StartEventLoopTask();
	if (err != CHIP_NO_ERROR) {
		LOG_ERR("PlatformMgr().StartEventLoopTask() failed");
		return err;
	}

	WindowCovering::Instance().PositionLEDUpdate(WindowCovering::MoveType::LIFT);
	WindowCovering::Instance().PositionLEDUpdate(WindowCovering::MoveType::TILT);

	return err;
}

CHIP_ERROR AppTask::StartApp()
{
	ReturnErrorOnFailure(Init());

	AppEvent event{};

	while (true) {
		k_msgq_get(&sAppEventQueue, &event, K_FOREVER);
		DispatchEvent(&event);
	}

	return CHIP_NO_ERROR;
}

void AppTask::ButtonEventHandler(uint32_t aButtonState, uint32_t aHasChanged)
{
	AppEvent event;
	event.Type = AppEvent::Type::Button;

	if (FUNCTION_BUTTON_MASK & aHasChanged) {
		event.ButtonEvent.PinNo = FUNCTION_BUTTON;
		event.ButtonEvent.Action =
			(FUNCTION_BUTTON_MASK & aButtonState) ? BUTTON_PUSH_EVENT : BUTTON_RELEASE_EVENT;
		event.Handler = FunctionHandler;
		PostEvent(&event);
	}

	if (BLE_ADVERTISEMENT_START_BUTTON_MASK & aButtonState & aHasChanged) {
		event.ButtonEvent.PinNo = BLE_ADVERTISEMENT_START_BUTTON;
		event.ButtonEvent.Action = BUTTON_PUSH_EVENT;
		event.Handler = StartBLEAdvertisementHandler;
		PostEvent(&event);
	}

	if (OPEN_BUTTON_MASK & aHasChanged) {
		event.ButtonEvent.PinNo = OPEN_BUTTON;
		event.ButtonEvent.Action = (OPEN_BUTTON_MASK & aButtonState) ? BUTTON_PUSH_EVENT : BUTTON_RELEASE_EVENT;
		event.Handler = OpenHandler;
		PostEvent(&event);
	}

	if (CLOSE_BUTTON_MASK & aHasChanged) {
		event.ButtonEvent.PinNo = CLOSE_BUTTON;
		event.ButtonEvent.Action =
			(CLOSE_BUTTON_MASK & aButtonState) ? BUTTON_PUSH_EVENT : BUTTON_RELEASE_EVENT;
		event.Handler = CloseHandler;
		PostEvent(&event);
	}
}

#ifdef CONFIG_MCUMGR_SMP_BT
void AppTask::RequestSMPAdvertisingStart(void)
{
	AppEvent event;
	event.Type = AppEvent::Type::StartSMPAdvertising;
	event.Handler = [](AppEvent *) { GetDFUOverSMP().StartBLEAdvertising(); };
	PostEvent(&event);
}
#endif

void AppTask::FunctionTimerTimeoutCallback(k_timer *aTimer)
{
	if (!aTimer)
		return;

	AppEvent event;
	event.Type = AppEvent::Type::Timer;
	event.TimerEvent.Context = k_timer_user_data_get(aTimer);
	event.Handler = FunctionTimerEventHandler;
	PostEvent(&event);
}

void AppTask::FunctionTimerEventHandler(AppEvent *aEvent)
{
	if (!aEvent)
		return;
	if (aEvent->Type != AppEvent::Type::Timer)
		return;

	/* If we reached here, the button was held past FACTORY_RESET_TRIGGER_TIMEOUT, initiate factory reset */
	if (Instance().mFunctionTimerActive && Instance().mMode == OperatingMode::FirmwareUpdate) {
		LOG_INF("Factory Reset Triggered. Release button within %ums to cancel.",
			FACTORY_RESET_TRIGGER_TIMEOUT);

		/* Start timer for FACTORY_RESET_CANCEL_WINDOW_TIMEOUT to allow user to cancel, if required. */
		StartTimer(FACTORY_RESET_CANCEL_WINDOW_TIMEOUT);
		Instance().mMode = OperatingMode::FactoryReset;

#ifdef CONFIG_STATE_LEDS
		/* Turn off all LEDs before starting blink to make sure blink is co-ordinated. */
		sStatusLED.Set(false);
		sUnusedLED.Set(false);

		sStatusLED.Blink(LedConsts::kBlinkRate_ms);
		sUnusedLED.Blink(LedConsts::kBlinkRate_ms);
#endif
	} else if (Instance().mFunctionTimerActive && Instance().mMode == OperatingMode::FactoryReset) {
		/* Actually trigger Factory Reset */
		Instance().mMode = OperatingMode::Normal;
		ConfigurationMgr().InitiateFactoryReset();
	}
}

void AppTask::FunctionHandler(AppEvent *aEvent)
{
	if (!aEvent)
		return;
	if (aEvent->ButtonEvent.PinNo != FUNCTION_BUTTON)
		return;

	/* To initiate factory reset: press the FUNCTION_BUTTON for FACTORY_RESET_TRIGGER_TIMEOUT + */
	/* FACTORY_RESET_CANCEL_WINDOW_TIMEOUT. LED1 AND LED4 start blinking after FACTORY_RESET_TRIGGER_TIMEOUT to
	 * signal */
	/* factory reset has been initiated. To cancel factory reset: release the FUNCTION_BUTTON once all LEDs start */
	/* blinking within the FACTORY_RESET_CANCEL_WINDOW_TIMEOUT */
	if (aEvent->ButtonEvent.Action == BUTTON_PUSH_EVENT) {
		if (!Instance().mFunctionTimerActive && Instance().mMode == OperatingMode::Normal) {
			StartTimer(FACTORY_RESET_TRIGGER_TIMEOUT);
			Instance().mMode = OperatingMode::FirmwareUpdate;
		}
	} else {
		/* If the button was released before factory reset got initiated, trigger a software update. */
		if (Instance().mFunctionTimerActive && Instance().mMode == OperatingMode::FirmwareUpdate) {
			CancelTimer();
#ifdef CONFIG_MCUMGR_SMP_BT
			GetDFUOverSMP().StartServer();
#else
			LOG_INF("Software update is disabled");
#endif
			Instance().mMode = OperatingMode::Normal;
		} else if (Instance().mFunctionTimerActive && Instance().mMode == OperatingMode::FactoryReset) {
			sUnusedLED.Set(false);

			UpdateStatusLED();
			CancelTimer();

			/* Change the mode to normal since factory reset has been canceled. */
			Instance().mMode = OperatingMode::Normal;

			LOG_INF("Factory Reset has been Canceled");
		} else if (Instance().mFunctionTimerActive) {
			CancelTimer();
			Instance().mMode = OperatingMode::Normal;
		}
	}
}

void AppTask::StartBLEAdvertisementHandler(AppEvent *)
{
	if (Server::GetInstance().GetFabricTable().FabricCount() != 0) {
		LOG_INF("Matter service BLE advertising not started - device is already commissioned");
		return;
	}

	if (ConnectivityMgr().IsBLEAdvertisingEnabled()) {
		LOG_INF("BLE advertising is already enabled");
		return;
	}

	if (Server::GetInstance().GetCommissioningWindowManager().OpenBasicCommissioningWindow() != CHIP_NO_ERROR) {
		LOG_ERR("OpenBasicCommissioningWindow() failed");
	}
}

void AppTask::OpenHandler(AppEvent *aEvent)
{
	if (!aEvent)
		return;
	if (aEvent->ButtonEvent.PinNo != OPEN_BUTTON || Instance().mMode != OperatingMode::Normal)
		return;

	if (aEvent->ButtonEvent.Action == BUTTON_PUSH_EVENT) {
		Instance().mOpenButtonIsPressed = true;
		if (Instance().mCloseButtonIsPressed) {
			Instance().ToggleMoveType();
		}
	} else if (aEvent->ButtonEvent.Action == BUTTON_RELEASE_EVENT) {
		if (!Instance().mCloseButtonIsPressed) {
			if (!Instance().mMoveTypeRecentlyChanged) {
				WindowCovering::Instance().SetSingleStepTarget(OperationalState::MovingUpOrOpen);
			} else {
				Instance().mMoveTypeRecentlyChanged = false;
			}
		}
		Instance().mOpenButtonIsPressed = false;
	}
}

void AppTask::CloseHandler(AppEvent *aEvent)
{
	if (!aEvent)
		return;
	if (aEvent->ButtonEvent.PinNo != CLOSE_BUTTON || Instance().mMode != OperatingMode::Normal)
		return;

	if (aEvent->ButtonEvent.Action == BUTTON_PUSH_EVENT) {
		Instance().mCloseButtonIsPressed = true;
		if (Instance().mOpenButtonIsPressed) {
			Instance().ToggleMoveType();
		}
	} else if (aEvent->ButtonEvent.Action == BUTTON_RELEASE_EVENT) {
		if (!Instance().mOpenButtonIsPressed) {
			if (!Instance().mMoveTypeRecentlyChanged) {
				WindowCovering::Instance().SetSingleStepTarget(OperationalState::MovingDownOrClose);
			} else {
				Instance().mMoveTypeRecentlyChanged = false;
			}
		}
		Instance().mCloseButtonIsPressed = false;
	}
}

void AppTask::ToggleMoveType()
{
	if (WindowCovering::Instance().GetMoveType() == WindowCovering::MoveType::LIFT) {
		WindowCovering::Instance().SetMoveType(WindowCovering::MoveType::TILT);
		LOG_INF("Window covering move: tilt");
	} else {
		WindowCovering::Instance().SetMoveType(WindowCovering::MoveType::LIFT);
		LOG_INF("Window covering move: lift");
	}
	mMoveTypeRecentlyChanged = true;
}

void AppTask::UpdateLedStateEventHandler(AppEvent *aEvent)
{
	if (!aEvent)
		return;
	if (aEvent->Type == AppEvent::Type::UpdateLedState) {
		aEvent->UpdateLedStateEvent.LedWidget->UpdateState();
	}
}

void AppTask::LEDStateUpdateHandler(LEDWidget &aLedWidget)
{
	AppEvent event;
	event.Type = AppEvent::Type::UpdateLedState;
	event.Handler = UpdateLedStateEventHandler;
	event.UpdateLedStateEvent.LedWidget = &aLedWidget;
	PostEvent(&event);
}

void AppTask::UpdateStatusLED()
{
#ifdef CONFIG_STATE_LEDS
	/* Update the status LED.
	 *
	 * If thread and service provisioned, keep the LED On constantly.
	 *
	 * If the system has ble connection(s) uptill the stage above, THEN blink the LED at an even
	 * rate of 100ms.
	 *
	 * Otherwise, blink the LED On for a very short time. */
	if (Instance().mIsThreadProvisioned && Instance().mIsThreadEnabled) {
		sStatusLED.Set(true);
	} else if (Instance().mHaveBLEConnections) {
		sStatusLED.Blink(LedConsts::StatusLed::Unprovisioned::kOn_ms,
				 LedConsts::StatusLed::Unprovisioned::kOff_ms);
	} else {
		sStatusLED.Blink(LedConsts::StatusLed::Provisioned::kOn_ms, LedConsts::StatusLed::Provisioned::kOff_ms);
	}
#endif
}

void AppTask::ChipEventHandler(const ChipDeviceEvent *aEvent, intptr_t)
{
	if (!aEvent)
		return;
	switch (aEvent->Type) {
	case DeviceEventType::kCHIPoBLEAdvertisingChange:
#ifdef CONFIG_CHIP_NFC_COMMISSIONING
		if (aEvent->CHIPoBLEAdvertisingChange.Result == kActivity_Started) {
			if (NFCMgr().IsTagEmulationStarted()) {
				LOG_INF("NFC Tag emulation is already started");
			} else {
				ShareQRCodeOverNFC(
					chip::RendezvousInformationFlags(chip::RendezvousInformationFlag::kBLE));
			}
		} else if (aEvent->CHIPoBLEAdvertisingChange.Result == kActivity_Stopped) {
			NFCMgr().StopTagEmulation();
		}
#endif
		Instance().mHaveBLEConnections = ConnectivityMgr().NumBLEConnections() != 0;
		UpdateStatusLED();
		break;
	case DeviceEventType::kThreadStateChange:
		Instance().mIsThreadProvisioned = ConnectivityMgr().IsThreadProvisioned();
		Instance().mIsThreadEnabled = ConnectivityMgr().IsThreadEnabled();
		UpdateStatusLED();
		break;
	default:
		break;
	}
}

void AppTask::CancelTimer()
{
	k_timer_stop(&sFunctionTimer);
	Instance().mFunctionTimerActive = false;
}

void AppTask::StartTimer(uint32_t aTimeoutInMs)
{
	k_timer_start(&sFunctionTimer, K_MSEC(aTimeoutInMs), K_NO_WAIT);
	Instance().mFunctionTimerActive = true;
}

void AppTask::PostEvent(AppEvent *aEvent)
{
	if (!aEvent)
		return;
	if (k_msgq_put(&sAppEventQueue, aEvent, K_NO_WAIT)) {
		LOG_INF("Failed to post event to app task event queue");
	}
}

void AppTask::DispatchEvent(AppEvent *aEvent)
{
	if (!aEvent)
		return;
	if (aEvent->Handler) {
		aEvent->Handler(aEvent);
	} else {
		LOG_INF("Event received with no handler. Dropping event.");
	}
}
