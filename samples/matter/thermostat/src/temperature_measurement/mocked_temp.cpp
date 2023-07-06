/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "sensor.h"
#include "temp_sensor_manager.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

constexpr uint16_t kSimulatedReadingFrequency = (60000 / kSensorTimerPeriodMs); /*Change simulated number every minute*/

#if CONFIG_THERMOSTAT_TEMPERATURE_STEP == 0
constexpr int16_t sMockTemp[] = { 2000, 2731, 1600, 2100, 1937, 3011, 1500, 1899 };

#else
constexpr int16_t kMockedTemperatureStep = CONFIG_THERMOSTAT_TEMPERATURE_STEP;
#endif

void MockedSensor::TemperatureMeasurement()
{
#if CONFIG_THERMOSTAT_TEMPERATURE_STEP == 0

	int16_t temperature = sMockTemp[mMockTempIndex];

	mCycleCounter++;
	if (mCycleCounter >= kSimulatedReadingFrequency) {
		if (mMockTempIndex >= ArraySize(sMockTemp) - 1) {
			mMockTempIndex = 0;
		} else {
			mMockTempIndex++;
		}
		mCycleCounter = 0;
	}
#else
#ifdef CONFIG_THERMOSTAT_MOCKED_TEMPERATURE_RAISE
	static int16_t temperature = CONFIG_THERMOSTAT_MINIMUM_MOCKED_TEMPERATURE - CONFIG_THERMOSTAT_TEMPERATURE_STEP;

	temperature = (PreviousTemperature >= CONFIG_THERMOSTAT_MAXIMUM_MOCKED_TEMPERATURE) ?
			      CONFIG_THERMOSTAT_MINIMUM_MOCKED_TEMPERATURE :
			      mPreviousTemperature + CONFIG_THERMOSTAT_TEMPERATURE_STEP;
#else
	static int16_t temperature = CONFIG_THERMOSTAT_MAXIMUM_MOCKED_TEMPERATURE - CONFIG_THERMOSTAT_TEMPERATURE_STEP;

	temperature = (PreviousTemperature <= CONFIG_THERMOSTAT_MINIMUM_MOCKED_TEMPERATURE) ?
			      CONFIG_THERMOSTAT_MAXIMUM_MOCKED_TEMPERATURE :
			      mPreviousTemperature - CONFIG_THERMOSTAT_TEMPERATURE_STEP;
#endif
#endif

	mPreviousTemperature = temperature;

	TempSensorManager::Instance().SetLocalTemperature(temperature);
}
