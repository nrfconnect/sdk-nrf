/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/npm2100_vbat.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include "fuel_gauge.h"

#include <nrf_fuel_gauge.h>

static int64_t ref_time;

static const struct battery_model_primary battery_models[] = {
	[BATTERY_TYPE_ALKALINE_AA] = {
		#include <battery_models/primary_cell/AA_Alkaline.inc>
	},
	[BATTERY_TYPE_ALKALINE_AAA] = {
		#include <battery_models/primary_cell/AAA_Alkaline.inc>
	},
	[BATTERY_TYPE_ALKALINE_2SAA] = {
		#include <battery_models/primary_cell/2SAA_Alkaline.inc>
	},
	[BATTERY_TYPE_ALKALINE_2SAAA] = {
		#include <battery_models/primary_cell/2SAAA_Alkaline.inc>
	},
	[BATTERY_TYPE_ALKALINE_LR44] = {
		#include <battery_models/primary_cell/LR44.inc>
	},
	[BATTERY_TYPE_LITHIUM_CR2032] = {
		#include <battery_models/primary_cell/CR2032.inc>
	},
};

/* Basic assumption of average battery current.
 * Using a non-zero value improves the fuel gauge accuracy, even if the number is not exact.
 */
static const float battery_current[] = {
	[BATTERY_TYPE_ALKALINE_AA] = 5e-3f,
	[BATTERY_TYPE_ALKALINE_AAA] = 5e-3f,
	[BATTERY_TYPE_ALKALINE_2SAA] = 5e-3f,
	[BATTERY_TYPE_ALKALINE_2SAAA] = 5e-3f,
	[BATTERY_TYPE_ALKALINE_LR44] = 1.5e-3f,
	[BATTERY_TYPE_LITHIUM_CR2032] = 1.5e-3f,
};

static enum battery_type selected_battery_model;

static int read_sensors(const struct device *vbat, float *voltage, float *temp)
{
	struct sensor_value value;
	int ret;

	ret = sensor_sample_fetch(vbat);
	if (ret < 0) {
		return ret;
	}

	sensor_channel_get(vbat, SENSOR_CHAN_GAUGE_VOLTAGE, &value);
	*voltage = (float)value.val1 + ((float)value.val2 / 1000000);

	sensor_channel_get(vbat, SENSOR_CHAN_DIE_TEMP, &value);
	*temp = (float)value.val1 + ((float)value.val2 / 1000000);

	return 0;
}

int fuel_gauge_init(const struct device *vbat, enum battery_type battery)
{
	struct nrf_fuel_gauge_init_parameters parameters = {
		.model_primary = &battery_models[battery],
		.i0 = 0.0f,
		.opt_params = NULL,
	};
	int ret;

	printk("nRF Fuel Gauge version: %s\n", nrf_fuel_gauge_version);

	ret = read_sensors(vbat, &parameters.v0, &parameters.t0);
	if (ret < 0) {
		return ret;
	}

	ret = nrf_fuel_gauge_init(&parameters, NULL);
	if (ret < 0) {
		return ret;
	}

	ref_time = k_uptime_get();
	selected_battery_model = battery;

	return 0;
}

int fuel_gauge_update(const struct device *vbat)
{
	float voltage;
	float temp;
	float soc;
	float delta;
	int ret;

	ret = read_sensors(vbat, &voltage, &temp);
	if (ret < 0) {
		printk("Error: Could not read from vbat device\n");
		return ret;
	}

	delta = (float)k_uptime_delta(&ref_time) / 1000.f;

	soc = nrf_fuel_gauge_process(
		voltage, battery_current[selected_battery_model], temp, delta, NULL);

	printk("V: %.3f, T: %.2f, SoC: %.2f\n", (double)voltage, (double)temp, (double)soc);

	return 0;
}
