/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "temperature_sensor_data_provider.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::app;

void TemperatureSensorDataProvider::Init()
{
	k_timer_init(&mTimer, TemperatureSensorDataProvider::TimerTimeoutCallback, nullptr);
	k_timer_user_data_set(&mTimer, this);
	k_timer_start(&mTimer, K_MSEC(kMeasurementsIntervalMs), K_MSEC(kMeasurementsIntervalMs));
}

void TemperatureSensorDataProvider::NotifyUpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId,
						      void *data, size_t dataSize)
{
	if (mUpdateAttributeCallback) {
		mUpdateAttributeCallback(*this, Clusters::TemperatureMeasurement::Id,
					 Clusters::TemperatureMeasurement::Attributes::MeasuredValue::Id, data,
					 dataSize);
	}
}

CHIP_ERROR TemperatureSensorDataProvider::UpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId,
						      uint8_t *buffer)
{
	return CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE;
}

void TemperatureSensorDataProvider::TimerTimeoutCallback(k_timer *timer)
{
	if (!timer || !timer->user_data) {
		return;
	}

	TemperatureSensorDataProvider *provider = reinterpret_cast<TemperatureSensorDataProvider *>(timer->user_data);

	/* Get some random data to emulate sensor measurements. */
	provider->mTemperature =
		chip::Crypto::GetRandU16() % (kMaxRandomTemperature - kMinRandomTemperature) + kMinRandomTemperature;

	LOG_INF("TemperatureSensorDataProvider: Updated temperature value to %d", provider->mTemperature);

	DeviceLayer::PlatformMgr().ScheduleWork(NotifyAttributeChange, reinterpret_cast<intptr_t>(provider));
}

void TemperatureSensorDataProvider::NotifyAttributeChange(intptr_t context)
{
	TemperatureSensorDataProvider *provider = reinterpret_cast<TemperatureSensorDataProvider *>(context);

	provider->NotifyUpdateState(Clusters::TemperatureMeasurement::Id,
				    Clusters::TemperatureMeasurement::Attributes::MeasuredValue::Id,
				    &provider->mTemperature, sizeof(provider->mTemperature));
}
