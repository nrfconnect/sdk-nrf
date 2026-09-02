/*
 * Copyright (c) 2026, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <drivers/pulse_meas.h>
#include <nrfx_pwm.h>
#include <pinctrl_soc.h>
#include <zephyr/devicetree/pinctrl.h>
#ifdef CONFIG_HAS_NORDIC_DMM
#include <dmm.h>
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(test);

#define PWM_OUTPUT_PIN(idx)                                                                        \
	NRF_GET_PIN(DT_PROP_BY_IDX(                                                                \
		DT_CHILD(DT_PINCTRL_BY_NAME(DT_NODELABEL(tester_pwm), default, 0), group1), psels, \
		idx))
#define PWM_INSTANCE &NRFX_PWM_INSTANCE(DT_REG_ADDR(DT_NODELABEL(tester_pwm)))

#define NUMBER_OF_MEASUREMENTS 8

#define PWM_TOP_VALUE 1000
#define MEASUREMENT_PRECISION_US 1

static const struct device *const pulse_meas_dev = DEVICE_DT_GET(DT_NODELABEL(pulse_meas));

K_MEM_SLAB_DEFINE(my_meas_slab, PULSE_MEAS_BLOCK_SIZE(NUMBER_OF_MEASUREMENTS), 128, 4);

static volatile bool pulse_meas_handler_called;

static void pulse_meas_handler(void *context)
{
	pulse_meas_handler_called = true;
}

ZTEST(test_pulse_meas, test_pulse_meas_positive)
{
	nrfx_pwm_config_t const pwm_config =
		NRFX_PWM_DEFAULT_CONFIG(PWM_OUTPUT_PIN(0), PWM_OUTPUT_PIN(1),
					NRF_PWM_PIN_NOT_CONNECTED, NRF_PWM_PIN_NOT_CONNECTED);

	nrf_pwm_values_common_t pwm_duty_cycle_values[] = {1, 10, 100, 500, 900, 990, 999};
	nrf_pwm_sequence_t pwm_sequence = {.values = {pwm_duty_cycle_values},
					   .length = ARRAY_SIZE(pwm_duty_cycle_values),
					   .repeats = 0,
					   .end_delay = 0};

	static nrfx_pwm_t pwm = NRFX_PWM_INSTANCE(DT_REG_ADDR(DT_NODELABEL(tester_pwm)));

#ifdef CONFIG_HAS_NORDIC_DMM
	nrf_pwm_values_t pwm_duty_cycle_values_buffer;

	zassert_ok(dmm_buffer_out_prepare(
		DMM_DEV_TO_REG(DT_NODELABEL(tester_pwm)), pwm_duty_cycle_values,
		sizeof(nrf_pwm_values_common_t) * ARRAY_SIZE(pwm_duty_cycle_values),
		(void **)&pwm_duty_cycle_values_buffer));

	pwm_sequence.values = pwm_duty_cycle_values_buffer;
#endif

	zassert_ok(nrfx_pwm_init(&pwm, &pwm_config, NULL, NULL));

	static struct pulse_meas_config pulse_meas_cfg = {
		.num_of_meas = ARRAY_SIZE(pwm_duty_cycle_values),
		.pulse_type = PULSE_MEAS_PULSE_POSITIVE,
		.mode = PULSE_MEAS_MODE_ONE_SHOT,
		.pull_config = NRF_GPIO_PIN_NOPULL,
		.user_handler = &pulse_meas_handler,
		.user_context = NULL,
	};

	pulse_meas_handler_called = false;

	zassert_true(device_is_ready(pulse_meas_dev));

	zassert_ok(pulse_meas_configure(pulse_meas_dev, &pulse_meas_cfg));

	zassert_ok(pulse_meas_start(pulse_meas_dev, &my_meas_slab));

	zassert_ok(nrfx_pwm_simple_playback(&pwm, &pwm_sequence, 1, NRFX_PWM_FLAG_STOP));

	while (!pulse_meas_handler_called) {
		k_sleep(K_MSEC(1));
	}

	uint32_t *data;

	zassert_ok(pulse_meas_get(pulse_meas_dev, &data));

	for (uint32_t i = 0; i < ARRAY_SIZE(pwm_duty_cycle_values); i++) {
		zassert_within(data[i], PWM_TOP_VALUE - pwm_duty_cycle_values[i],
			MEASUREMENT_PRECISION_US);
	}
}

ZTEST(test_pulse_meas, test_pulse_meas_negative)
{
	nrfx_pwm_config_t const pwm_config =
		NRFX_PWM_DEFAULT_CONFIG(PWM_OUTPUT_PIN(0), PWM_OUTPUT_PIN(1),
					NRF_PWM_PIN_NOT_CONNECTED, NRF_PWM_PIN_NOT_CONNECTED);

	nrf_pwm_values_common_t pwm_duty_cycle_values[] = {1, 10, 100, 500, 900, 990, 999};
	nrf_pwm_sequence_t pwm_sequence = {.values = {pwm_duty_cycle_values},
					   .length = ARRAY_SIZE(pwm_duty_cycle_values),
					   .repeats = 0,
					   .end_delay = 0};

	static nrfx_pwm_t pwm = NRFX_PWM_INSTANCE(DT_REG_ADDR(DT_NODELABEL(tester_pwm)));

#ifdef CONFIG_HAS_NORDIC_DMM
	nrf_pwm_values_t pwm_duty_cycle_values_buffer;

	zassert_ok(dmm_buffer_out_prepare(
		DMM_DEV_TO_REG(DT_NODELABEL(tester_pwm)), pwm_duty_cycle_values,
		sizeof(nrf_pwm_values_common_t) * ARRAY_SIZE(pwm_duty_cycle_values),
		(void **)&pwm_duty_cycle_values_buffer));

	pwm_sequence.values = pwm_duty_cycle_values_buffer;
#endif

	zassert_ok(nrfx_pwm_init(&pwm, &pwm_config, NULL, NULL));

	static struct pulse_meas_config pulse_meas_cfg = {
		.num_of_meas = ARRAY_SIZE(pwm_duty_cycle_values),
		.pulse_type = PULSE_MEAS_PULSE_NEGATIVE,
		.mode = PULSE_MEAS_MODE_ONE_SHOT,
		.pull_config = NRF_GPIO_PIN_NOPULL,
		.user_handler = &pulse_meas_handler,
		.user_context = NULL,
	};

	pulse_meas_handler_called = false;

	zassert_true(device_is_ready(pulse_meas_dev));

	zassert_ok(pulse_meas_configure(pulse_meas_dev, &pulse_meas_cfg));

	zassert_ok(pulse_meas_start(pulse_meas_dev, &my_meas_slab));

	zassert_ok(nrfx_pwm_simple_playback(&pwm, &pwm_sequence, 1, NRFX_PWM_FLAG_STOP));

	while (!pulse_meas_handler_called) {
		k_sleep(K_MSEC(1));
	}

	uint32_t *data;

	zassert_ok(pulse_meas_get(pulse_meas_dev, &data));

	for (uint32_t i = 1; i < ARRAY_SIZE(pwm_duty_cycle_values); i++) {
		zassert_within(data[i], pwm_duty_cycle_values[i], MEASUREMENT_PRECISION_US);
	}
}

ZTEST(test_pulse_meas, test_pulse_meas_continuous)
{
	nrfx_pwm_config_t const pwm_config =
		NRFX_PWM_DEFAULT_CONFIG(PWM_OUTPUT_PIN(0), PWM_OUTPUT_PIN(1),
					NRF_PWM_PIN_NOT_CONNECTED, NRF_PWM_PIN_NOT_CONNECTED);

	nrf_pwm_values_common_t pwm_duty_cycle_values1[] = {1, 10, 100, 500, 900, 990, 999};
	nrf_pwm_values_common_t pwm_duty_cycle_values2[] = {32, 99, 346, 2, 43, 897, 376};

	zassert_equal(ARRAY_SIZE(pwm_duty_cycle_values1), ARRAY_SIZE(pwm_duty_cycle_values2));

	nrf_pwm_sequence_t pwm_sequence1 = {.values = {pwm_duty_cycle_values1},
					    .length = ARRAY_SIZE(pwm_duty_cycle_values1),
					    .repeats = 0,
					    .end_delay = 0};
	nrf_pwm_sequence_t pwm_sequence2 = {.values = {pwm_duty_cycle_values2},
					    .length = ARRAY_SIZE(pwm_duty_cycle_values2),
					    .repeats = 0,
					    .end_delay = 0};

	static nrfx_pwm_t pwm = NRFX_PWM_INSTANCE(DT_REG_ADDR(DT_NODELABEL(tester_pwm)));

#ifdef CONFIG_HAS_NORDIC_DMM
	nrf_pwm_values_t pwm_duty_cycle_values1_buffer;
	nrf_pwm_values_t pwm_duty_cycle_values2_buffer;

	zassert_ok(dmm_buffer_out_prepare(
		DMM_DEV_TO_REG(DT_NODELABEL(tester_pwm)), pwm_duty_cycle_values1,
		sizeof(nrf_pwm_values_common_t) * ARRAY_SIZE(pwm_duty_cycle_values1),
		(void **)&pwm_duty_cycle_values1_buffer));
	zassert_ok(dmm_buffer_out_prepare(
		DMM_DEV_TO_REG(DT_NODELABEL(tester_pwm)), pwm_duty_cycle_values2,
		sizeof(nrf_pwm_values_common_t) * ARRAY_SIZE(pwm_duty_cycle_values2),
		(void **)&pwm_duty_cycle_values2_buffer));

	pwm_sequence1.values = pwm_duty_cycle_values1_buffer;
	pwm_sequence2.values = pwm_duty_cycle_values2_buffer;
#endif

	zassert_ok(nrfx_pwm_init(&pwm, &pwm_config, NULL, NULL));

	static struct pulse_meas_config pulse_meas_cfg = {
		.num_of_meas = ARRAY_SIZE(pwm_duty_cycle_values1),
		.pulse_type = PULSE_MEAS_PULSE_POSITIVE,
		.mode = PULSE_MEAS_MODE_CONTINUOUS,
		.pull_config = NRF_GPIO_PIN_NOPULL,
		.user_handler = &pulse_meas_handler,
		.user_context = NULL,
	};

	pulse_meas_handler_called = false;

	zassert_true(device_is_ready(pulse_meas_dev));

	zassert_ok(pulse_meas_configure(pulse_meas_dev, &pulse_meas_cfg));

	zassert_ok(pulse_meas_start(pulse_meas_dev, &my_meas_slab));

	zassert_ok(nrfx_pwm_simple_playback(&pwm, &pwm_sequence1, 1, NRFX_PWM_FLAG_STOP));

	while (!pulse_meas_handler_called) {
		k_sleep(K_MSEC(1));
	}

	pulse_meas_handler_called = false;

	zassert_equal(pulse_meas_pending(pulse_meas_dev), 1);

	uint32_t *data;

	zassert_ok(pulse_meas_get(pulse_meas_dev, &data));

	for (uint32_t i = 0; i < ARRAY_SIZE(pwm_duty_cycle_values1); i++) {
		zassert_within(data[i], PWM_TOP_VALUE - pwm_duty_cycle_values1[i],
			MEASUREMENT_PRECISION_US);
	}

	zassert_ok(nrfx_pwm_simple_playback(&pwm, &pwm_sequence2, 1, NRFX_PWM_FLAG_STOP));
	while (!pulse_meas_handler_called) {
		k_sleep(K_MSEC(1));
	}

	pulse_meas_handler_called = false;

	zassert_equal(pulse_meas_pending(pulse_meas_dev), 1);

	zassert_ok(pulse_meas_get(pulse_meas_dev, &data));

	for (uint32_t i = 0; i < ARRAY_SIZE(pwm_duty_cycle_values1); i++) {
		zassert_within(data[i], PWM_TOP_VALUE - pwm_duty_cycle_values2[i],
			       MEASUREMENT_PRECISION_US);
	}

	pulse_meas_stop(pulse_meas_dev, true);

	zassert_ok(nrfx_pwm_simple_playback(&pwm, &pwm_sequence1, 1, NRFX_PWM_FLAG_STOP));

	k_sleep(K_MSEC(1000));

	zassert_false(pulse_meas_handler_called);

	zassert_equal(pulse_meas_pending(pulse_meas_dev), 0);
}

static void *suite_setup(void)
{

	return NULL;
}

ZTEST_SUITE(test_pulse_meas, NULL, suite_setup, NULL, NULL, NULL);
