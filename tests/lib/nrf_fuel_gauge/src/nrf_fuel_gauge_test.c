/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <unity.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/sys/util.h>

#include <nrf_fuel_gauge.h>

static const struct battery_model_primary battery_primary = {
#include "battery_models/primary_cell/AAA_Alkaline.inc"
};

static const struct battery_model battery_secondary = {
#include "battery_model.inc"
};

void setUp(void)
{
}

void tearDown(void)
{
}

static int initialize_fuel_gauge(float vbat, float ibat, float tbat, void *state)
{
	struct nrf_fuel_gauge_config_parameters opt_params;
	const struct nrf_fuel_gauge_init_parameters params = {
		.v0 = vbat,
		.i0 = ibat,
		.t0 = tbat,
#if IS_ENABLED(CONFIG_NRF_FUEL_GAUGE_VARIANT_SECONDARY_CELL)
		.model = &battery_secondary,
#elif IS_ENABLED(CONFIG_NRF_FUEL_GAUGE_VARIANT_PRIMARY_CELL)
		.model_primary = &battery_primary,
#endif
		.opt_params = &opt_params,
		.state = state,
	};
	float v0;

	(void)battery_primary;
	(void)battery_secondary;

	TEST_ASSERT_TRUE(strlen(nrf_fuel_gauge_version) > 0);
	TEST_ASSERT_TRUE(strlen(nrf_fuel_gauge_build_date) > 0);
	TEST_ASSERT_TRUE(nrf_fuel_gauge_state_size > 0);

	nrf_fuel_gauge_opt_params_default_get(&opt_params);
	opt_params.tte_min_time = 0.0f;

	return nrf_fuel_gauge_init(&params, &v0);
}

void test_nrf_fuel_gauge_init(void)
{
	int err;

	err = initialize_fuel_gauge(4.2f, 0.0f, 25.0f, NULL);
	TEST_ASSERT_EQUAL(0, err);
}

void test_nrf_fuel_gauge_sanity_discharge(void)
{
	struct nrf_fuel_gauge_state_info state_info;
	float soc_begin;
	float soc_end;
	float tte;
	float ttf;
	float temperature;
	float current;
	float time_step;
	float voltage_begin;
	float voltage_end;
	float voltage;
	int err;

	const float voltage_step = 0.001f;

#if IS_ENABLED(CONFIG_NRF_FUEL_GAUGE_VARIANT_SECONDARY_CELL)
	voltage_begin = 4.2f;
	voltage_end = 3.2f;
#elif IS_ENABLED(CONFIG_NRF_FUEL_GAUGE_VARIANT_PRIMARY_CELL)
	voltage_begin = 1.5f;
	voltage_end = 1.0f;
#endif

	/* Simple sanity check:
	 * decrease voltage over some iterations and verify that SOC and TTE changes accordingly.
	 */

	voltage = voltage_begin;
	temperature = 25.0f;
	current = 0.08f;
	time_step = 60.0f;

	err = initialize_fuel_gauge(voltage, current, temperature, NULL);
	TEST_ASSERT_EQUAL(0, err);

	(void)nrf_fuel_gauge_process(voltage, current, temperature, time_step, &state_info);
	soc_begin = state_info.soc_raw;
	soc_end = soc_begin;

	TEST_ASSERT_FALSE(isnan(soc_begin));
	TEST_ASSERT_FALSE(signbit(soc_begin));
	TEST_ASSERT_FALSE(isnan(state_info.yhat));
	TEST_ASSERT_FALSE(isnan(state_info.r0));

	for (int i = 0; i < ((voltage_begin - voltage_end) / voltage_step); ++i) {
		voltage -= voltage_step;
		(void)nrf_fuel_gauge_process(voltage, current, temperature, time_step, &state_info);
		soc_end = state_info.soc_raw;
		TEST_ASSERT_FALSE(isnan(soc_end));
		TEST_ASSERT_FALSE(signbit(soc_end));

		if (IS_ENABLED(CONFIG_NRF_FUEL_GAUGE_VARIANT_SECONDARY_CELL) && i > 100) {
			/* Expect "time-to-empty" to become valid after a few cycles */
			tte = nrf_fuel_gauge_tte_get();
			TEST_ASSERT_FALSE(isnan(tte));
		} else if (IS_ENABLED(CONFIG_NRF_FUEL_GAUGE_VARIANT_PRIMARY_CELL)) {
			/* Always NAN for primary cell */
			ttf = nrf_fuel_gauge_ttf_get();
			TEST_ASSERT_TRUE(isnan(ttf));
		}

		/* Don't expect time-to-full to be valid when discharging */
		ttf = nrf_fuel_gauge_ttf_get();
		TEST_ASSERT_TRUE(isnan(ttf));
	}

	TEST_ASSERT_TRUE(soc_end < soc_begin);
}

void test_nrf_fuel_gauge_sanity_state_resume(void)
{
	struct nrf_fuel_gauge_state_info state_info;
	float temperature;
	float current;
	float time_step;
	float voltage_begin;
	float voltage_end;
	float voltage;
	float voltage_saved;
	float soc_saved[100];
	int step_count;
	int err;

	const float voltage_step = 0.001f;

	static uint8_t state_memory[1024];

#if IS_ENABLED(CONFIG_NRF_FUEL_GAUGE_VARIANT_SECONDARY_CELL)
	voltage_begin = 4.2f;
	voltage_end = 3.2f;
#elif IS_ENABLED(CONFIG_NRF_FUEL_GAUGE_VARIANT_PRIMARY_CELL)
	voltage_begin = 1.5f;
	voltage_end = 1.0f;
#endif

	/* Simple sanity check:
	 * Run the fuel gauge for a while, store the state, continue running, resume the stored
	 * state, and verify that SOC is the same.
	 */

	voltage = voltage_begin;
	temperature = 25.0f;
	current = 0.08f;
	time_step = 60.0f;
	step_count = ((voltage_begin - voltage_end) / voltage_step);

	TEST_ASSERT_TRUE((step_count / 2) > ARRAY_SIZE(soc_saved));
	TEST_ASSERT_TRUE(sizeof(state_memory) >= nrf_fuel_gauge_state_size);

	err = initialize_fuel_gauge(voltage, current, temperature, NULL);
	TEST_ASSERT_EQUAL(0, err);

	/* Run for a while to fill filters */
	for (int i = 0; i < step_count / 2; ++i) {
		voltage -= voltage_step;
		(void)nrf_fuel_gauge_process(voltage, current, temperature, time_step, &state_info);
	}

	/* Store state */
	err = nrf_fuel_gauge_state_get(state_memory, sizeof(state_memory));
	TEST_ASSERT_EQUAL(0, err);

	voltage_saved = voltage;

	/* Fill comparison vector */
	for (int i = 0; i < ARRAY_SIZE(soc_saved); ++i) {
		voltage -= voltage_step;
		(void)nrf_fuel_gauge_process(voltage, current, temperature, time_step, &state_info);
		soc_saved[i] = state_info.soc_raw;
	}

	/* Re-init from previous state memory */
	err = initialize_fuel_gauge(voltage, current, temperature, state_memory);
	TEST_ASSERT_EQUAL(0, err);
	voltage = voltage_saved;

	/* Compare with previous run */
	for (int i = 0; i < ARRAY_SIZE(soc_saved); ++i) {
		float soc;

		voltage -= voltage_step;
		(void)nrf_fuel_gauge_process(voltage, current, temperature, time_step, &state_info);
		soc = state_info.soc_raw;
		TEST_ASSERT_EQUAL_FLOAT(soc_saved[i], soc);
	}
}

void test_nrf_fuel_gauge_sanity_charge(void)
{
	struct nrf_fuel_gauge_state_info state_info;
	float soc_begin;
	float soc_end;
	float ttf;
	float temperature;
	float init_current;
	float charge_current;
	float time_step;
	int err;

	const float voltage_step = 0.001f;
	const float voltage_begin = 3.2f;
	const float voltage_end = 4.2f;
	float voltage;

	if (IS_ENABLED(CONFIG_NRF_FUEL_GAUGE_VARIANT_PRIMARY_CELL)) {
		/* Cannot charge primary cell batteries */
		return;
	}

	/* Simple sanity check:
	 * apply negative current over some iterations and verify that SOC and TTF changes
	 * accordingly.
	 */

	voltage = voltage_begin;
	temperature = 25.0f;
	init_current = 0.08f;
	charge_current = -0.1f;
	time_step = 60.0f;

	err = initialize_fuel_gauge(voltage, init_current, temperature, NULL);
	TEST_ASSERT_EQUAL(0, err);

	(void)nrf_fuel_gauge_process(voltage, init_current, temperature, time_step, &state_info);
	soc_begin = state_info.soc_raw;
	soc_end = soc_begin;

	TEST_ASSERT_FALSE(isnan(soc_begin));
	TEST_ASSERT_FALSE(signbit(soc_begin));
	TEST_ASSERT_FALSE(isnan(state_info.yhat));
	TEST_ASSERT_FALSE(isnan(state_info.r0));

	/* Inform library of charging */
	err = nrf_fuel_gauge_ext_state_update(NRF_FUEL_GAUGE_EXT_STATE_INFO_VBUS_CONNECTED, NULL);
	TEST_ASSERT_EQUAL(0, err);
	err = nrf_fuel_gauge_ext_state_update(NRF_FUEL_GAUGE_EXT_STATE_INFO_CHARGE_CURRENT_LIMIT,
					      &(union nrf_fuel_gauge_ext_state_info_data){
						      .charge_current_limit = charge_current});
	TEST_ASSERT_EQUAL(0, err);
	err = nrf_fuel_gauge_ext_state_update(
		NRF_FUEL_GAUGE_EXT_STATE_INFO_TERM_CURRENT,
		&(union nrf_fuel_gauge_ext_state_info_data){.charge_term_current =
								    charge_current * 0.1f});
	TEST_ASSERT_EQUAL(0, err);
	err = nrf_fuel_gauge_ext_state_update(
		NRF_FUEL_GAUGE_EXT_STATE_INFO_CHARGE_STATE_CHANGE,
		&(union nrf_fuel_gauge_ext_state_info_data){
			.charge_state = NRF_FUEL_GAUGE_CHARGE_STATE_CC});
	TEST_ASSERT_EQUAL(0, err);

	for (int i = 0; i < (fabsf(voltage_begin - voltage_end) / voltage_step); ++i) {
		voltage += voltage_step;
		(void)nrf_fuel_gauge_process(voltage, charge_current, temperature, time_step,
					     &state_info);
		soc_end = state_info.soc_raw;
		TEST_ASSERT_FALSE(isnan(soc_end));
		TEST_ASSERT_FALSE(signbit(soc_end));

		/* Expect time-to-full to be valid after 1 iteration */
		if (i > 0) {
			ttf = nrf_fuel_gauge_ttf_get();
			TEST_ASSERT_TRUE(!isnan(ttf));
			TEST_ASSERT_TRUE(ttf > 0.0f);
		}
	}

	TEST_ASSERT_TRUE(soc_end > soc_begin);
}

void test_nrf_fuel_gauge_temp_range(void)
{
	struct nrf_fuel_gauge_state_info state_info;

	/* Verify that temperature is truncated to battery model range */
	const float model_temp_min = battery_secondary.temps[0];
	const float model_temp_max =
		battery_secondary.temps[ARRAY_SIZE(((struct battery_model){}).temps) - 1];
	const float temp_range_begin = model_temp_min - 10.0f;
	const float temp_range_end = model_temp_max + 10.0f;
	const float temp_step_size = 0.5f;

	float voltage = 4.2f;
	float temperature = 25.0f;
	float current = 0.08f;
	float time_step = 60.0f;
	int err;

	if (IS_ENABLED(CONFIG_NRF_FUEL_GAUGE_VARIANT_PRIMARY_CELL)) {
		/* Primary cell battery model does not have temperature range */
		return;
	}

	TEST_ASSERT_FALSE(isnan(model_temp_min));
	TEST_ASSERT_FALSE(isnan(model_temp_max));
	TEST_ASSERT_TRUE(model_temp_min < model_temp_max);

	err = initialize_fuel_gauge(voltage, current, temperature, NULL);
	TEST_ASSERT_EQUAL(0, err);

	for (float t = temp_range_begin; t <= temp_range_end; t += temp_step_size) {
		(void)nrf_fuel_gauge_process(voltage, current, t, time_step, &state_info);
		if (t < model_temp_min) {
			TEST_ASSERT_TRUE(state_info.T_truncated == model_temp_min);
		} else if (t > model_temp_max) {
			TEST_ASSERT_TRUE(state_info.T_truncated == model_temp_max);
		} else {
			TEST_ASSERT_TRUE(state_info.T_truncated == t);
		}
	}
}

void test_nrf_fuel_gauge_linking(void)
{
	const struct nrf_fuel_gauge_init_parameters params = {
		.v0 = 4.2f,
		.i0 = 0.0f,
		.t0 = 25.0f,
		.model = &battery_secondary,
		.state = NULL,
	};
	float v0;

	(void)nrf_fuel_gauge_init(&params, &v0);
	(void)nrf_fuel_gauge_process(4.2f, 0.07f, 25.0f, 60.0f, NULL);
	(void)nrf_fuel_gauge_idle_set(4.2f, 25.0f, 0.0f);
	(void)nrf_fuel_gauge_tte_get();
	(void)nrf_fuel_gauge_ttf_get();
	(void)nrf_fuel_gauge_ext_state_update(NRF_FUEL_GAUGE_EXT_STATE_INFO_VBUS_CONNECTED, NULL);
	(void)nrf_fuel_gauge_state_get(NULL, 0);
	(void)nrf_fuel_gauge_param_adjust(&(struct nrf_fuel_gauge_runtime_parameters){
		.a = 0.0f,
		.b = 0.0f,
		.c = 0.0f,
		.d = 0.0f,
	});
}

void test_nrf_fuel_gauge_variant(void)
{
	if (IS_ENABLED(CONFIG_NRF_FUEL_GAUGE_VARIANT_PRIMARY_CELL)) {
		TEST_ASSERT_EQUAL(NRF_FUEL_GAUGE_VARIANT_PRIMARY, nrf_fuel_gauge_variant);
	} else if (IS_ENABLED(CONFIG_NRF_FUEL_GAUGE_VARIANT_SECONDARY_CELL)) {
		TEST_ASSERT_EQUAL(NRF_FUEL_GAUGE_VARIANT_SECONDARY, nrf_fuel_gauge_variant);
	} else {
		TEST_FAIL();
	}
}

extern int unity_main(void);

int main(void)
{
	(void)unity_main();

	return 0;
}
