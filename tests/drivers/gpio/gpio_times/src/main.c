/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/ztest.h>

const struct gpio_dt_spec in_pins[] = {
	DT_FOREACH_PROP_ELEM_SEP(DT_NODELABEL(in_gpios), gpios, GPIO_DT_SPEC_GET_BY_IDX, (,))
};
const uint8_t npairs = ARRAY_SIZE(in_pins);


#define PIN_STATE_SIZE 16384
static int pin_state[PIN_STATE_SIZE] = {};

#define REPEAT_NUMBER 10

ZTEST(gpio_times, test_gpios_read_times)
{
	uint64_t pulse_count = 0;
	uint64_t cycles_s_sys;
	uint32_t cycle_start_time;
	uint32_t cycle_stop_time;
	double gpio_read_time_us;
	double gpio_read_time_us_mean;

	cycles_s_sys = (uint64_t)sys_clock_hw_cycles_per_sec();

	for (uint32_t a=0; a < 15; a++) {
		pulse_count++;
		for (uint8_t p = 0; p < npairs; ++p) {
			gpio_read_time_us_mean = 0;
			for (uint32_t t = 0; t < REPEAT_NUMBER; ++t) {
				cycle_start_time = k_cycle_get_32();
				for (uint32_t i = 0; i < PIN_STATE_SIZE; ++i) {
					pin_state[i] = gpio_pin_get_dt(&in_pins[p]);
				}
				cycle_stop_time = k_cycle_get_32();
				gpio_read_time_us =
					((cycle_stop_time - cycle_start_time) / (double)PIN_STATE_SIZE) *
					(1e6 / cycles_s_sys);
				gpio_read_time_us_mean += gpio_read_time_us;
			}
			gpio_read_time_us_mean /= (double)REPEAT_NUMBER;
			TC_PRINT("[%s]GPIO get takes: %.2f us\n", in_pins[p].port->name, gpio_read_time_us_mean);
		}
		k_msleep(1);
	}
}

static void *suite_setup(void)
{
	int rc;

	for (uint8_t i = 0; i < npairs; i++) {
		zassert_true(gpio_is_ready_dt(&in_pins[i]), "IN[%d] is not ready", i);
		rc = gpio_pin_configure_dt(&in_pins[i], GPIO_INPUT);
		zassert_equal(rc, 0, "IN[%d] config failed", i);
	}

	return NULL;
}

ZTEST_SUITE(gpio_times, NULL, suite_setup, NULL, NULL, NULL);
