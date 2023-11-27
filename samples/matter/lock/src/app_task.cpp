/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task.h"

#include "app_config.h"
#include "bolt_lock_manager.h"
#include "fabric_table_delegate.h"
#include "led_util.h"

#ifdef CONFIG_CHIP_NUS
#include "bt_nus_service.h"
#endif

#include <platform/CHIPDeviceLayer.h>

#include "board_util.h"
#include <app-common/zap-generated/attributes/Accessors.h>
#include <app/clusters/door-lock-server/door-lock-server.h>
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

#ifdef CONFIG_THREAD_WIFI_SWITCHING
#include "thread_wifi_switch.h"
#endif

#include <dk_buttons_and_leds.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <app/InteractionModelEngine.h>

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
constexpr EndpointId kLockEndpointId = 1;
#if NUMBER_OF_BUTTONS == 2
constexpr uint32_t kAdvertisingTriggerTimeout = 3000;
#endif

#ifdef CONFIG_CHIP_NUS
constexpr uint16_t kAdvertisingIntervalMin = 400;
constexpr uint16_t kAdvertisingIntervalMax = 500;
constexpr uint8_t kLockNUSPriority = 2;
#endif

K_MSGQ_DEFINE(sAppEventQueue, sizeof(AppEvent), kAppEventQueueSize, alignof(AppEvent));
k_timer sFunctionTimer;

#ifdef CONFIG_THREAD_WIFI_SWITCHING
k_timer sSwitchTransportTimer;
constexpr uint32_t kSwitchTransportTimeout = 10000;
#endif

Identify sIdentify = { kLockEndpointId, AppTask::IdentifyStartHandler, AppTask::IdentifyStopHandler,
		       Clusters::Identify::IdentifyTypeEnum::kVisibleIndicator };

LEDWidget sStatusLED;
LEDWidget sLockLED;
#if NUMBER_OF_LEDS == 4
FactoryResetLEDsWrapper<2> sFactoryResetLEDs{ { FACTORY_RESET_SIGNAL_LED, FACTORY_RESET_SIGNAL_LED1 } };
#endif

bool sIsNetworkProvisioned = false;
bool sIsNetworkEnabled = false;
bool sHaveBLEConnections = false;
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

#if defined(CONFIG_CHIP_WIFI) && !defined(CONFIG_THREAD_WIFI_SWITCHING)
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

#ifdef CONFIG_OPENTHREAD_MTD_SED
	err = ConnectivityMgr().SetThreadDeviceType(ConnectivityManager::kThreadDeviceType_SleepyEndDevice);
#else
	err = ConnectivityMgr().SetThreadDeviceType(ConnectivityManager::kThreadDeviceType_MinimalEndDevice);
#endif /* CONFIG_OPENTHREAD_MTD_SED */
	if (err != CHIP_NO_ERROR) {
		LOG_ERR("ConnectivityMgr().SetThreadDeviceType() failed: %s", ErrorStr(err));
		return err;
	}
#endif /* CONFIG_NET_L2_OPENTHREAD */

#ifdef CONFIG_THREAD_WIFI_SWITCHING
	err = ThreadWifiSwitch::StartCurrentTransport();
	if (err != CHIP_NO_ERROR) {
		LOG_ERR("ThreadWifiSwitch::StartCurrentTransport() failed: %" CHIP_ERROR_FORMAT, err.Format());
		return err;
	}
#elif defined(CONFIG_CHIP_WIFI)
	sWiFiCommissioningInstance.Init();
#endif

	/* Initialize LEDs */
	LEDWidget::InitGpio();
	LEDWidget::SetStateUpdateCallback(LEDStateUpdateHandler);

	sStatusLED.Init(SYSTEM_STATE_LED);
	sLockLED.Init(LOCK_STATE_LED);
	sLockLED.Set(BoltLockMgr().IsLocked());

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

#ifdef CONFIG_THREAD_WIFI_SWITCHING
	k_timer_init(&sSwitchTransportTimer, &AppTask::SwitchTransportTimerTimeoutCallback, nullptr);
#endif

#ifdef CONFIG_CHIP_NUS
	/* Initialize Nordic UART Service for Lock purposes */
	if (!GetNUSService().Init(kLockNUSPriority, kAdvertisingIntervalMin, kAdvertisingIntervalMax)) {
		ChipLogError(Zcl, "Cannot initialize NUS service");
	}
	GetNUSService().RegisterCommand("Lock", sizeof("Lock"), NUSLockCallback, nullptr);
	GetNUSService().RegisterCommand("Unlock", sizeof("Unlock"), NUSUnlockCallback, nullptr);
	if(!GetNUSService().StartServer()){
		LOG_ERR("GetNUSService().StartServer() failed");
	}
#endif

	/* Initialize lock manager */
	BoltLockMgr().Init(LockStateChanged);

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

	static chip::CommonCaseDeviceServerInitParams initParams;
	(void)initParams.InitializeStaticResourcesBeforeServerInit();

	ReturnErrorOnFailure(chip::Server::GetInstance().Init(initParams));
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
	event.Handler = [](const AppEvent &) { sLockLED.Blink(LedConsts::kIdentifyBlinkRate_ms); };
	PostEvent(event);
}

void AppTask::IdentifyStopHandler(Identify *)
{
	AppEvent event;
	event.Type = AppEventType::IdentifyStop;
	event.Handler = [](const AppEvent &) { sLockLED.Set(BoltLockMgr().IsLocked()); };
	PostEvent(event);
}

#if NUMBER_OF_BUTTONS == 2
void AppTask::StartBLEAdvertisementAndLockActionEventHandler(const AppEvent &event)
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
			button_event.ButtonEvent.PinNo = BLE_ADVERTISEMENT_START_AND_LOCK_BUTTON;
			button_event.ButtonEvent.Action = static_cast<uint8_t>(AppEventType::ButtonReleased);
			button_event.Handler = LockActionEventHandler;
			PostEvent(button_event);
		}
	}
}
#endif

void AppTask::LockActionEventHandler(const AppEvent &event)
{
	if (BoltLockMgr().IsLocked()) {
		BoltLockMgr().Unlock(BoltLockManager::OperationSource::kButton);
	} else {
		BoltLockMgr().Lock(BoltLockManager::OperationSource::kButton);
	}
}

#ifdef CONFIG_THREAD_WIFI_SWITCHING
void AppTask::SwitchTransportEventHandler(const AppEvent &event)
{
	LOG_INF("Switching to %s", ThreadWifiSwitch::IsThreadActive() ? "Wi-Fi" : "Thread");

	ThreadWifiSwitch::SwitchTransport();
}

void AppTask::SwitchTransportTimerTimeoutCallback(k_timer *timer)
{
	AppEvent event;
	event.Type = AppEventType::Timer;
	event.Handler = SwitchTransportEventHandler;
	PostEvent(event);
}

void AppTask::SwitchTransportTriggerHandler(const AppEvent &event)
{
	if (event.ButtonEvent.Action == static_cast<uint8_t>(AppEventType::ButtonPushed)) {
		LOG_INF("Keep button pressed for %u ms to switch to %s", kSwitchTransportTimeout,
			ThreadWifiSwitch::IsThreadActive() ? "Wi-Fi" : "Thread");
		k_timer_start(&sSwitchTransportTimer, K_MSEC(kSwitchTransportTimeout), K_NO_WAIT);
	} else {
		LOG_INF("Switching to %s cancelled", ThreadWifiSwitch::IsThreadActive() ? "Wi-Fi" : "Thread");
		k_timer_stop(&sSwitchTransportTimer);
	}
}
#endif

void AppTask::ButtonEventHandler(uint32_t buttonState, uint32_t hasChanged)
{
	AppEvent button_event;
	button_event.Type = AppEventType::Button;

#if NUMBER_OF_BUTTONS == 2
	if (BLE_ADVERTISEMENT_START_AND_LOCK_BUTTON_MASK & hasChanged) {
		button_event.ButtonEvent.PinNo = BLE_ADVERTISEMENT_START_AND_LOCK_BUTTON;
		button_event.ButtonEvent.Action = static_cast<uint8_t>(
			(BLE_ADVERTISEMENT_START_AND_LOCK_BUTTON_MASK & buttonState) ? AppEventType::ButtonPushed :
										       AppEventType::ButtonReleased);
		button_event.Handler = StartBLEAdvertisementAndLockActionEventHandler;
		PostEvent(button_event);
	}
#else
	if (LOCK_BUTTON_MASK & buttonState & hasChanged) {
		button_event.ButtonEvent.PinNo = LOCK_BUTTON;
		button_event.ButtonEvent.Action = static_cast<uint8_t>(AppEventType::ButtonPushed);
		button_event.Handler = LockActionEventHandler;
		PostEvent(button_event);
	}

#ifdef CONFIG_THREAD_WIFI_SWITCHING
	if (THREAD_WIFI_SWITCH_BUTTON_MASK & hasChanged) {
		button_event.ButtonEvent.PinNo = THREAD_WIFI_SWITCH_BUTTON;
		button_event.ButtonEvent.Action = static_cast<uint8_t>((THREAD_WIFI_SWITCH_BUTTON_MASK & buttonState) ?
									       AppEventType::ButtonPushed :
									       AppEventType::ButtonReleased);
		button_event.Handler = SwitchTransportTriggerHandler;
		PostEvent(button_event);
	}
#endif

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

		/* Turn off all LEDs before starting blink to make sure blink is coordinated. */
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
	 * If the system has BLE connection(s) uptill the stage above, THEN blink the LED at an even
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

void AppTask::LockStateChanged(BoltLockManager::State state, BoltLockManager::OperationSource source)
{
	switch (state) {
	case BoltLockManager::State::kLockingInitiated:
		LOG_INF("Lock action initiated");
		sLockLED.Blink(50, 50);
#ifdef CONFIG_CHIP_NUS
		GetNUSService().SendData("locking", sizeof("locking"));
#endif
		break;
	case BoltLockManager::State::kLockingCompleted:
		LOG_INF("Lock action completed");
		sLockLED.Set(true);
#ifdef CONFIG_CHIP_NUS
		GetNUSService().SendData("locked", sizeof("locked"));
#endif
		break;
	case BoltLockManager::State::kUnlockingInitiated:
		LOG_INF("Unlock action initiated");
		sLockLED.Blink(50, 50);
#ifdef CONFIG_CHIP_NUS
		GetNUSService().SendData("unlocking", sizeof("unlocking"));
#endif
		break;
	case BoltLockManager::State::kUnlockingCompleted:
		LOG_INF("Unlock action completed");
#ifdef CONFIG_CHIP_NUS
		GetNUSService().SendData("unlocked", sizeof("unlocked"));
#endif
		sLockLED.Set(false);
		break;
	}

	/* Handle changing attribute state in the application */
	Instance().UpdateClusterState(state, source);
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

void AppTask::UpdateClusterState(BoltLockManager::State state, BoltLockManager::OperationSource source)
{
	DlLockState newLockState;

	switch (state) {
	case BoltLockManager::State::kLockingCompleted:
		newLockState = DlLockState::kLocked;
		break;
	case BoltLockManager::State::kUnlockingCompleted:
		newLockState = DlLockState::kUnlocked;
		break;
	default:
		newLockState = DlLockState::kNotFullyLocked;
		break;
	}

	SystemLayer().ScheduleLambda([newLockState, source] {
		chip::app::DataModel::Nullable<chip::app::Clusters::DoorLock::DlLockState> currentLockState;
		chip::app::Clusters::DoorLock::Attributes::LockState::Get(kLockEndpointId, currentLockState);

		if (currentLockState.IsNull()) {
			/* Initialize lock state with start value, but not invoke lock/unlock. */
			chip::app::Clusters::DoorLock::Attributes::LockState::Set(kLockEndpointId, newLockState);
		} else {
			LOG_INF("Updating LockState attribute");

			if (!DoorLockServer::Instance().SetLockState(kLockEndpointId, newLockState, source)) {
				LOG_ERR("Failed to update LockState attribute");
			}
		}
	});
}

#ifdef CONFIG_CHIP_NUS
void AppTask::NUSLockCallback(void *context)
{
	LOG_DBG("Received LOCK command from NUS");
	if (BoltLockMgr().mState == BoltLockManager::State::kLockingCompleted ||
	    BoltLockMgr().mState == BoltLockManager::State::kLockingInitiated) {
		LOG_INF("Device is already locked");
	} else {
		AppEvent nus_event;
		nus_event.Type = AppEventType::NUSCommand;
		nus_event.Handler = LockActionEventHandler;
		PostEvent(nus_event);
	}
}

void AppTask::NUSUnlockCallback(void *context)
{
	LOG_DBG("Received UNLOCK command from NUS");
	if (BoltLockMgr().mState == BoltLockManager::State::kUnlockingCompleted ||
	    BoltLockMgr().mState == BoltLockManager::State::kUnlockingInitiated) {
		LOG_INF("Device is already unlocked");
	} else {
		AppEvent nus_event;
		nus_event.Type = AppEventType::NUSCommand;
		nus_event.Handler = LockActionEventHandler;
		PostEvent(nus_event);
	}
}
#endif
