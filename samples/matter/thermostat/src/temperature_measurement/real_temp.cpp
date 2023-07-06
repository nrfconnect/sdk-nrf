/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "sensor.h"

#include <DeviceInfoProviderImpl.h>
#include <zephyr/logging/log.h>

using namespace ::chip;
using namespace ::chip::DeviceLayer;

LOG_MODULE_DECLARE(app);

namespace
{

constexpr chip::EndpointId mTemperatureMeasurementEndpointId = 1;

} // namespace

RealSensor::RealSensor()
{
	BindingHandler::GetInstance().Init();
}

void RealSensor::TemperatureMeasurement()
{
	BindingHandler::BindingData *data = Platform::New<BindingHandler::BindingData>();
	data->clusterId = Clusters::TemperatureMeasurement::Id;
	data->localEndpointId = mTemperatureMeasurementEndpointId;
	data->isGroup = BindingHandler::GetInstance().IsGroupBound();
	DeviceLayer::PlatformMgr().ScheduleWork(BindingHandler::ThermostatWorkerHandler,
						reinterpret_cast<intptr_t>(data));
}
