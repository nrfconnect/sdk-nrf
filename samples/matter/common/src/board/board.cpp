/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "board.h"
#include "app/task_executor.h"
#include "app/group_data_provider.h"

#include <app/server/Server.h>
#include <platform/CHIPDeviceLayer.h>

#include <zephyr/logging/log.h>

#ifdef CONFIG_MCUMGR_TRANSPORT_BT
#include "dfu/smp/dfu_over_smp.h"
#endif

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip::DeviceLayer;

namespace Nrf
{

Board Board::sInstance;

bool Board::Init(button_handler_t buttonHandler, LedStateHandler ledStateHandler)
{
#ifdef CONFIG_DK_LIBRARY
	/* Initialize LEDs */
	LEDWidget::InitGpio();
	LEDWidget::SetStateUpdateCallback(LEDStateUpdateHandler);
	mLED1.Init(DK_LED1);
	mLED2.Init(DK_LED2);
#if NUMBER_OF_LEDS == 3
	mLED3.Init(DK_LED3);
#elif NUMBER_OF_LEDS == 4
	mLED3.Init(DK_LED3);
	mLED4.Init(DK_LED4);
#endif

	/* Initialize buttons */
	int ret = dk_buttons_init(ButtonEventHandler);
	if (ret) {
		LOG_ERR("dk_buttons_init() failed");
		return false;
	}

	/* Register an additional button handler for the user purposes */
	if (buttonHandler) {
		static struct button_handler handler = {
			.cb = buttonHandler,
		};
		dk_button_handler_add(&handler);
	}

	/* Initialize function timer */
	k_timer_init(&mFunctionTimer, &FunctionTimerTimeoutCallback, nullptr);
	k_timer_user_data_set(&mFunctionTimer, this);

	if (ledStateHandler) {
		mLedStateHandler = ledStateHandler;
	}

	mLedStateHandler();
#endif
	return true;
}

void Board::UpdateDeviceState(DeviceState state)
{
	if (mState != state) {
		mState = state;
		mLedStateHandler();
	}
}

void Board::ResetAllLeds()
{
	mLED1SavedState = mLED1.GetState();
	mLED2SavedState = mLED2.GetState();
	mLED1.Set(false);
	mLED2.Set(false);
#if NUMBER_OF_LEDS == 3
	mLED3SavedState = mLED3.GetState();
	mLED3.Set(false);
#elif NUMBER_OF_LEDS == 4
	mLED3SavedState = mLED3.GetState();
	mLED4SavedState = mLED4.GetState();
	mLED3.Set(false);
	mLED4.Set(false);
#endif
}

void Board::RestoreAllLedsState()
{
	mLED1.Set(mLED1SavedState);
	mLED2.Set(mLED2SavedState);
#if NUMBER_OF_LEDS == 3
	mLED3.Set(mLED3SavedState);
#elif NUMBER_OF_LEDS == 4
	mLED3.Set(mLED3SavedState);
	mLED4.Set(mLED4SavedState);
#endif
}

void Board::LEDStateUpdateHandler(LEDWidget &ledWidget)
{
	LEDEvent event;
	event.LedWidget = &ledWidget;
	PostTask([event] { UpdateLedStateEventHandler(event); });
}

void Board::UpdateLedStateEventHandler(const LEDEvent &event)
{
	event.LedWidget->UpdateState();
}

void Board::UpdateStatusLED()
{
	/* Update the status LED.
	 *
	 * If IPv6 network and service provisioned, keep the LED On constantly.
	 *
	 * If the system has BLE connection(s) uptill the stage above, THEN blink the LED at an even
	 * rate of 100ms.
	 *
	 * Otherwise, blink the LED for a very short time. */
	sInstance.mLED1.Set(false);

	switch (sInstance.mState) {
	case DeviceState::DeviceDisconnected:
	case DeviceState::DeviceAdvertisingBLE:
		sInstance.mLED1.Blink(LedConsts::StatusLed::Disconnected::kOn_ms,
				      LedConsts::StatusLed::Disconnected::kOff_ms);

		break;
	case DeviceState::DeviceConnectedBLE:
		sInstance.mLED1.Blink(LedConsts::StatusLed::BleConnected::kOn_ms,
				      LedConsts::StatusLed::BleConnected::kOff_ms);
		break;
	case DeviceState::DeviceProvisioned:
		sInstance.mLED1.Set(true);
		break;
	default:
		break;
	}
}

LEDWidget &Board::GetLED(DeviceLeds led)
{
	switch (led) {
#if NUMBER_OF_LEDS == 3
	case DeviceLeds::LED3:
		return mLED3;
#elif NUMBER_OF_LEDS == 4
	case DeviceLeds::LED3:
		return mLED3;
	case DeviceLeds::LED4:
		return mLED4;
#endif
	case DeviceLeds::LED2:
		return mLED2;
	case DeviceLeds::LED1:
	default:
		return mLED1;
	}
}

void Board::CancelTimer()
{
	k_timer_stop(&mFunctionTimer);
	mFunctionTimerActive = false;
}

void Board::StartTimer(uint32_t timeoutInMs)
{
	k_timer_start(&mFunctionTimer, K_MSEC(timeoutInMs), K_NO_WAIT);
	mFunctionTimerActive = true;
}

void Board::FunctionTimerTimeoutCallback(k_timer *timer)
{
	PostTask([] { FunctionTimerEventHandler(); });
}

void Board::FunctionTimerEventHandler()
{
	/* If we reached here, the button was held past FactoryResetTriggerTimeout, initiate factory reset */
	if (sInstance.mFunction == BoardFunctions::SoftwareUpdate) {
		LOG_INF("Factory reset has been triggered. Release button within %ums to cancel.",
			FactoryResetConsts::kFactoryResetTriggerTimeout);

		/* Start timer for kFactoryResetCancelWindowTimeout to allow user to cancel, if required. */
		sInstance.StartTimer(FactoryResetConsts::kFactoryResetCancelWindowTimeout);
		sInstance.mFunction = BoardFunctions::FactoryReset;

		/* Turn off all LEDs before starting blink to make sure blink is coordinated. */
		sInstance.ResetAllLeds();

		sInstance.mLED1.Blink(LedConsts::kBlinkRate_ms);
		sInstance.mLED2.Blink(LedConsts::kBlinkRate_ms);
#if NUMBER_OF_LEDS == 3
		sInstance.mLED3.Blink(LedConsts::kBlinkRate_ms);
#elif NUMBER_OF_LEDS == 4
		sInstance.mLED3.Blink(LedConsts::kBlinkRate_ms);
		sInstance.mLED4.Blink(LedConsts::kBlinkRate_ms);
#endif
	} else if (sInstance.mFunction == BoardFunctions::FactoryReset) {
		/* Actually trigger Factory Reset */
		sInstance.mFunction = BoardFunctions::None;
		Matter::GroupDataProviderImpl::Instance().WillBeFactoryReseted();
		chip::Server::GetInstance().ScheduleFactoryReset();
	}
}

void Board::ButtonEventHandler(ButtonState buttonState, ButtonMask hasChanged)
{
	if (BLUETOOTH_ADV_BUTTON_MASK & hasChanged) {
		ButtonAction action =
			(BLUETOOTH_ADV_BUTTON_MASK & buttonState) ? ButtonAction::Pressed : ButtonAction::Released;
		PostTask([action] { StartBLEAdvertisementHandler(action); });
	}

	if (FUNCTION_BUTTON_MASK & hasChanged) {
		ButtonAction action =
			(BLUETOOTH_ADV_BUTTON_MASK & buttonState) ? ButtonAction::Pressed : ButtonAction::Released;
		PostTask([action] { FunctionHandler(action); });
	}
}

void Board::FunctionHandler(const ButtonAction &action)
{
	if (action == ButtonAction::Pressed) {
		if (!sInstance.mFunctionTimerActive && sInstance.mFunction == BoardFunctions::None) {
			sInstance.mFunction = BoardFunctions::SoftwareUpdate;
			sInstance.StartTimer(FactoryResetConsts::kFactoryResetTriggerTimeout);
		}
	} else {
		/* If the button was released before factory reset got initiated, trigger a software update. */
		if (sInstance.mFunctionTimerActive && sInstance.mFunction == BoardFunctions::SoftwareUpdate) {
			sInstance.CancelTimer();
			sInstance.mFunction = BoardFunctions::None;
		} else if (sInstance.mFunctionTimerActive && sInstance.mFunction == BoardFunctions::FactoryReset) {
			sInstance.CancelTimer();
			sInstance.RestoreAllLedsState();
			sInstance.mLedStateHandler();
			sInstance.mFunction = BoardFunctions::None;
			LOG_INF("Factory reset has been canceled");
		}
	}
}

void Board::StartBLEAdvertisementHandler(const ButtonAction &action)
{
#ifndef CONFIG_NCS_SAMPLE_MATTER_CUSTOM_BLUETOOTH_ADVERTISING
	if (action == ButtonAction::Pressed) {
		if (sInstance.mState == DeviceState::DeviceProvisioned) {
/* In this case we need to run only Bluetooth LE SMP advertising if it is available */
#ifdef CONFIG_MCUMGR_TRANSPORT_BT
			GetDFUOverSMP().StartServer();
#else
			LOG_INF("Software update is disabled");
#endif
		} else {
			/* In this case we start both Bluetooth LE SMP and Matter advertising at the same time */
			StartBLEAdvertisement();
		}
	}
#endif /* CONFIG_NCS_SAMPLE_MATTER_CUSTOM_BLUETOOTH_ADVERTISING */
}

void Board::StartBLEAdvertisement()
{
	if (chip::Server::GetInstance().GetFabricTable().FabricCount() != 0) {
		LOG_INF("Matter service BLE advertising not started - device is already commissioned");
		return;
	}

	if (chip::DeviceLayer::ConnectivityMgr().IsBLEAdvertisingEnabled()) {
		if (!chip::DeviceLayer::ConnectivityMgr().IsBLEAdvertising()) {
			LOG_INF("BLE advertising is already enabled but not active - restarting");
			/* Kick the BLEMgr state machine to retry to start advertising one more time. */
			CHIP_ERROR err = chip::DeviceLayer::ConnectivityMgr().SetBLEAdvertisingEnabled(true);
			VerifyOrReturn(CHIP_NO_ERROR == err, LOG_ERR("Could not restart Matter advertisement"));
		} else {
			LOG_INF("BLE advertising is already enabled and active");
		}
		return;
	}

	if (chip::Server::GetInstance().GetCommissioningWindowManager().OpenBasicCommissioningWindow() !=
	    CHIP_NO_ERROR) {
		LOG_ERR("OpenBasicCommissioningWindow() failed");
	}
}

void Board::DefaultMatterEventHandler(const ChipDeviceEvent *event, intptr_t /* unused */)
{
	static bool isNetworkProvisioned = false;
	static bool isBleConnected = false;

	switch (event->Type) {
	case DeviceEventType::kCHIPoBLEAdvertisingChange:
		isBleConnected = ConnectivityMgr().NumBLEConnections() != 0;
		break;
#if defined(CONFIG_NET_L2_OPENTHREAD)
	case DeviceEventType::kThreadStateChange:
		isNetworkProvisioned = ConnectivityMgr().IsThreadProvisioned() && ConnectivityMgr().IsThreadEnabled();
		break;
#endif /* CONFIG_NET_L2_OPENTHREAD */
#if defined(CONFIG_CHIP_WIFI)
	case DeviceEventType::kWiFiConnectivityChange:
		isNetworkProvisioned =
			ConnectivityMgr().IsWiFiStationProvisioned() && ConnectivityMgr().IsWiFiStationEnabled();
		break;
#endif /* CONFIG_CHIP_WIFI */
	default:
		break;
	}

	if (isNetworkProvisioned) {
		sInstance.UpdateDeviceState(DeviceState::DeviceProvisioned);
	} else if (isBleConnected) {
		sInstance.UpdateDeviceState(DeviceState::DeviceConnectedBLE);
	} else if (ConnectivityMgr().IsBLEAdvertising()) {
		sInstance.UpdateDeviceState(DeviceState::DeviceAdvertisingBLE);
	} else {
		sInstance.UpdateDeviceState(DeviceState::DeviceDisconnected);
	}
}

} /* namespace Nrf */
