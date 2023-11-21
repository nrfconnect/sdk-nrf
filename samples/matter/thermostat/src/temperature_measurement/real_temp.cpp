/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "sensor.h"

#include <DeviceInfoProviderImpl.h>
#include <zephyr/logging/log.h>

#include <app-common/zap-generated/ids/Attributes.h>

using namespace ::chip;
using namespace ::chip::DeviceLayer;
using namespace ::chip::app;

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
	data->ClusterId = Clusters::TemperatureMeasurement::Id;
	data->EndpointId = mTemperatureMeasurementEndpointId;
	data->IsGroup = BindingHandler::GetInstance().IsGroupBound();
	DeviceLayer::PlatformMgr().ScheduleWork(BindingHandler::DeviceWorkerHandler, reinterpret_cast<intptr_t>(data));
}
