/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "temp_sensor_manager.h"
#include "app_task.h"
#include "temperature_measurement/sensor.h"

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace chip;
using namespace chip::DeviceLayer;

constexpr EndpointId kThermostatEndpoint = 1;

k_timer sSensorTimer;

CHIP_ERROR TempSensorManager::Init()
{
	k_timer_init(&sSensorTimer, &TempSensorManager::TimerEventHandler, nullptr);
	k_timer_start(&sSensorTimer, K_MSEC(kSensorTimerPeriodMs), K_MSEC(kSensorTimerPeriodMs));
	return CHIP_NO_ERROR;
}

void TempSensorManager::TimerEventHandler(k_timer *timer)
{
	AppEvent event;
	event.Type = AppEventType::Timer;
	event.TimerEvent.Context = k_timer_user_data_get(timer);
	event.Handler = TempSensorManager::SensorTimerEventHandler;
	AppTask::Instance().PostEvent(event);
}

/*
 *	Initiate measurement, by giving RealSensor instance  function
 *	read real sensor value. By given MockedSensor user can rad mocked value.
 */

void TempSensorManager::SensorTimerEventHandler(const AppEvent &Event)
{
#ifdef CONFIG_THERMOSTAT_EXTERNAL_SENSOR
	GetMeasurement(RealSensor::Instance());
#else
	GetMeasurement(MockedSensor::Instance());

#endif
}

void TempSensorManager::SetLocalTemperature(int16_t temperature)
{
	PlatformMgr().LockChipStack();
	app::Clusters::Thermostat::Attributes::LocalTemperature::Set(kThermostatEndpoint, temperature);
	PlatformMgr().UnlockChipStack();
}

template <typename T> void TempSensorManager::GetMeasurement(T const &sensor)
{
	sensor.Instance().TemperatureMeasurement();
}
