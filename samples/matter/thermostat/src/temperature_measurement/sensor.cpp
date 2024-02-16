/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "sensor.h"
#include "temp_sensor_manager.h"

#include <stdint.h>

#include <temp_sensor_manager.h>

#include <app-common/zap-generated/ids/Attributes.h>
#include <controller/ReadInteraction.h>
#include <zephyr/logging/log.h>

using namespace ::chip;
using namespace ::chip::DeviceLayer;
using namespace ::chip::app;

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

namespace
{

constexpr chip::EndpointId mTemperatureMeasurementEndpointId = 1;
constexpr int16_t startingMockedValue = 1600;

constexpr uint16_t kSimulatedReadingFrequency = (60000 / kSensorTimerPeriodMs); /*Change simulated number every minute*/

#if CONFIG_THERMOSTAT_TEMPERATURE_STEP == 0
constexpr int16_t sMockTemp[] = { 2000, 2731, 1600, 2100, 1937, 3011, 1500, 1899 };
#endif

} // namespace

TemperatureSensor::TemperatureSensor()
{
	mMockTempIndex = 0;
	mPreviousTemperature = startingMockedValue;
	BindingHandler::Init();
}

void TemperatureSensor::FullMeasurement()
{
	InternalMeasurement();
	ExternalMeasurement();
}

void TemperatureSensor::InternalMeasurement()
{
	int16_t temperature;
#if CONFIG_THERMOSTAT_TEMPERATURE_STEP == 0

	temperature = sMockTemp[mMockTempIndex];

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
	int16_t nextValue = mPreviousTemperature + CONFIG_THERMOSTAT_TEMPERATURE_STEP;
#if CONFIG_THERMOSTAT_TEMPERATURE_STEP > 0
	temperature = (nextValue >= CONFIG_THERMOSTAT_SIMULATED_TEMPERATURE_MAX) ?
			      CONFIG_THERMOSTAT_SIMULATED_TEMPERATURE_MIN :
			      nextValue;
#elif CONFIG_THERMOSTAT_TEMPERATURE_STEP < 0
	temperature = (nextValue <= CONFIG_THERMOSTAT_SIMULATED_TEMPERATURE_MIN) ?
			      CONFIG_THERMOSTAT_SIMULATED_TEMPERATURE_MAX :
			      nextValue;
#endif
#endif

	mPreviousTemperature = temperature;

	TempSensorManager::Instance().SetLocalTemperature(temperature);
}

void TemperatureSensor::ExternalMeasurement()
{
	Nrf::Matter::BindingHandler::BindingData *data = Platform::New<Nrf::Matter::BindingHandler::BindingData>();
	data->ClusterId = Clusters::TemperatureMeasurement::Id;
	data->EndpointId = mTemperatureMeasurementEndpointId;
	data->InvokeCommandFunc = ExternalTemperatureMeasurementReadHandler;
	BindingHandler::RunBoundClusterAction(data);
}

void TemperatureSensor::ExternalTemperatureMeasurementReadHandler(const EmberBindingTableEntry &binding,
								  OperationalDeviceProxy *deviceProxy,
								  Nrf::Matter::BindingHandler::BindingData &bindingData)
{
	auto onSuccess = [dataPointer = Platform::New<Nrf::Matter::BindingHandler::BindingData>(bindingData)](
				 const ConcreteDataAttributePath &attributePath, const auto &dataResponse) {
		ChipLogProgress(NotSpecified, "Read Temperature Sensor attribute succeeded");

		VerifyOrReturn(!(dataResponse.IsNull()), LOG_ERR("Device invalid");
			       Platform::Delete<Nrf::Matter::BindingHandler::BindingData>(dataPointer););

		int16_t responseValue = dataResponse.Value();

		TempSensorManager::Instance().SetOutdoorTemperature(responseValue);
		BindingHandler::OnInvokeCommandSucces(dataPointer);
	};

	auto onFailure = [](const ConcreteDataAttributePath *attributePath, CHIP_ERROR error) {
		ChipLogError(NotSpecified, "Read Temperature Sensor attribute failed: %" CHIP_ERROR_FORMAT,
			     error.Format());
		TempSensorManager::Instance().ClearOutdoorTemperature();
	};

	VerifyOrReturn(deviceProxy != nullptr && deviceProxy->ConnectionReady(), LOG_ERR("Device invalid"));

	Controller::ReadAttribute<Clusters::TemperatureMeasurement::Attributes::MeasuredValue::TypeInfo>(
		deviceProxy->GetExchangeManager(), deviceProxy->GetSecureSession().Value(), binding.remote, onSuccess,
		onFailure);
}
