/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "simulated_temperature_sensor_data_provider.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::app;

void SimulatedTemperatureSensorDataProvider::Init()
{
	k_timer_init(&mTimer, SimulatedTemperatureSensorDataProvider::TimerTimeoutCallback, nullptr);
	k_timer_user_data_set(&mTimer, this);
	k_timer_start(&mTimer, K_MSEC(kMeasurementsIntervalMs), K_MSEC(kMeasurementsIntervalMs));
}

void SimulatedTemperatureSensorDataProvider::NotifyUpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId,
							       void *data, size_t dataSize)
{
	if (mUpdateAttributeCallback) {
		mUpdateAttributeCallback(*this, Clusters::TemperatureMeasurement::Id,
					 Clusters::TemperatureMeasurement::Attributes::MeasuredValue::Id, data,
					 dataSize);
	}
}

CHIP_ERROR SimulatedTemperatureSensorDataProvider::UpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId,
							       uint8_t *buffer)
{
	return CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE;
}

void SimulatedTemperatureSensorDataProvider::TimerTimeoutCallback(k_timer *timer)
{
	if (!timer || !timer->user_data) {
		return;
	}

	SimulatedTemperatureSensorDataProvider *provider =
		reinterpret_cast<SimulatedTemperatureSensorDataProvider *>(timer->user_data);

	/* Get some random data to emulate sensor measurements. */
	provider->mTemperature =
		chip::Crypto::GetRandU16() % (kMaxRandomTemperature - kMinRandomTemperature) + kMinRandomTemperature;

	LOG_INF("SimulatedTemperatureSensorDataProvider: Updated temperature value to %d", provider->mTemperature);

	DeviceLayer::PlatformMgr().ScheduleWork(NotifyAttributeChange, reinterpret_cast<intptr_t>(provider));
}

void SimulatedTemperatureSensorDataProvider::NotifyAttributeChange(intptr_t context)
{
	SimulatedTemperatureSensorDataProvider *provider =
		reinterpret_cast<SimulatedTemperatureSensorDataProvider *>(context);

	provider->NotifyUpdateState(Clusters::TemperatureMeasurement::Id,
				    Clusters::TemperatureMeasurement::Attributes::MeasuredValue::Id,
				    &provider->mTemperature, sizeof(provider->mTemperature));
}
