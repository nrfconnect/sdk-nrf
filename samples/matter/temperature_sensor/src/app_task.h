/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "board/board.h"

#include <platform/CHIPDeviceLayer.h>

struct Identify;

class AppTask {
public:
	static AppTask &Instance()
	{
		static AppTask sAppTask;
		return sAppTask;
	};

	CHIP_ERROR StartApp();
	static void IdentifyStartHandler(Identify *);
	static void IdentifyStopHandler(Identify *);

	static constexpr chip::EndpointId kTemperatureSensorEndpointId = 1;

	void UpdateTemperatureMeasurement()
	{
		/* Linear temperature increase that is wrapped around to min value after reaching the max value. */
		if (mCurrentTemperature < mTemperatureSensorMaxValue) {
			mCurrentTemperature++;
		} else {
			mCurrentTemperature = mTemperatureSensorMinValue;
		}
	}

	int16_t GetCurrentTemperature() const { return mCurrentTemperature; }

private:
	CHIP_ERROR Init();
	k_timer mTimer;

	static constexpr uint16_t kTemperatureMeasurementIntervalMs = 10000;

	static void UpdateTemperatureTimeoutCallback(k_timer *timer);

	static void ButtonEventHandler(Nrf::ButtonState state, Nrf::ButtonMask hasChanged);

	int16_t mTemperatureSensorMaxValue = 0;
	int16_t mTemperatureSensorMinValue = 0;
	int16_t mCurrentTemperature = 0;
};
