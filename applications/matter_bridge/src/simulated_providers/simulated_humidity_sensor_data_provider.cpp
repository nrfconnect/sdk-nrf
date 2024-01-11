/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "simulated_humidity_sensor_data_provider.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::app;
using namespace Nrf;

void SimulatedHumiditySensorDataProvider::Init()
{
	k_timer_init(&mTimer, SimulatedHumiditySensorDataProvider::TimerTimeoutCallback, nullptr);
	k_timer_user_data_set(&mTimer, this);
	k_timer_start(&mTimer, K_MSEC(kMeasurementsIntervalMs), K_MSEC(kMeasurementsIntervalMs));
}

void SimulatedHumiditySensorDataProvider::NotifyUpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId,
							    void *data, size_t dataSize)
{
	if (mUpdateAttributeCallback) {
		mUpdateAttributeCallback(*this, Clusters::RelativeHumidityMeasurement::Id,
					 Clusters::RelativeHumidityMeasurement::Attributes::MeasuredValue::Id, data,
					 dataSize);
	}
}

CHIP_ERROR SimulatedHumiditySensorDataProvider::UpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId,
							    uint8_t *buffer)
{
	return CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE;
}

void SimulatedHumiditySensorDataProvider::TimerTimeoutCallback(k_timer *timer)
{
	if (!timer || !timer->user_data) {
		return;
	}

	SimulatedHumiditySensorDataProvider *provider =
		reinterpret_cast<SimulatedHumiditySensorDataProvider *>(timer->user_data);

	/* Get some random data to emulate sensor measurements. */
	provider->mHumidity =
		chip::Crypto::GetRandU16() % (kMaxRandomTemperature - kMinRandomTemperature) + kMinRandomTemperature;

	LOG_INF("SimulatedHumiditySensorDataProvider: Updated humidity value to %d", provider->mHumidity);

	DeviceLayer::PlatformMgr().ScheduleWork(NotifyAttributeChange, reinterpret_cast<intptr_t>(provider));
}

void SimulatedHumiditySensorDataProvider::NotifyAttributeChange(intptr_t context)
{
	SimulatedHumiditySensorDataProvider *provider =
		reinterpret_cast<SimulatedHumiditySensorDataProvider *>(context);

	provider->NotifyUpdateState(Clusters::RelativeHumidityMeasurement::Id,
				    Clusters::RelativeHumidityMeasurement::Attributes::MeasuredValue::Id,
				    &provider->mHumidity, sizeof(provider->mHumidity));
}
