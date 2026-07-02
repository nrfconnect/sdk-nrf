/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief VTF provider framework sample
 */

#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <drivers/vtf_monitoring/vtf_monitoring.h>

LOG_MODULE_REGISTER(vtf_sample, CONFIG_LOG_DEFAULT_LEVEL);

int main(void)
{
	while (1) {
		union vtf_sample_value die_temp;
		union vtf_sample_value battery_voltage;

		vtf_monitoring_sample_get(VTF_CH_DIE_TEMP, &die_temp);
		vtf_monitoring_sample_get(VTF_CH_BATTERY_VOLTAGE, &battery_voltage);

		LOG_INF("die temp: %d.%02d C (tier 1, built-in sensor provider) | "
			"battery: %d mV (tier 2, custom mock provider)",
			die_temp.i32 / 100, abs(die_temp.i32 % 100), battery_voltage.i32);

		k_sleep(K_MSEC(CONFIG_VTF_SNAPSHOT_INTERVAL_MS));
	}
	return 0;
}
