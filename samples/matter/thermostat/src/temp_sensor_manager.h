/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdbool.h>
#include <stdint.h>

#include <app-common/zap-generated/attributes/Accessors.h>
#include <lib/core/CHIPError.h>

using namespace chip;

constexpr uint16_t kSensorTimerPeriodMs = 30000; /* 30s timer period*/

class TempSensorManager {
public:
	CHIP_ERROR Init();

	static TempSensorManager &Instance()
	{
		static TempSensorManager sTempSensorManager;
		return sTempSensorManager;
	};

	static void SetLocalTemperature(int16_t temperature);

private:
	template <typename T> static void GetMeasurement(T const &sensor);
	static void TimerEventHandler(k_timer *timer);
	static void SensorTimerEventHandler();
};
