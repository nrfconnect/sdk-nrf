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

static const struct battery_model battery = {
#include "battery_model.inc"
};

void setUp(void)
{
}

void tearDown(void)
{
}

void test_nrf_fuel_gauge_init(void)
{
	const struct nrf_fuel_gauge_init_parameters params = {
		.v0 = 4.2f,
		.i0 = 0.0f,
		.t0 = 25.0f,
		.model = &battery,
	};
	float v0;
	int err;

	TEST_ASSERT_TRUE(strlen(nrf_fuel_gauge_version) > 0);
	TEST_ASSERT_TRUE(strlen(nrf_fuel_gauge_build_date) > 0);

	err = nrf_fuel_gauge_init(&params, &v0);
	TEST_ASSERT_EQUAL(0, err);
}

void test_nrf_fuel_gauge_sanity(void)
{
	struct nrf_fuel_gauge_state_info state_info;
	float soc_begin;
	float soc_end;
	float tte;
	float ttf;
	float temperature;
	float current;
	float time_step;

	const float voltage_step = 0.001f;
	const float voltage_begin = 4.2f;
	const float voltage_end = 3.2f;
	float voltage;

	/* Simple sanity check:
	 * decrease voltage over some iterations and verify that SOC and TTE changes accordingly.
	 */

	voltage = voltage_begin;
	temperature = 25.0f;
	current = 0.08f;
	time_step = 60.0f;

	soc_begin = nrf_fuel_gauge_process(voltage, current, temperature, time_step, &state_info);
	soc_end = soc_begin;

	TEST_ASSERT_FALSE(isnan(soc_begin));
	TEST_ASSERT_FALSE(signbit(soc_begin));
	TEST_ASSERT_FALSE(isnan(state_info.yhat));
	TEST_ASSERT_FALSE(isnan(state_info.r0));

	for (int i = 0; i < ((voltage_begin - voltage_end) / voltage_step); ++i) {
		voltage -= voltage_step;
		soc_end = nrf_fuel_gauge_process(
					voltage, current, temperature, time_step, &state_info);
		TEST_ASSERT_FALSE(isnan(soc_end));
		TEST_ASSERT_FALSE(signbit(soc_end));

		if (i > 100) {
			/* Expect "time-to-empty" to become valid after a few cycles */

			tte = nrf_fuel_gauge_tte_get();
			TEST_ASSERT_FALSE(isnan(tte));
		}

		/* Don't expect time-to-full to be valid when discharging */
		ttf = nrf_fuel_gauge_ttf_get(0.4f, 0.04f);
		TEST_ASSERT_TRUE(isnan(ttf));
	}

	TEST_ASSERT_TRUE(soc_end < soc_begin);

	/* Verify that temperature is truncated to battery model range */
	const float model_temp_min = battery.temps[0];
	const float model_temp_max =
		battery.temps[ARRAY_SIZE(((struct battery_model) {}).temps) - 1];
	const float temp_range_begin = model_temp_min - 10.0f;
	const float temp_range_end = model_temp_max + 10.0f;
	const float temp_step_size = 0.5f;

	TEST_ASSERT_FALSE(isnan(model_temp_min));
	TEST_ASSERT_FALSE(isnan(model_temp_max));
	TEST_ASSERT_TRUE(model_temp_min < model_temp_max);

	for (float t = temp_range_begin; t <= temp_range_end; t += temp_step_size) {
		(void) nrf_fuel_gauge_process(voltage, current, t, time_step, &state_info);
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
		.model = &battery,
	};
	float v0;

	(void) nrf_fuel_gauge_init(&params, &v0);
	(void) nrf_fuel_gauge_process(4.2f, 0.07f, 25.0f, 60.0f, NULL);
	(void) nrf_fuel_gauge_idle_set(4.2f, 25.0f, 0.0f);
	(void) nrf_fuel_gauge_tte_get();
	(void) nrf_fuel_gauge_ttf_get(0.0f, 0.0f);
	(void) nrf_fuel_gauge_param_adjust(0.0f, 0.0f, 0.0f, 0.0f);
}

extern int unity_main(void);

int main(void)
{
	(void)unity_main();

	return 0;
}
