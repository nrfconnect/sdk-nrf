/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <math.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/charger.h>
#include <zephyr/drivers/charger/npm10xx.h>
#include <zephyr/settings/settings.h>
#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>
#include <nrf_fuel_gauge.h>
#include "fuel_gauge.h"

LOG_MODULE_REGISTER(fuel_gauge, CONFIG_LOG_DEFAULT_LEVEL);

#define SETTINGS_KEY "fuel_gauge_state"

static const uint8_t iterm_div[] = {8, 1, 2, 4, 16, 32, 64, 128};
static const char * const chg_states[] = {"Charger idle",
				   "Trickle charging",
				   "Constant current charging",
				   "Constant current charging",
				   "Constant voltage charging",
				   "Charge complete"};

static int64_t ref_time;
static int32_t vterm_uv;

static atomic_t store_state;
static uint8_t fuel_gauge_state_buf[1024];

static enum nrf_fuel_gauge_charge_state chg_state_prev;

static const struct battery_model battery_model = {
#include "battery_model_20mAh.inc"
};

static enum nrf_fuel_gauge_charge_state
translate_charge_state(enum charger_status status, enum charger_charge_type type, int32_t vbat_uv)
{
	static enum nrf_fuel_gauge_charge_state state;

	if (status == CHARGER_STATUS_FULL) {
		state = NRF_FUEL_GAUGE_CHARGE_STATE_COMPLETE;

	} else if (status == CHARGER_STATUS_CHARGING) {
		switch (type) {
		case CHARGER_CHARGE_TYPE_TRICKLE:
			state = NRF_FUEL_GAUGE_CHARGE_STATE_TRICKLE;
			break;
		case CHARGER_CHARGE_TYPE_STANDARD:
			state = NRF_FUEL_GAUGE_CHARGE_STATE_CV;
			break;
		case CHARGER_CHARGE_TYPE_FAST:
			state = NRF_FUEL_GAUGE_CHARGE_STATE_CC;
			break;
		case CHARGER_CHARGE_TYPE_LONGLIFE:
			state = NRF_FUEL_GAUGE_CHARGE_STATE_CC_LIMITED;
			break;

		default:
			LOG_ERR("Wrong charge type %d", type);
		}

	} else {
		state = NRF_FUEL_GAUGE_CHARGE_STATE_IDLE;
	}

	return state;
}

static int read_sensors(const struct device *adc, const struct device *charger, float *voltage,
			float *current, float *temp, enum nrf_fuel_gauge_charge_state *state)
{
	struct sensor_value value;
	union charger_propval prop_status, prop_type;
	int32_t uv;
	int ret;

	ret = sensor_sample_fetch(adc);
	if (ret < 0) {
		return ret;
	}

	sensor_channel_get(adc, SENSOR_CHAN_GAUGE_VOLTAGE, &value);
	*voltage = sensor_value_to_float(&value);
	uv = (int32_t)sensor_value_to_micro(&value);

	sensor_channel_get(adc, SENSOR_CHAN_GAUGE_TEMP, &value);
	*temp = sensor_value_to_float(&value);

	sensor_channel_get(adc, SENSOR_CHAN_GAUGE_AVG_CURRENT, &value);
	*current = sensor_value_to_float(&value);

	ret = charger_get_prop(charger, CHARGER_PROP_STATUS, &prop_status);
	if (ret < 0) {
		return ret;
	}

	ret = charger_get_prop(charger, CHARGER_PROP_CHARGE_TYPE, &prop_type);
	if (ret < 0) {
		return ret;
	}

	*state = translate_charge_state(prop_status.status, prop_type.charge_type, uv);

	return 0;
}

static int charge_status_inform(enum nrf_fuel_gauge_charge_state state)
{
	union nrf_fuel_gauge_ext_state_info_data state_info = {.charge_state = state};

	LOG_INF("%s", chg_states[state]);

	return nrf_fuel_gauge_ext_state_update(NRF_FUEL_GAUGE_EXT_STATE_INFO_CHARGE_STATE_CHANGE,
					       &state_info);
}

int fuel_gauge_init(const struct device *adc, const struct device *charger)
{
	struct nrf_fuel_gauge_init_parameters parameters =
		NRF_FUEL_GAUGE_DEFAULT_INIT_PARAMETERS_SECONDARY(0.0f, 0.0f, 0.0f, &battery_model);
	union nrf_fuel_gauge_ext_state_info_data max_charge_current;
	union nrf_fuel_gauge_ext_state_info_data term_charge_current;
	union charger_propval chg_prop;
	enum nrf_fuel_gauge_charge_state chg_state;
	int ret;

	/* Store termination voltage to estimate CV state */
	ret = charger_get_prop(charger, CHARGER_PROP_CONSTANT_CHARGE_VOLTAGE_UV, &chg_prop);
	if (ret < 0) {
		return ret;
	}
	vterm_uv = chg_prop.const_charge_voltage_uv;

	/* Store charge nominal and termination current, needed for ttf calculation */
	ret = charger_get_prop(charger, CHARGER_PROP_CONSTANT_CHARGE_CURRENT_UA, &chg_prop);
	if (ret < 0) {
		return ret;
	}
	max_charge_current.charge_current_limit =
		(float)(chg_prop.const_charge_current_ua) / 1000000;
	ret = charger_get_prop(charger, NPM10XX_CHARGER_PROP_TERM_CURRENT, &chg_prop);
	if (ret < 0) {
		return ret;
	}
	term_charge_current.charge_term_current =
		max_charge_current.charge_current_limit / iterm_div[chg_prop.custom_uint];

	LOG_INF("nRF Fuel Gauge version: %s", nrf_fuel_gauge_version);

	ret = read_sensors(adc, charger, &parameters.v0, &parameters.i0, &parameters.t0,
			   &chg_state);
	if (ret < 0) {
		return ret;
	}
	/* Zephyr sensor API convention for Gauge current is negative=discharging,
	 * while nrf_fuel_gauge lib expects the opposite negative=charging
	 */
	parameters.i0 = -parameters.i0;

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		ssize_t len;

		len = settings_load_one(SETTINGS_KEY, fuel_gauge_state_buf,
					sizeof(fuel_gauge_state_buf));
		if (len == nrf_fuel_gauge_state_size &&
			nrf_fuel_gauge_state_compatible_check(fuel_gauge_state_buf, len) == 0) {
			parameters.state = fuel_gauge_state_buf;
			parameters.state_size = len;
			LOG_INF("Previous fuel gauge state loaded");
		} else {
			LOG_INF("No previous fuel gauge state found, starting fresh");
		}
	}

	ret = nrf_fuel_gauge_init(&parameters, NULL);
	if (ret < 0) {
		LOG_ERR("Could not initialise fuel gauge (err %d)", ret);
		return ret;
	}

	ret = nrf_fuel_gauge_ext_state_update(NRF_FUEL_GAUGE_EXT_STATE_INFO_CHARGE_CURRENT_LIMIT,
					      &max_charge_current);
	if (ret < 0) {
		LOG_ERR("Could not set fuel gauge state (err %d)", ret);
		return ret;
	}

	ret = nrf_fuel_gauge_ext_state_update(NRF_FUEL_GAUGE_EXT_STATE_INFO_TERM_CURRENT,
					      &term_charge_current);
	if (ret < 0) {
		LOG_ERR("Could not set fuel gauge state (err %d)", ret);
		return ret;
	}

	ret = charge_status_inform(chg_state);
	if (ret < 0) {
		LOG_ERR("Could not set fuel gauge state (err %d)", ret);
		return ret;
	}

	ref_time = k_uptime_get();
	chg_state_prev = chg_state;

	return 0;
}

int fuel_gauge_update(const struct device *adc, const struct device *charger, bool vbus_connected)
{
	struct nrf_fuel_gauge_state_info state_info;
	enum nrf_fuel_gauge_charge_state chg_state;
	float voltage;
	float current;
	float temp;
	float soc;
	float soh;
	float tte;
	float ttf;
	float delta;
	int ret;

	ret = read_sensors(adc, charger, &voltage, &current, &temp, &chg_state);
	if (ret < 0) {
		LOG_ERR("Could not read from adc device (err %d)", ret);
		return ret;
	}

	ret = nrf_fuel_gauge_ext_state_update(
		vbus_connected ? NRF_FUEL_GAUGE_EXT_STATE_INFO_VBUS_CONNECTED
			       : NRF_FUEL_GAUGE_EXT_STATE_INFO_VBUS_DISCONNECTED,
		NULL);
	if (ret < 0) {
		LOG_ERR("Could not inform of state (err %d)", ret);
		return ret;
	}

	if (chg_state != chg_state_prev) {
		chg_state_prev = chg_state;

		ret = charge_status_inform(chg_state);
		if (ret < 0) {
			LOG_ERR("Could not inform of charge status (err %d)", ret);
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
		LOG_ERR("Fuel gauge process failed (err %d)", ret);
		return ret;
	}
	ret = nrf_fuel_gauge_soh_get(&soh);
	if (ret < 0) {
		LOG_ERR("Could not get SOH (err %d)", ret);
	}
	if (current >= 0.0f) {
		/* Discharging */
		ret = nrf_fuel_gauge_tte_get(&tte);
		if (ret < 0) {
			LOG_ERR("Could not get TTE (err %d)", ret);
		}
		ttf = NAN;
	} else {
		/* Charging */
		ret = nrf_fuel_gauge_ttf_get(&ttf);
		if (ret < 0) {
			LOG_ERR("Could not get TTE (err %d)", ret);
		}
		tte = NAN;
	}

	/* Current is displayed according to Zephyr sign convention */
	LOG_INF("V: %.3f, I: %.3f, T: %.2f, SoC: %.2f, TTE: %.0f, TTF: %.0f, SoH: %.2f, "
		"Cycles: %d, Chg: %.2f",
		(double)voltage, (double)-current, (double)temp, (double)soc, (double)tte,
		(double)ttf, (double)soh, state_info.cycle_count,
		(double)state_info.cycle_count_cumulative_mah);

	if (IS_ENABLED(CONFIG_SETTINGS) && atomic_set(&store_state, false)) {

		ret = nrf_fuel_gauge_state_get(fuel_gauge_state_buf, sizeof(fuel_gauge_state_buf));
		if (ret < 0) {
			LOG_ERR("Could not get fuel gauge state (err %d)", ret);
			return ret;
		}

		ret = settings_save_one(SETTINGS_KEY, fuel_gauge_state_buf,
					nrf_fuel_gauge_state_size);
		if (ret < 0) {
			LOG_ERR("Could not store fuel gauge state (err %d)", ret);
			return ret;
		}

		LOG_INF("Fuel gauge state stored");
	}

	return 0;
}

#if CONFIG_SHELL && CONFIG_SETTINGS
static int cmd_fuel_gauge_state_store(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "Storing state after next update");
	atomic_set(&store_state, true);

	return 0;
}

SHELL_CMD_ARG_REGISTER(fuel_gauge_state_store, NULL, "Store battery state after next iteration",
		       cmd_fuel_gauge_state_store, 1, 0);
#endif /* CONFIG_SHELL && CONFIG_SETTINGS */
