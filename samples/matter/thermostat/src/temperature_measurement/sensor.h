/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "binding/binding_handler.h"

#include <platform/CHIPDeviceLayer.h>

#pragma once

class MockedSensor {
public:
	static MockedSensor &Instance()
	{
		static MockedSensor sSensor;
		return sSensor;
	};

	void TemperatureMeasurement();

private:
	int16_t mPreviousTemperature;
	uint8_t mMockTempIndex;
	uint8_t mCycleCounter;
};

class RealSensor {
public:
	RealSensor();

	static RealSensor &Instance()
	{
		static RealSensor sSensor;
		return sSensor;
	};

	void TemperatureMeasurement();

private:
	static void TemperatureMeasurementReadHandler(const EmberBindingTableEntry &binding,
						      chip::OperationalDeviceProxy *deviceProxy,
						      BindingHandler::BindingData &bindingData);
};
