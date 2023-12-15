/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "board_config.h"
#include "board_consts.h"
#include "led_util.h"
#include "led_widget.h"

enum class DeviceState : uint8_t { DeviceDisconnected, DeviceAdvertisingBLE, DeviceConnectedBLE, DeviceProvisioned };
enum class DeviceLeds : uint8_t { LED1, LED2, LED3, LED4 };
enum class ButtonAction : uint8_t { Pressed, Released };
enum class BoardFunctions : uint8_t { None, SoftwareUpdate, FactoryReset };

using ButtonState = uint32_t;
using ButtonMask = uint32_t;
using LedStateHandler = void (*)();

struct LEDEvent {
	LEDWidget *LedWidget;
};

class Board {
	using LedState = bool;

public:
	/**
	 * @brief initialize Board components such as LEDs and Buttons
	 *
	 * buttonCallback: User can register a callback for button interruption that can be used in the
	 * specific way.
	 * The callback enters two arguments:
	 * ButtonState as uint32_t which represents a button state (Pressed, Released)
	 * ButtonMask as uint32_t which represents a bitmask that shows indicates whether the button has been changed
	 *
	 * ledStateHandler: User can register a custom callback for status LED behaviour,
	 * and handle the indications of the device states in the specific way.
	 *
	 * @param buttonCallback the callback function for button interruption.
	 * @param ledStateHandler the custom callback for status LED behaviour.
	 * @return true if board components has been initialized successfully.
	 * @return false if an error occurred.
	 */
	bool Init(button_handler_t buttonHandler = nullptr, LedStateHandler ledStateHandler = nullptr);

	/**
	 * @brief Get the LED located on the board.
	 *
	 * @param led LEDWidget an enum value of the requested led.
	 * @return LEDWidget& a reference of the choosen LED.
	 */
	LEDWidget &GetLED(DeviceLeds led);

	/**
	 * @brief Update a device state to change LED indicator
	 *
	 * The device state should be changed after three interactions:
	 * - The device is disconnected from a network.
	 * - The device is connected via Bluetooth LE and commissioning process is in progress.
	 * - The device is provisioned to the Matter network and the connection is established.
	 *
	 * @param state the new state to be set.
	 */
	void UpdateDeviceState(DeviceState state);

	/**
	 * @brief Get the current device state
	 *
	 * This method returns the current device states as one of the following:
	 * kDeviceDisconnected - the device is not connected to any network
	 * kDeviceAdvertisingBLE - the device is not connected to any network, Bluetooth LE advertising is started
	 * kDeviceConnectedBLE - the device is connected to the Matter controller via Bluetooth LE
	 * kDeviceProvisioned - the device is connected to the Matter network.
	 *
	 * @return DeviceState the current device state defined in @ref DeviceState enum
	 */
	DeviceState GetDeviceState() { return mState; }

	/**
	 * @brief Start Bluetooth LE advertising for Matter commissioning purpose
	 *
	 * This method starts Bluetooth LE advertising and opening the new Matter commissioning window
	 * for 15 minutes.
	 * Within this time commissioning to the Matter network via Bluetooth LE can be performed.
	 *
	 * This method should be run from the application code if the CONFIG_NCS_SAMPLE_MATTER_CUSTOM_BLUETOOTH_ADVERTISING config
	 * is set to "y".
	 *
	 */
	static void StartBLEAdvertisement();

private:
	Board() = default;
	friend Board &GetBoard();
	static Board sInstance;

	/* LEDs */
	static void UpdateStatusLED();
	static void LEDStateUpdateHandler(LEDWidget &ledWidget);
	static void UpdateLedStateEventHandler(const LEDEvent &event);
	void ResetAllLeds();

	LEDWidget mLED1;
	LEDWidget mLED2;
	k_timer mFunctionTimer;
	DeviceState mState = DeviceState::DeviceDisconnected;
	LedStateHandler mLedStateHandler = UpdateStatusLED;
#if NUMBER_OF_LEDS == 3
	LEDWidget mLED3;
#elif NUMBER_OF_LEDS == 4
	LEDWidget mLED3;
	LEDWidget mLED4;
#endif

	/* Function Timer */
	void CancelTimer();
	void StartTimer(uint32_t timeoutInMs);
	static void FunctionTimerTimeoutCallback(k_timer *timer);
	static void FunctionTimerEventHandler();

	bool mFunctionTimerActive = false;
	BoardFunctions mFunction;

	/* Buttons */
	static void ButtonEventHandler(ButtonState buttonState, ButtonMask hasChanged);
	static void FunctionHandler(const ButtonAction &action);
	static void StartBLEAdvertisementHandler(const ButtonAction &action);

	button_handler_t mButtonHandler = nullptr;
};

/**
 * @brief Get the Board instance
 *
 * Obtain the Board instance to initialize the module, get the LEDWidget object,
 * and update the device state.
 *
 * @return Board& instance for the board.
 */
inline Board &GetBoard()
{
	return Board::sInstance;
}
