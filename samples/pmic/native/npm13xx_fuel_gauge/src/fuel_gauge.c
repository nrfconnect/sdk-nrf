/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <math.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/npm13xx_charger.h>
#include <zephyr/settings/settings.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include <nrf_fuel_gauge.h>

#define SETTINGS_KEY "fuel_gauge_state"

/* nPM13xx CHARGER.BCHGCHARGESTATUS register bitmasks */
#define NPM13XX_CHG_STATUS_COMPLETE_MASK BIT(1)
#define NPM13XX_CHG_STATUS_TRICKLE_MASK	 BIT(2)
#define NPM13XX_CHG_STATUS_CC_MASK	 BIT(3)
#define NPM13XX_CHG_STATUS_CV_MASK	 BIT(4)

static int64_t ref_time;

static const struct battery_model battery_model = {
#if DT_NODE_EXISTS(DT_NODELABEL(npm1300_ek_pmic))
#include "battery_model.inc"
#elif DT_NODE_EXISTS(DT_NODELABEL(npm1304_ek_pmic))
#include "battery_model_20mAh.inc"
#endif
};

static bool store_state;
static uint8_t fuel_gauge_state_buf[1024];

static int read_sensors(const struct device *charger, float *voltage, float *current, float *temp,
			int32_t *chg_status)
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

	sensor_channel_get(charger, SENSOR_CHAN_NPM13XX_CHARGER_STATUS, &value);
	*chg_status = value.val1;

	return 0;
}

static int charge_status_inform(int32_t chg_status)
{
	union nrf_fuel_gauge_ext_state_info_data state_info;

	if (chg_status & NPM13XX_CHG_STATUS_COMPLETE_MASK) {
		printk("Charge complete\n");
		state_info.charge_state = NRF_FUEL_GAUGE_CHARGE_STATE_COMPLETE;
	} else if (chg_status & NPM13XX_CHG_STATUS_TRICKLE_MASK) {
		printk("Trickle charging\n");
		state_info.charge_state = NRF_FUEL_GAUGE_CHARGE_STATE_TRICKLE;
	} else if (chg_status & NPM13XX_CHG_STATUS_CC_MASK) {
		printk("Constant current charging\n");
		/* Using NRF_FUEL_GAUGE_CHARGE_STATE_CC_LIMITED instead of
		 * NRF_FUEL_GAUGE_CHARGE_STATE_CC because the charge current may be limited. This
		 * can be confirmed by checking the VBUS status register, etc.
		 */
		state_info.charge_state = NRF_FUEL_GAUGE_CHARGE_STATE_CC_LIMITED;
	} else if (chg_status & NPM13XX_CHG_STATUS_CV_MASK) {
		printk("Constant voltage charging\n");
		state_info.charge_state = NRF_FUEL_GAUGE_CHARGE_STATE_CV;
	} else {
		printk("Charger idle\n");
		state_info.charge_state = NRF_FUEL_GAUGE_CHARGE_STATE_IDLE;
	}

	return nrf_fuel_gauge_ext_state_update(NRF_FUEL_GAUGE_EXT_STATE_INFO_CHARGE_STATE_CHANGE,
					       &state_info);
}

int fuel_gauge_init(const struct device *charger)
{
	struct sensor_value value;
	struct nrf_fuel_gauge_init_parameters parameters =
		NRF_FUEL_GAUGE_DEFAULT_INIT_PARAMETERS_SECONDARY(0.0f, 0.0f, 0.0f, &battery_model);
	float max_charge_current;
	float term_charge_current;
	int32_t chg_status;
	int ret;

	printk("nRF Fuel Gauge version: %s\n", nrf_fuel_gauge_version);

	ret = read_sensors(charger, &parameters.v0, &parameters.i0, &parameters.t0, &chg_status);
	if (ret < 0) {
		return ret;
	}
	/* Zephyr sensor API convention for Gauge current is negative=discharging,
	 * while nrf_fuel_gauge lib expects the opposite negative=charging
	 */
	parameters.i0 = -parameters.i0;

	/* Store charge nominal and termination current, needed for ttf calculation */
	sensor_channel_get(charger, SENSOR_CHAN_GAUGE_DESIRED_CHARGING_CURRENT, &value);
	max_charge_current = (float)value.val1 + ((float)value.val2 / 1000000);
	term_charge_current = max_charge_current / 10.f;

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		ssize_t len;

		len = settings_load_one(SETTINGS_KEY, fuel_gauge_state_buf,
					sizeof(fuel_gauge_state_buf));
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
		printk("Error: Could not initialise fuel gauge\n");
		return ret;
	}

	ret = nrf_fuel_gauge_ext_state_update(NRF_FUEL_GAUGE_EXT_STATE_INFO_CHARGE_CURRENT_LIMIT,
					      &(union nrf_fuel_gauge_ext_state_info_data){
						      .charge_current_limit = max_charge_current});
	if (ret < 0) {
		printk("Error: Could not set fuel gauge state\n");
		return ret;
	}

	ret = nrf_fuel_gauge_ext_state_update(NRF_FUEL_GAUGE_EXT_STATE_INFO_TERM_CURRENT,
					      &(union nrf_fuel_gauge_ext_state_info_data){
						      .charge_term_current = term_charge_current});
	if (ret < 0) {
		printk("Error: Could not set fuel gauge state\n");
		return ret;
	}

	ret = charge_status_inform(chg_status);
	if (ret < 0) {
		printk("Error: Could not set fuel gauge state\n");
		return ret;
	}

	ref_time = k_uptime_get();

	return 0;
}

int fuel_gauge_update(const struct device *charger, bool vbus_connected)
{
	static int32_t chg_status_prev;

	struct nrf_fuel_gauge_state_info state_info;
	float voltage;
	float current;
	float temp;
	float soc;
	float soh;
	float tte;
	float ttf;
	float delta;
	int32_t chg_status;
	int ret;

	ret = read_sensors(charger, &voltage, &current, &temp, &chg_status);
	if (ret < 0) {
		printk("Error: Could not read from charger device\n");
		return ret;
	}

	ret = nrf_fuel_gauge_ext_state_update(
		vbus_connected ? NRF_FUEL_GAUGE_EXT_STATE_INFO_VBUS_CONNECTED
			       : NRF_FUEL_GAUGE_EXT_STATE_INFO_VBUS_DISCONNECTED,
		NULL);
	if (ret < 0) {
		printk("Error: Could not inform of state\n");
		return ret;
	}

	if (chg_status != chg_status_prev) {
		chg_status_prev = chg_status;

		ret = charge_status_inform(chg_status);
		if (ret < 0) {
			printk("Error: Could not inform of charge status\n");
			return ret;
		}
	}

	delta = (float)k_uptime_delta(&ref_time) / 1000.f;

	/* Zephyr sensor API convention for Gauge current is negative=discharging,
	 * while nrf_fuel_gauge lib expects the opposite negative=charging
	 */
	current = -current;
	ret = nrf_fuel_gauge_process(voltage, current, temp, delta, &soc, &state_info);
	if (ret < 0) {
		printk("Error: Fuel gauge process failed\n");
		return ret;
	}
	ret = nrf_fuel_gauge_soh_get(&soh);
	if (ret < 0) {
		printk("Error: Could not get SOH\n");
	}
	if (current >= 0.0f) {
		/* Discharging */
		ret = nrf_fuel_gauge_tte_get(&tte);
		if (ret < 0) {
			printk("Error: Could not get TTE\n");
		}
		ttf = NAN;
	} else {
		/* Charging */
		ret = nrf_fuel_gauge_ttf_get(&ttf);
		if (ret < 0) {
			printk("Error: Could not get TTE\n");
		}
		tte = NAN;
	}

	printk("V: %.3f, I: %.3f, T: %.2f, SoC: %.2f, TTE: %.0f, TTF: %.0f, SoH: %.2f, Cycles: %d, "
	       "Chg: %.2f\n",
	       (double)voltage, (double)current, (double)temp, (double)soc, (double)tte,
	       (double)ttf, (double)soh, state_info.cycle_count,
	       (double)state_info.cycle_count_cumulative_mah);

	if (IS_ENABLED(CONFIG_SETTINGS) && store_state) {
		store_state = false;

		ret = nrf_fuel_gauge_state_get(fuel_gauge_state_buf, sizeof(fuel_gauge_state_buf));
		if (ret < 0) {
			printk("Error: Could not get fuel gauge state\n");
			return ret;
		}

		ret = settings_save_one(SETTINGS_KEY, fuel_gauge_state_buf,
					nrf_fuel_gauge_state_size);
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
			    "Store battery state after next iteration", cmd_fuel_gauge_state_store,
			    1, 0);
#endif /* CONFIG_SHELL */
