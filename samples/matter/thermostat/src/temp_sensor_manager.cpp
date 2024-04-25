/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "temp_sensor_manager.h"
#include "app/task_executor.h"
#include "app_task.h"
#include "temperature_measurement/sensor.h"

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace chip;
using namespace chip::DeviceLayer;

static constexpr EndpointId kThermostatEndpoint = 1;

k_timer sSensorTimer;

CHIP_ERROR TempSensorManager::Init()
{
	k_timer_init(&sSensorTimer, &TempSensorManager::TimerEventHandler, nullptr);
	k_timer_start(&sSensorTimer, K_MSEC(kSensorTimerPeriodMs), K_MSEC(kSensorTimerPeriodMs));
	/* Workaround: Default null values are not correctly propagated from the zap file. Set it manually to make sure
	 * the starting value is cleared. */
	TempSensorManager::Instance().ClearLocalTemperature();
	TempSensorManager::Instance().ClearOutdoorTemperature();
	TemperatureSensor::Instance().FullMeasurement();

	return CHIP_NO_ERROR;
}

void TempSensorManager::TimerEventHandler(k_timer *timer)
{
	Nrf::PostTask([] { TempSensorManager::SensorTimerEventHandler(); });
}

/*
 *	This function is called when the timer is stopped and initiate temperature measurement.
 *	Depending ond object given to GetMeasurement() function thermostat will use true temeprature device or simulated
 *	values
 */

void TempSensorManager::SensorTimerEventHandler()
{
	TemperatureSensor::Instance().FullMeasurement();
}

void TempSensorManager::SetLocalTemperature(int16_t temperature)
{
	PlatformMgr().LockChipStack();
	app::Clusters::Thermostat::Attributes::LocalTemperature::Set(kThermostatEndpoint, temperature);
	PlatformMgr().UnlockChipStack();
}

void TempSensorManager::ClearLocalTemperature()
{
	PlatformMgr().LockChipStack();
	app::Clusters::Thermostat::Attributes::LocalTemperature::SetNull(kThermostatEndpoint);
	PlatformMgr().UnlockChipStack();
}

void TempSensorManager::SetOutdoorTemperature(int16_t temperature)
{
	PlatformMgr().LockChipStack();
	app::Clusters::Thermostat::Attributes::OutdoorTemperature::Set(kThermostatEndpoint, temperature);
	PlatformMgr().UnlockChipStack();
}

void TempSensorManager::ClearOutdoorTemperature()
{
	PlatformMgr().LockChipStack();
	app::Clusters::Thermostat::Attributes::OutdoorTemperature::SetNull(kThermostatEndpoint);
	PlatformMgr().UnlockChipStack();
}
