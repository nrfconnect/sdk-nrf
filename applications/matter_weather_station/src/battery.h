/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <cstdint>

int BatteryMeasurementInit();
int BatteryMeasurementEnable();
int32_t BatteryMeasurementReadVoltageMv();
int BatteryChargeControlInit();
bool BatteryCharged();
