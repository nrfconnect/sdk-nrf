/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/npm1300_charger.h>
#include <zephyr/sys/printk.h>

#include "nrf_fuel_gauge.h"

static float max_charge_current;
static float term_charge_current;
static int64_t ref_time;

static const struct battery_model battery_model = {
#include "battery_model.inc"
};

static int read_sensors(const struct device *charger, float *voltage, float *current, float *temp)
{
	struct sensor_value value;
	int ret;

	ret = sensor_sample_fetch(charger);
	if (ret < 0) {
		return ret;
	}

	sensor_channel_get(charger, SENSOR_CHAN_GAUGE_VOLTAGE, &value);
	*voltage = (float)value.val1 + ((float)value.val2 / 1000000);

	sensor_channel_get(charger, SENSOR_CHAN_GAUGE_TEMP, &value);
	*temp = (float)value.val1 + ((float)value.val2 / 1000000);

	sensor_channel_get(charger, SENSOR_CHAN_GAUGE_AVG_CURRENT, &value);
	*current = (float)value.val1 + ((float)value.val2 / 1000000);

	return 0;
}

int fuel_gauge_init(const struct device *charger)
{
	struct sensor_value value;
	struct nrf_fuel_gauge_init_parameters parameters = { .model = &battery_model };
	int ret;

	ret = read_sensors(charger, &parameters.v0, &parameters.i0, &parameters.t0);
	if (ret < 0) {
		return ret;
	}

	/* Store charge nominal and termination current, needed for ttf calculation */
	sensor_channel_get(charger, SENSOR_CHAN_GAUGE_DESIRED_CHARGING_CURRENT, &value);
	max_charge_current = (float)value.val1 + ((float)value.val2 / 1000000);
	term_charge_current = max_charge_current / 10.f;

	nrf_fuel_gauge_init(&parameters, NULL);

	ref_time = k_uptime_get();

	return 0;
}

int fuel_gauge_update(const struct device *charger)
{
	float voltage;
	float current;
	float temp;
	float soc;
	float tte;
	float ttf;
	float delta;
	int ret;

	ret = read_sensors(charger, &voltage, &current, &temp);
	if (ret < 0) {
		printk("Error: Could not read from charger device\n");
		return ret;
	}

	delta = (float) k_uptime_delta(&ref_time) / 1000.f;

	soc = nrf_fuel_gauge_process(voltage, current, temp, delta, NULL);
	tte = nrf_fuel_gauge_tte_get();
	ttf = nrf_fuel_gauge_ttf_get(-max_charge_current, -term_charge_current);

	printk("V: %.3f, I: %.3f, T: %.2f, ", voltage, current, temp);
	printk("SoC: %.2f, TTE: %.0f, TTF: %.0f\n", soc, tte, ttf);

	return 0;
}
