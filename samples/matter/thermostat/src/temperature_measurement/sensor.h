/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#pragma once

#include "binding/binding_handler.h"

#include <platform/CHIPDeviceLayer.h>

using namespace ::Nrf::Matter;
class TemperatureSensor {
public:
	TemperatureSensor();

	static TemperatureSensor &Instance()
	{
		static TemperatureSensor sSensor;
		return sSensor;
	};

	void FullMeasurement();
	void ExternalMeasurement();
	void InternalMeasurement();

private:
	static void ExternalTemperatureMeasurementReadHandler(const EmberBindingTableEntry &binding,
							      chip::OperationalDeviceProxy *deviceProxy,
							      BindingHandler::BindingData &bindingData);

	int16_t mPreviousTemperature;
	uint8_t mMockTempIndex;
	uint8_t mCycleCounter;
};
