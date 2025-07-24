/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/npm13xx_charger.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include <nrf_fuel_gauge.h>

/* nPM13xx CHARGER.BCHGCHARGESTATUS register bitmasks */
#define NPM13XX_CHG_STATUS_COMPLETE_MASK BIT(1)
#define NPM13XX_CHG_STATUS_TRICKLE_MASK  BIT(2)
#define NPM13XX_CHG_STATUS_CC_MASK       BIT(3)
#define NPM13XX_CHG_STATUS_CV_MASK       BIT(4)

static int64_t ref_time;

static const struct battery_model battery_model = {
#if DT_NODE_EXISTS(DT_NODELABEL(npm1300_ek_pmic))
#include "battery_model.inc"
#elif DT_NODE_EXISTS(DT_NODELABEL(npm1304_ek_pmic))
#include "battery_model_20mAh.inc"
#endif
};

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
		state_info.charge_state = NRF_FUEL_GAUGE_CHARGE_STATE_CC;
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
	struct nrf_fuel_gauge_init_parameters parameters = {
		.model = &battery_model,
		.opt_params = NULL,
		.state = NULL,
	};
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

	float voltage;
	float current;
	float temp;
	float soc;
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
	/* Zephyr sensor API convention for Gauge current is negative=discharging,
	 * while nrf_fuel_gauge lib expects the opposite negative=charging
	 */
	current = -current;

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
	soc = nrf_fuel_gauge_process(voltage, -current, temp, delta, NULL);
	tte = nrf_fuel_gauge_tte_get();
	ttf = nrf_fuel_gauge_ttf_get();

	printk("V: %.3f, I: %.3f, T: %.2f, ", (double)voltage, (double)current, (double)temp);
	printk("SoC: %.2f, TTE: %.0f, TTF: %.0f\n", (double)soc, (double)tte, (double)ttf);

	return 0;
}
