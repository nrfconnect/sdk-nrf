/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief VTF monitoring framework sample
 */

#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <vtf_monitoring/vtf_monitoring.h>

LOG_MODULE_REGISTER(vtf_monitoring_sample, CONFIG_LOG_DEFAULT_LEVEL);

#define SLEEP_TIME_MS 1000

#if DT_HAS_CHOSEN(nordic_vtf_region)
#define VTF_NODE DT_CHOSEN(nordic_vtf_region)
#else
#error " 'nordic,vtf-region' must be chosen and enabled"
#endif

int main(void)
{
	while (1) {
		int32_t temp = vtf_snapshots[VTF_CH_DIE_TEMP].i32;
		int32_t batt = vtf_snapshots[VTF_CH_BATTERY_VOLTAGE].i32;
		int32_t xo = vtf_snapshots[VTF_CH_FREQ_OFFSET].i32;

		LOG_INF("die temp: %d.%02d C", temp / 100, abs(temp % 100));
		LOG_INF("battery voltage: %d mV", batt);
		LOG_INF("XO offset: %d ppm", xo);

		k_msleep(SLEEP_TIME_MS);
	}
	return 0;
}
