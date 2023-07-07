/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "humidity_sensor_data_provider.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::app;

void HumiditySensorDataProvider::Init()
{
	k_timer_init(&mTimer, HumiditySensorDataProvider::TimerTimeoutCallback, nullptr);
	k_timer_user_data_set(&mTimer, this);
	k_timer_start(&mTimer, K_MSEC(kMeasurementsIntervalMs), K_MSEC(kMeasurementsIntervalMs));
}

void HumiditySensorDataProvider::NotifyUpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId, void *data,
						   size_t dataSize)
{
	if (mUpdateAttributeCallback) {
		mUpdateAttributeCallback(*this, Clusters::RelativeHumidityMeasurement::Id,
					 Clusters::RelativeHumidityMeasurement::Attributes::MeasuredValue::Id, data,
					 dataSize);
	}
}

CHIP_ERROR HumiditySensorDataProvider::UpdateState(chip::ClusterId clusterId, chip::AttributeId attributeId,
						   uint8_t *buffer)
{
	return CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE;
}

void HumiditySensorDataProvider::TimerTimeoutCallback(k_timer *timer)
{
	if (!timer || !timer->user_data) {
		return;
	}

	HumiditySensorDataProvider *provider = reinterpret_cast<HumiditySensorDataProvider *>(timer->user_data);

	/* Get some random data to emulate sensor measurements. */
	provider->mHumidity =
		chip::Crypto::GetRandU16() % (kMaxRandomTemperature - kMinRandomTemperature) + kMinRandomTemperature;

	LOG_INF("HumiditySensorDataProvider: Updated humidity value to %d", provider->mHumidity);

	DeviceLayer::PlatformMgr().ScheduleWork(NotifyAttributeChange, reinterpret_cast<intptr_t>(provider));
}

void HumiditySensorDataProvider::NotifyAttributeChange(intptr_t context)
{
	HumiditySensorDataProvider *provider = reinterpret_cast<HumiditySensorDataProvider *>(context);

	provider->NotifyUpdateState(Clusters::RelativeHumidityMeasurement::Id,
				    Clusters::RelativeHumidityMeasurement::Attributes::MeasuredValue::Id,
				    &provider->mHumidity, sizeof(provider->mHumidity));
}
