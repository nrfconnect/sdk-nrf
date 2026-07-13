/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Wi-Fi recalibration monitor provider framework sample
 */

#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>

#include <platform_metrics.h>

LOG_MODULE_REGISTER(wifi_recal_sample, CONFIG_LOG_DEFAULT_LEVEL);

/*
 * XO frequency offset is a per-board-design constant, not a live channel:
 * read it straight from the Wi-Fi node's devicetree property, the same way
 * the Wi-Fi driver itself does, instead of adding a platform_metrics
 * channel for something that never changes at runtime.
 */
#define WIFI_NODE DT_CHOSEN(zephyr_wifi)
#define XO_FREQ_OFFSET_PPM DT_PROP(DT_PARENT(WIFI_NODE), nordic_wifi_xo_freq_offset)

int main(void)
{
	while (1) {
		union platform_metrics_sample_value die_temp;
		union platform_metrics_sample_value battery_voltage;

		platform_metrics_sample_get(PLATFORM_METRICS_CH_DIE_TEMP, &die_temp);
		platform_metrics_sample_get(PLATFORM_METRICS_CH_BATTERY_VOLTAGE, &battery_voltage);

		LOG_INF("die temp: %d.%02d C (tier 1, built-in sensor provider) | "
			"battery: %d mV (tier 2, custom mock provider) | "
			"XO offset: %d ppm (board devicetree constant)",
			die_temp.i32 / 100, abs(die_temp.i32 % 100), battery_voltage.i32,
			XO_FREQ_OFFSET_PPM);

		k_sleep(K_MSEC(CONFIG_PLATFORM_METRICS_SNAPSHOT_INTERVAL_MS));
	}
	return 0;
}
