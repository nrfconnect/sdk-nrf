/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/npm2100_vbat.h>
#include <zephyr/settings/settings.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include "fuel_gauge.h"

#include <nrf_fuel_gauge.h>

#define SETTINGS_KEY_ROOT "fuel_gauge_state"

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
static const char *selected_battery_model_str;
static bool store_state;
static char settings_key[sizeof(SETTINGS_KEY_ROOT) + 128];
static uint8_t fuel_gauge_state_buf[1024];

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

int fuel_gauge_init(const struct device *vbat, enum battery_type battery, const char *batt_name)
{
	struct nrf_fuel_gauge_init_parameters parameters =
		NRF_FUEL_GAUGE_DEFAULT_INIT_PARAMETERS_PRIMARY(0.0f, 0.0f, 0.0f,
			&battery_models[battery]);
	int ret;


	printk("nRF Fuel Gauge version: %s\n", nrf_fuel_gauge_version);

	ret = read_sensors(vbat, &parameters.v0, &parameters.t0);
	if (ret < 0) {
		return ret;
	}

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		ssize_t len;

		ret = snprintf(
			settings_key, sizeof(settings_key), "%s/%s", SETTINGS_KEY_ROOT, batt_name);
		if (ret < 0 || ret >= sizeof(settings_key)) {
			printk("Error: Settings key format error\n");
			return -EINVAL;
		}

		len = settings_load_one(
			settings_key, fuel_gauge_state_buf, sizeof(fuel_gauge_state_buf));
		if (len == nrf_fuel_gauge_state_size) {
			parameters.state = fuel_gauge_state_buf;
			parameters.state_size = len;
			printk("Previous fuel gauge state loaded\n");
		} else {
			printk("No previous fuel gauge state found, starting fresh\n");
		}
	}

	ret = nrf_fuel_gauge_init(&parameters, NULL);
	if (ret < 0) {
		return ret;
	}

	ref_time = k_uptime_get();
	selected_battery_model = battery;
	selected_battery_model_str = batt_name;

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

	ret = nrf_fuel_gauge_process(
		voltage, battery_current[selected_battery_model], temp, delta, &soc, NULL);
	if (ret < 0) {
		printk("Error: Fuel gauge process failed\n");
		return ret;
	}

	printk("V: %.3f, T: %.2f, SoC: %.2f\n", (double)voltage, (double)temp, (double)soc);

	if (IS_ENABLED(CONFIG_SETTINGS) && store_state) {
		store_state = false;

		ret = nrf_fuel_gauge_state_get(fuel_gauge_state_buf, sizeof(fuel_gauge_state_buf));
		if (ret < 0) {
			printk("Error: Could not get fuel gauge state\n");
			return ret;
		}

		ret = settings_save_one(
			settings_key, fuel_gauge_state_buf, nrf_fuel_gauge_state_size);
		if (ret < 0) {
			printk("Error: Could not store fuel gauge state\n");
			return ret;
		}

		printk("Fuel gauge state stored\n");
	}

	return 0;
}

#if CONFIG_SHELL
static int cmd_fuel_gauge_state_store(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "Storing state after next update");
	store_state = true;

	return 0;
}


SHELL_COND_CMD_ARG_REGISTER(IS_ENABLED(CONFIG_SETTINGS), fuel_gauge_state_store, NULL,
	"Store battery state after next iteration", cmd_fuel_gauge_state_store, 1, 0);
#endif /* CONFIG_SHELL */
