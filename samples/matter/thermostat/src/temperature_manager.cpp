/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "temperature_manager.h"
#include "app_config.h"
#include "app_task.h"
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace chip;
using namespace ::chip::DeviceLayer;
using namespace ::chip::app::Clusters;

constexpr EndpointId kThermostatEndpoint = 1;

namespace
{

/* Convert and return only complete part of value to printable type */
uint8_t ReturnCompleteValue(int16_t Value)
{
	return static_cast<uint8_t>(Value / 100);
}

/* Converts and returns only reminder part of value to printable type.
 * This formula rounds reminder value to one significant figure
 */

uint8_t ReturnRemainderValue(int16_t Value)
{
	return static_cast<uint8_t>((Value % 100 + 5) / 10);
}

} // namespace

CHIP_ERROR TemperatureManager::Init()
{
	app::DataModel::Nullable<int16_t> temp;

	PlatformMgr().LockChipStack();
	Thermostat::Attributes::LocalTemperature::Get(kThermostatEndpoint, temp);
	Thermostat::Attributes::OccupiedCoolingSetpoint::Get(kThermostatEndpoint, &mCoolingCelsiusSetPoint);
	Thermostat::Attributes::OccupiedHeatingSetpoint::Get(kThermostatEndpoint, &mHeatingCelsiusSetPoint);
	Thermostat::Attributes::SystemMode::Get(kThermostatEndpoint, &mThermMode);
	PlatformMgr().UnlockChipStack();

	mCurrentTempCelsius = temp.Value();

	LogThermostatStatus();

	return CHIP_NO_ERROR;
}

void TemperatureManager::LogThermostatStatus()
{
	LOG_INF("Thermostat:");
	LOG_INF("Mode - %d", GetMode());
	LOG_INF("Temperature - %d,%d'C", ReturnCompleteValue(GetCurrentTemp()), ReturnRemainderValue(GetCurrentTemp()));
	LOG_INF("HeatingSetpoint - %d,%d'C", ReturnCompleteValue(GetHeatingSetPoint()),
		ReturnRemainderValue(GetHeatingSetPoint()));
	LOG_INF("CoolingSetpoint - %d,%d'C \n", ReturnCompleteValue(GetCoolingSetPoint()),
		ReturnRemainderValue(GetCoolingSetPoint()));
}

void TemperatureManager::AttributeChangeHandler(EndpointId endpointId, AttributeId attributeId, uint8_t *value,
						uint16_t size)
{
	switch (attributeId) {
	case Thermostat::Attributes::LocalTemperature::Id: {
		int16_t temp = *reinterpret_cast<int16_t *>(value);
		mCurrentTempCelsius = temp;
	} break;

	case Thermostat::Attributes::OccupiedCoolingSetpoint::Id: {
		int16_t coolingTemp = *reinterpret_cast<int16_t *>(value);
		mCoolingCelsiusSetPoint = coolingTemp;
		LOG_INF("Cooling TEMP: %d", mCoolingCelsiusSetPoint);
	} break;

	case Thermostat::Attributes::OccupiedHeatingSetpoint::Id: {
		int16_t heatingTemp = *reinterpret_cast<int16_t *>(value);
		mHeatingCelsiusSetPoint = heatingTemp;
		LOG_INF("Heating TEMP %d", mHeatingCelsiusSetPoint);
	} break;

	case Thermostat::Attributes::SystemMode::Id: {
		uint8_t mThermMode = (*value);
	} break;

	default: {
		LOG_INF("Unhandled thermostat attribute %x", attributeId);
		return;
	} break;
	}

	LogThermostatStatus();
}

const uint8_t TemperatureManager::GetMode()
{
	return mThermMode;
}

const uint16_t TemperatureManager::GetCurrentTemp()
{
	return mCurrentTempCelsius;
}
const uint16_t TemperatureManager::GetHeatingSetPoint()
{
	return mHeatingCelsiusSetPoint;
}

const uint16_t TemperatureManager::GetCoolingSetPoint()
{
	return mCoolingCelsiusSetPoint;
}
