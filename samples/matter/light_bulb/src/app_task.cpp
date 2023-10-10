/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task.h"

#include "app_config.h"
#include "fabric_table_delegate.h"
#include "led_util.h"
#include "pwm_device.h"

#include <platform/CHIPDeviceLayer.h>

#include <app-common/zap-generated/attributes/Accessors.h>
#include <app/DeferredAttributePersistenceProvider.h>
#include <app/clusters/identify-server/identify-server.h>
#include <app/server/OnboardingCodesUtil.h>
#include <app/server/Server.h>
#include <credentials/DeviceAttestationCredsProvider.h>
#include <credentials/examples/DeviceAttestationCredsExample.h>
#include <lib/support/CHIPMem.h>
#include <lib/support/CodeUtils.h>
#include <system/SystemError.h>

#ifdef CONFIG_CHIP_WIFI
#include <app/clusters/network-commissioning/network-commissioning.h>
#include <platform/nrfconnect/wifi/NrfWiFiDriver.h>
#endif

#ifdef CONFIG_CHIP_OTA_REQUESTOR
#include "ota_util.h"
#endif

#include <dk_buttons_and_leds.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::app;
using namespace ::chip::Credentials;
using namespace ::chip::DeviceLayer;

namespace
{
constexpr uint32_t kFactoryResetTriggerTimeout = 3000;
constexpr uint32_t kFactoryResetCancelWindowTimeout = 3000;
constexpr size_t kAppEventQueueSize = 10;
constexpr EndpointId kLightEndpointId = 1;
constexpr uint8_t kDefaultMinLevel = 0;
constexpr uint8_t kDefaultMaxLevel = 254;
#if NUMBER_OF_BUTTONS == 2
constexpr uint32_t kAdvertisingTriggerTimeout = 3000;
#endif
constexpr uint16_t kTriggerEffectTimeout = 5000;
constexpr uint16_t kTriggerEffectFinishTimeout = 1000;

K_MSGQ_DEFINE(sAppEventQueue, sizeof(AppEvent), kAppEventQueueSize, alignof(AppEvent));
k_timer sFunctionTimer;
k_timer sTriggerEffectTimer;

Identify sIdentify = { kLightEndpointId, AppTask::IdentifyStartHandler, AppTask::IdentifyStopHandler,
		       EMBER_ZCL_IDENTIFY_IDENTIFY_TYPE_VISIBLE_LED, AppTask::TriggerIdentifyEffectHandler };

LEDWidget sStatusLED;
LEDWidget sIdentifyLED;
#if NUMBER_OF_LEDS == 4
FactoryResetLEDsWrapper<2> sFactoryResetLEDs{ { FACTORY_RESET_SIGNAL_LED, FACTORY_RESET_SIGNAL_LED1 } };
#endif

bool sIsNetworkProvisioned = false;
bool sIsNetworkEnabled = false;
bool sHaveBLEConnections = false;
bool sIsTriggerEffectActive = false;

const struct pwm_dt_spec sLightPwmDevice = PWM_DT_SPEC_GET(DT_ALIAS(pwm_led1));

// Define a custom attribute persister which makes actual write of the CurrentLevel attribute value
// to the non-volatile storage only when it has remained constant for 5 seconds. This is to reduce
// the flash wearout when the attribute changes frequently as a result of MoveToLevel command.
// DeferredAttribute object describes a deferred attribute, but also holds a buffer with a value to
// be written, so it must live so long as the DeferredAttributePersistenceProvider object.
DeferredAttribute gCurrentLevelPersister(ConcreteAttributePath(kLightEndpointId, Clusters::LevelControl::Id,
							       Clusters::LevelControl::Attributes::CurrentLevel::Id));
DeferredAttributePersistenceProvider gDeferredAttributePersister(Server::GetInstance().GetDefaultAttributePersister(),
								 Span<DeferredAttribute>(&gCurrentLevelPersister, 1),
								 System::Clock::Milliseconds32(5000));

} /* namespace */

namespace LedConsts
{
constexpr uint32_t kBlinkRate_ms{ 500 };
constexpr uint32_t kIdentifyBlinkRate_ms{ 500 };

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

#ifdef CONFIG_CHIP_WIFI
app::Clusters::NetworkCommissioning::Instance
	sWiFiCommissioningInstance(0, &(NetworkCommissioning::NrfWiFiDriver::Instance()));
#endif

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

#if defined(CONFIG_NET_L2_OPENTHREAD)
	err = ThreadStackMgr().InitThreadStack();
	if (err != CHIP_NO_ERROR) {
		LOG_ERR("ThreadStackMgr().InitThreadStack() failed: %s", ErrorStr(err));
		return err;
	}

	err = ConnectivityMgr().SetThreadDeviceType(ConnectivityManager::kThreadDeviceType_Router);

	if (err != CHIP_NO_ERROR) {
		LOG_ERR("ThreadStackMgr().InitThreadStack() failed: %s", ErrorStr(err));
		return err;
	}

#elif defined(CONFIG_CHIP_WIFI)
	sWiFiCommissioningInstance.Init();
#else
	return CHIP_ERROR_INTERNAL;
#endif /* CONFIG_NET_L2_OPENTHREAD */

	/* Initialize LEDs */
	LEDWidget::InitGpio();
	LEDWidget::SetStateUpdateCallback(LEDStateUpdateHandler);

	sStatusLED.Init(SYSTEM_STATE_LED);
	sIdentifyLED.Init(LIGHTING_STATE_LED);
	sIdentifyLED.Set(false);

	UpdateStatusLED();

	/* Initialize buttons */
	int ret = dk_buttons_init(ButtonEventHandler);
	if (ret) {
		LOG_ERR("dk_buttons_init() failed");
		return chip::System::MapErrorZephyr(ret);
	}

	/* Initialize function timer */
	k_timer_init(&sFunctionTimer, &AppTask::FunctionTimerTimeoutCallback, nullptr);
	k_timer_user_data_set(&sFunctionTimer, this);

	/* Initialize trigger effect timer */
	k_timer_init(&sTriggerEffectTimer, &AppTask::TriggerEffectTimerTimeoutCallback, nullptr);

	/* Initialize lighting device (PWM) */
	uint8_t minLightLevel = kDefaultMinLevel;
	Clusters::LevelControl::Attributes::MinLevel::Get(kLightEndpointId, &minLightLevel);

	uint8_t maxLightLevel = kDefaultMaxLevel;
	Clusters::LevelControl::Attributes::MaxLevel::Get(kLightEndpointId, &maxLightLevel);

	ret = mPWMDevice.Init(&sLightPwmDevice, minLightLevel, maxLightLevel, maxLightLevel);
	if (ret != 0) {
		return chip::System::MapErrorZephyr(ret);
	}
	mPWMDevice.SetCallbacks(ActionInitiated, ActionCompleted);

#ifdef CONFIG_CHIP_OTA_REQUESTOR
	/* OTA image confirmation must be done before the factory data init. */
	OtaConfirmNewImage();
#endif

#ifdef CONFIG_MCUMGR_TRANSPORT_BT
	/* Initialize DFU over SMP */
	GetDFUOverSMP().Init();
	GetDFUOverSMP().ConfirmNewImage();
#endif

	/* Initialize CHIP server */
#if CONFIG_CHIP_FACTORY_DATA
	ReturnErrorOnFailure(mFactoryDataProvider.Init());
	SetDeviceInstanceInfoProvider(&mFactoryDataProvider);
	SetDeviceAttestationCredentialsProvider(&mFactoryDataProvider);
	SetCommissionableDataProvider(&mFactoryDataProvider);
#else
	SetDeviceInstanceInfoProvider(&DeviceInstanceInfoProviderMgrImpl());
	SetDeviceAttestationCredentialsProvider(Examples::GetExampleDACProvider());
#endif

	static CommonCaseDeviceServerInitParams initParams;
	(void)initParams.InitializeStaticResourcesBeforeServerInit();

	ReturnErrorOnFailure(chip::Server::GetInstance().Init(initParams));
	app::SetAttributePersistenceProvider(&gDeferredAttributePersister);
	ConfigurationMgr().LogDeviceConfig();
	PrintOnboardingCodes(chip::RendezvousInformationFlags(chip::RendezvousInformationFlag::kBLE));
	AppFabricTableDelegate::Init();

	/*
	 * Add CHIP event handler and start CHIP thread.
	 * Note that all the initialization code should happen prior to this point to avoid data races
	 * between the main and the CHIP threads.
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

void AppTask::IdentifyStartHandler(Identify *)
{
	AppEvent event;
	event.Type = AppEventType::IdentifyStart;
	event.Handler = [](const AppEvent &) {
		Instance().mPWMDevice.SuppressOutput();
		sIdentifyLED.Blink(LedConsts::kIdentifyBlinkRate_ms);
	};
	PostEvent(event);
}

void AppTask::IdentifyStopHandler(Identify *)
{
	AppEvent event;
	event.Type = AppEventType::IdentifyStop;
	event.Handler = [](const AppEvent &) {
		sIdentifyLED.Set(false);
		Instance().mPWMDevice.ApplyLevel();
	};
	PostEvent(event);
}

void AppTask::TriggerEffectTimerTimeoutCallback(k_timer *timer)
{
	LOG_INF("Identify effect completed");

	sIsTriggerEffectActive = false;

	sIdentifyLED.Set(false);
	Instance().mPWMDevice.ApplyLevel();
}

void AppTask::TriggerIdentifyEffectHandler(Identify *identify)
{
	switch (identify->mCurrentEffectIdentifier) {
	/* Just handle all effects in the same way. */
	case EMBER_ZCL_IDENTIFY_EFFECT_IDENTIFIER_BLINK:
	case EMBER_ZCL_IDENTIFY_EFFECT_IDENTIFIER_BREATHE:
	case EMBER_ZCL_IDENTIFY_EFFECT_IDENTIFIER_OKAY:
	case EMBER_ZCL_IDENTIFY_EFFECT_IDENTIFIER_CHANNEL_CHANGE:
		LOG_INF("Identify effect identifier changed to %d", identify->mCurrentEffectIdentifier);

		sIsTriggerEffectActive = false;

		k_timer_stop(&sTriggerEffectTimer);
		k_timer_start(&sTriggerEffectTimer, K_MSEC(kTriggerEffectTimeout), K_NO_WAIT);

		Instance().mPWMDevice.SuppressOutput();
		sIdentifyLED.Blink(LedConsts::kIdentifyBlinkRate_ms);

		break;
	case EMBER_ZCL_IDENTIFY_EFFECT_IDENTIFIER_FINISH_EFFECT:
		LOG_INF("Identify effect finish triggered");
		k_timer_stop(&sTriggerEffectTimer);
		k_timer_start(&sTriggerEffectTimer, K_MSEC(kTriggerEffectFinishTimeout), K_NO_WAIT);
		break;
	case EMBER_ZCL_IDENTIFY_EFFECT_IDENTIFIER_STOP_EFFECT:
		if (sIsTriggerEffectActive) {
			sIsTriggerEffectActive = false;

			k_timer_stop(&sTriggerEffectTimer);

			sIdentifyLED.Set(false);
			Instance().mPWMDevice.ApplyLevel();
		}
		break;
	default:
		LOG_ERR("Received invalid effect identifier.");
		break;
	}
}

#if NUMBER_OF_BUTTONS == 2
void AppTask::StartBLEAdvertisementAndLightActionEventHandler(const AppEvent &event)
{
	if (event.ButtonEvent.Action == static_cast<uint8_t>(AppEventType::ButtonPushed)) {
		Instance().StartTimer(kAdvertisingTriggerTimeout);
		Instance().mFunction = FunctionEvent::AdvertisingStart;
	} else {
		if (Instance().mFunction == FunctionEvent::AdvertisingStart && Instance().mFunctionTimerActive) {
			Instance().CancelTimer();
			Instance().mFunction = FunctionEvent::NoneSelected;

			AppEvent button_event;
			button_event.Type = AppEventType::Button;
			button_event.ButtonEvent.PinNo = BLE_ADVERTISEMENT_START_AND_LIGHTING_BUTTON;
			button_event.ButtonEvent.Action = static_cast<uint8_t>(AppEventType::ButtonReleased);
			button_event.Handler = LightingActionEventHandler;
			PostEvent(button_event);
		}
	}
}
#endif

void AppTask::LightingActionEventHandler(const AppEvent &event)
{
	PWMDevice::Action_t action = PWMDevice::INVALID_ACTION;
	int32_t actor = 0;

	if (event.Type == AppEventType::Lighting) {
		action = static_cast<PWMDevice::Action_t>(event.LightingEvent.Action);
		actor = event.LightingEvent.Actor;
	} else if (event.Type == AppEventType::Button) {
		action = Instance().mPWMDevice.IsTurnedOn() ? PWMDevice::OFF_ACTION : PWMDevice::ON_ACTION;
		actor = static_cast<int32_t>(AppEventType::Button);
	}

	if (action != PWMDevice::INVALID_ACTION && Instance().mPWMDevice.InitiateAction(action, actor, NULL)) {
		LOG_INF("Action is already in progress or active.");
	}
}

void AppTask::ButtonEventHandler(uint32_t buttonState, uint32_t hasChanged)
{
	AppEvent button_event;
	button_event.Type = AppEventType::Button;

#if NUMBER_OF_BUTTONS == 2
	if (BLE_ADVERTISEMENT_START_AND_LIGHTING_BUTTON_MASK & hasChanged) {
		button_event.ButtonEvent.PinNo = BLE_ADVERTISEMENT_START_AND_LIGHTING_BUTTON;
		button_event.ButtonEvent.Action =
			static_cast<uint8_t>((BLE_ADVERTISEMENT_START_AND_LIGHTING_BUTTON_MASK & buttonState) ?
						     AppEventType::ButtonPushed :
						     AppEventType::ButtonReleased);
		button_event.Handler = StartBLEAdvertisementAndLightActionEventHandler;
		PostEvent(button_event);
	}
#else
	if (LIGHTING_BUTTON_MASK & buttonState & hasChanged) {
		button_event.ButtonEvent.PinNo = LIGHTING_BUTTON;
		button_event.ButtonEvent.Action = static_cast<uint8_t>(AppEventType::ButtonPushed);
		button_event.Handler = LightingActionEventHandler;
		PostEvent(button_event);
	}

	if (BLE_ADVERTISEMENT_START_BUTTON_MASK & buttonState & hasChanged) {
		button_event.ButtonEvent.PinNo = BLE_ADVERTISEMENT_START_BUTTON;
		button_event.ButtonEvent.Action = static_cast<uint8_t>(AppEventType::ButtonPushed);
		button_event.Handler = StartBLEAdvertisementHandler;
		PostEvent(button_event);
	}
#endif

	if (FUNCTION_BUTTON_MASK & hasChanged) {
		button_event.ButtonEvent.PinNo = FUNCTION_BUTTON;
		button_event.ButtonEvent.Action =
			static_cast<uint8_t>((FUNCTION_BUTTON_MASK & buttonState) ? AppEventType::ButtonPushed :
										    AppEventType::ButtonReleased);
		button_event.Handler = FunctionHandler;
		PostEvent(button_event);
	}
}

void AppTask::FunctionTimerTimeoutCallback(k_timer *timer)
{
	if (!timer) {
		return;
	}

	Instance().mFunctionTimerActive = false;
	AppEvent event;
	event.Type = AppEventType::Timer;
	event.TimerEvent.Context = k_timer_user_data_get(timer);
	event.Handler = FunctionTimerEventHandler;
	PostEvent(event);
}

void AppTask::FunctionTimerEventHandler(const AppEvent &event)
{
	if (event.Type != AppEventType::Timer) {
		return;
	}

	/* If we reached here, the button was held past kFactoryResetTriggerTimeout, initiate factory reset */
	if (Instance().mFunction == FunctionEvent::SoftwareUpdate) {
		LOG_INF("Factory Reset Triggered. Release button within %ums to cancel.", kFactoryResetTriggerTimeout);

		/* Start timer for kFactoryResetCancelWindowTimeout to allow user to cancel, if required. */
		Instance().StartTimer(kFactoryResetCancelWindowTimeout);
		Instance().mFunction = FunctionEvent::FactoryReset;

		/* Turn off all LEDs before starting blink to make sure blink is co-ordinated. */
		sStatusLED.Set(false);
#if NUMBER_OF_LEDS == 4
		sFactoryResetLEDs.Set(false);
#endif

		sStatusLED.Blink(LedConsts::kBlinkRate_ms);
#if NUMBER_OF_LEDS == 4
		sFactoryResetLEDs.Blink(LedConsts::kBlinkRate_ms);
#endif
	} else if (Instance().mFunction == FunctionEvent::FactoryReset) {
		/* Actually trigger Factory Reset */
		Instance().mFunction = FunctionEvent::NoneSelected;
		chip::Server::GetInstance().ScheduleFactoryReset();
	} else if (Instance().mFunction == FunctionEvent::AdvertisingStart) {
		/* The button was held past kAdvertisingTriggerTimeout, start BLE advertisement
		   if we have 2 buttons UI*/
#if NUMBER_OF_BUTTONS == 2
		StartBLEAdvertisementHandler(event);
		Instance().mFunction = FunctionEvent::NoneSelected;
#endif
	}
}

void AppTask::FunctionHandler(const AppEvent &event)
{
	if (event.ButtonEvent.PinNo != FUNCTION_BUTTON)
		return;

	/* To trigger software update: press the FUNCTION_BUTTON button briefly (< kFactoryResetTriggerTimeout)
	 * To initiate factory reset: press the FUNCTION_BUTTON for kFactoryResetTriggerTimeout +
	 * kFactoryResetCancelWindowTimeout All LEDs start blinking after kFactoryResetTriggerTimeout to signal factory
	 * reset has been initiated. To cancel factory reset: release the FUNCTION_BUTTON once all LEDs start blinking
	 * within the kFactoryResetCancelWindowTimeout.
	 */
	if (event.ButtonEvent.Action == static_cast<uint8_t>(AppEventType::ButtonPushed)) {
		if (!Instance().mFunctionTimerActive && Instance().mFunction == FunctionEvent::NoneSelected) {
			Instance().StartTimer(kFactoryResetTriggerTimeout);
			Instance().mFunction = FunctionEvent::SoftwareUpdate;
		}
	} else {
		/* If the button was released before factory reset got initiated, trigger a software update. */
		if (Instance().mFunctionTimerActive && Instance().mFunction == FunctionEvent::SoftwareUpdate) {
			Instance().CancelTimer();
			Instance().mFunction = FunctionEvent::NoneSelected;

#ifdef CONFIG_MCUMGR_TRANSPORT_BT
			GetDFUOverSMP().StartServer();
#else
			LOG_INF("Software update is disabled");
#endif
		} else if (Instance().mFunctionTimerActive && Instance().mFunction == FunctionEvent::FactoryReset) {
#if NUMBER_OF_LEDS == 4
			sFactoryResetLEDs.Set(false);
#endif
			UpdateStatusLED();
			Instance().CancelTimer();
			Instance().mFunction = FunctionEvent::NoneSelected;
			LOG_INF("Factory Reset has been Canceled");
		}
	}
}

void AppTask::StartBLEAdvertisementHandler(const AppEvent &)
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

void AppTask::UpdateLedStateEventHandler(const AppEvent &event)
{
	if (event.Type == AppEventType::UpdateLedState) {
		event.UpdateLedStateEvent.LedWidget->UpdateState();
	}
}

void AppTask::LEDStateUpdateHandler(LEDWidget &ledWidget)
{
	AppEvent event;
	event.Type = AppEventType::UpdateLedState;
	event.Handler = UpdateLedStateEventHandler;
	event.UpdateLedStateEvent.LedWidget = &ledWidget;
	PostEvent(event);
}

void AppTask::UpdateStatusLED()
{
	/* Update the status LED.
	 *
	 * If IPv6 network and service provisioned, keep the LED On constantly.
	 *
	 * If the system has ble connection(s) uptill the stage above, THEN blink the LED at an even
	 * rate of 100ms.
	 *
	 * Otherwise, blink the LED for a very short time. */
	if (sIsNetworkProvisioned && sIsNetworkEnabled) {
		sStatusLED.Set(true);
	} else if (sHaveBLEConnections) {
		sStatusLED.Blink(LedConsts::StatusLed::Unprovisioned::kOn_ms,
				 LedConsts::StatusLed::Unprovisioned::kOff_ms);
	} else {
		sStatusLED.Blink(LedConsts::StatusLed::Provisioned::kOn_ms, LedConsts::StatusLed::Provisioned::kOff_ms);
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
#if defined(CONFIG_NET_L2_OPENTHREAD)
	case DeviceEventType::kDnssdInitialized:
#if CONFIG_CHIP_OTA_REQUESTOR
		InitBasicOTARequestor();
#endif /* CONFIG_CHIP_OTA_REQUESTOR */
		break;
	case DeviceEventType::kThreadStateChange:
		sIsNetworkProvisioned = ConnectivityMgr().IsThreadProvisioned();
		sIsNetworkEnabled = ConnectivityMgr().IsThreadEnabled();
#elif defined(CONFIG_CHIP_WIFI)
	case DeviceEventType::kWiFiConnectivityChange:
		sIsNetworkProvisioned = ConnectivityMgr().IsWiFiStationProvisioned();
		sIsNetworkEnabled = ConnectivityMgr().IsWiFiStationEnabled();
#if CONFIG_CHIP_OTA_REQUESTOR
		if (event->WiFiConnectivityChange.Result == kConnectivity_Established) {
			InitBasicOTARequestor();
		}
#endif /* CONFIG_CHIP_OTA_REQUESTOR */
#endif
		UpdateStatusLED();
		break;
	default:
		break;
	}
}

void AppTask::CancelTimer()
{
	k_timer_stop(&sFunctionTimer);
	mFunctionTimerActive = false;
}

void AppTask::StartTimer(uint32_t timeoutInMs)
{
	k_timer_start(&sFunctionTimer, K_MSEC(timeoutInMs), K_NO_WAIT);
	mFunctionTimerActive = true;
}

void AppTask::ActionInitiated(PWMDevice::Action_t action, int32_t actor)
{
	if (action == PWMDevice::ON_ACTION) {
		LOG_INF("Turn On Action has been initiated");
	} else if (action == PWMDevice::OFF_ACTION) {
		LOG_INF("Turn Off Action has been initiated");
	} else if (action == PWMDevice::LEVEL_ACTION) {
		LOG_INF("Level Action has been initiated");
	}
}

void AppTask::ActionCompleted(PWMDevice::Action_t action, int32_t actor)
{
	if (action == PWMDevice::ON_ACTION) {
		LOG_INF("Turn On Action has been completed");
	} else if (action == PWMDevice::OFF_ACTION) {
		LOG_INF("Turn Off Action has been completed");
	} else if (action == PWMDevice::LEVEL_ACTION) {
		LOG_INF("Level Action has been completed");
	}

	if (actor == static_cast<int32_t>(AppEventType::Button)) {
		Instance().UpdateClusterState();
	}
}

void AppTask::PostEvent(const AppEvent &event)
{
	if (k_msgq_put(&sAppEventQueue, &event, K_NO_WAIT) != 0) {
		LOG_INF("Failed to post event to app task event queue");
	}
}

void AppTask::DispatchEvent(const AppEvent &event)
{
	if (event.Handler) {
		event.Handler(event);
	} else {
		LOG_INF("Event received with no handler. Dropping event.");
	}
}

void AppTask::UpdateClusterState()
{
	SystemLayer().ScheduleLambda([this] {
		/* write the new on/off value */
		EmberAfStatus status =
			Clusters::OnOff::Attributes::OnOff::Set(kLightEndpointId, mPWMDevice.IsTurnedOn());

		if (status != EMBER_ZCL_STATUS_SUCCESS) {
			LOG_ERR("Updating on/off cluster failed: %x", status);
		}

		/* write the current level */
		status = Clusters::LevelControl::Attributes::CurrentLevel::Set(kLightEndpointId, mPWMDevice.GetLevel());

		if (status != EMBER_ZCL_STATUS_SUCCESS) {
			LOG_ERR("Updating level cluster failed: %x", status);
		}
	});
}
