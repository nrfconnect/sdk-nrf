/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <app-common/zap-generated/attributes/Accessors.h>

#include <lib/core/CHIPError.h>

using namespace chip;

class TemperatureManager {
public:
	static TemperatureManager &Instance()
	{
		static TemperatureManager sTemperatureManager;
		return sTemperatureManager;
	};

	CHIP_ERROR Init();
	void AttributeChangeHandler(EndpointId endpointId, AttributeId attributeId, uint8_t *value, uint16_t size);
	app::DataModel::Nullable<int16_t> GetLocalTemp();
	app::DataModel::Nullable<int16_t> GetOutdoorTemp();

	void LogThermostatStatus();

private:
	app::DataModel::Nullable<int16_t> mLocalTempCelsius;
	app::DataModel::Nullable<int16_t> mOutdoorTempCelsius;
	int16_t mCoolingCelsiusSetPoint;
	int16_t mHeatingCelsiusSetPoint;
	uint8_t mThermMode;
};
