/*
 * Copyright (c) 2026, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <drivers/pulse_meas.h>
#include <nrfx_pwm.h>
#include <nrfx_gpiote.h>
#include <gpiote_nrfx.h>
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

#define SEM_TIMEOUT K_MSEC(100)

#define PULSE_MEAS_DEV(node) DEVICE_DT_GET(node),

/* Test is executed on all enabled pulse_meas instances at the same time. Each instance is
 * connected to one of the PWM outputs. All PWM outputs generate the same signal.
 */
static const struct device *const pulse_meas_devs[] = {
	DT_FOREACH_STATUS_OKAY(nordic_pulse_meas, PULSE_MEAS_DEV)
};

#define NUM_INSTANCES ARRAY_SIZE(pulse_meas_devs)

#define GPIOTE_INST(node) NRF_DT_GPIOTE_NODE(node, assert_gpios)

/* GPIOTE interrupt must be connected by the application if GPIO driver is not used. It is
 * connected for the first instance and for the second instance only if it uses a different GPIOTE
 * instance.
 */
#define GPIOTE_IRQ_CONNECT(node)                                                                   \
	NRF_DT_IRQ_CONNECT(GPIOTE_INST(node), nrfx_gpiote_irq_handler,                             \
			   &GPIOTE_NRFX_INST_BY_NODE(GPIOTE_INST(node)))

#define PULSE_BLOCK_SIZE PULSE_MEAS_BLOCK_SIZE(NUMBER_OF_MEASUREMENTS)

K_MEM_SLAB_DEFINE(meas_slab, PULSE_BLOCK_SIZE, 128, 4);

static struct k_sem pulse_meas_sems[NUM_INSTANCES];
static nrfx_pwm_t pwm = NRFX_PWM_INSTANCE(DT_REG_ADDR(DT_NODELABEL(tester_pwm)));

static void pulse_meas_handler(void *context)
{
	k_sem_give(context);
}

/* Configure and start measurement on all instances. The semaphore of the given instance is
 * given when a series is completed on that instance.
 */
static void configure_and_start_all(const struct pulse_meas_config *cfg_template)
{
	for (size_t i = 0; i < NUM_INSTANCES; i++) {
		struct pulse_meas_config cfg = *cfg_template;

		cfg.user_handler = &pulse_meas_handler;
		cfg.user_context = &pulse_meas_sems[i];

		k_sem_reset(&pulse_meas_sems[i]);

		int ret = pulse_meas_configure(pulse_meas_devs[i], &cfg);

		zassert_ok(ret, "Instance %zu, configure failed: %d", i, ret);

		ret = pulse_meas_start(pulse_meas_devs[i], &meas_slab);
		zassert_ok(ret, "Instance %zu, start failed: %d", i, ret);
	}
}

static void wait_for_series(size_t inst_idx)
{
	zassert_ok(k_sem_take(&pulse_meas_sems[inst_idx], SEM_TIMEOUT), "Instance %zu",
		   inst_idx);
}

static nrf_pwm_values_t buf_prepare(void *buf, size_t size)
{
	nrf_pwm_values_t buffer;
#ifdef CONFIG_HAS_NORDIC_DMM

	zassert_ok(dmm_buffer_out_prepare(
		DMM_DEV_TO_REG(DT_NODELABEL(tester_pwm)), buf, size, (void **)&buffer));
#else
	buffer.p_raw = buf;
#endif
	return buffer;
}

static void run_pulse_meas_test(bool positive)
{
	nrfx_pwm_config_t const pwm_config =
		NRFX_PWM_DEFAULT_CONFIG(PWM_OUTPUT_PIN(0), PWM_OUTPUT_PIN(1),
					NRF_PWM_PIN_NOT_CONNECTED, NRF_PWM_PIN_NOT_CONNECTED);

	nrf_pwm_values_common_t pwm_duty_cycle_values[] = {1, 10, 100, 500, 900, 990, 999};
	nrf_pwm_sequence_t pwm_sequence = {.values = {pwm_duty_cycle_values},
					   .length = ARRAY_SIZE(pwm_duty_cycle_values),
					   .repeats = 0,
					   .end_delay = 0};

	pwm_sequence.values = buf_prepare(pwm_duty_cycle_values, sizeof(pwm_duty_cycle_values));

	zassert_ok(nrfx_pwm_init(&pwm, &pwm_config, NULL, NULL));

	struct pulse_meas_config pulse_meas_cfg = {
		.num_of_meas = ARRAY_SIZE(pwm_duty_cycle_values),
		.mode = PULSE_MEAS_MODE_ONE_SHOT,
		.pull_config = NRF_GPIO_PIN_NOPULL,
	};

	pulse_meas_cfg.pulse_type = positive ?
		PULSE_MEAS_PULSE_POSITIVE : PULSE_MEAS_PULSE_NEGATIVE;

	configure_and_start_all(&pulse_meas_cfg);

	zassert_ok(nrfx_pwm_simple_playback(&pwm, &pwm_sequence, 1, NRFX_PWM_FLAG_STOP));

	for (size_t inst = 0; inst < NUM_INSTANCES; inst++) {
		uint32_t *data;

		wait_for_series(inst);

		zassert_ok(pulse_meas_get(pulse_meas_devs[inst], &data), "Instance %zu", inst);

		/* The first measurement is skipped for the negative pulse. */
		for (uint32_t i = positive ? 0 : 1; i < ARRAY_SIZE(pwm_duty_cycle_values); i++) {
			uint32_t expected = positive ? PWM_TOP_VALUE - pwm_duty_cycle_values[i]
						     : pwm_duty_cycle_values[i];

			zassert_within(data[i], expected, MEASUREMENT_PRECISION_US,
				       "Instance %zu, measurement %u", inst, i);
		}
	}

	nrfx_pwm_uninit(&pwm);
}

ZTEST(test_pulse_meas, test_pulse_meas_positive)
{
	run_pulse_meas_test(true);
}

ZTEST(test_pulse_meas, test_pulse_meas_negative)
{
	run_pulse_meas_test(false);
}

static void test_pulse_meas_short_pulse(uint16_t pulse_width, bool low_latency)
{
	nrfx_pwm_config_t pwm_config =
		NRFX_PWM_DEFAULT_CONFIG(PWM_OUTPUT_PIN(0), PWM_OUTPUT_PIN(1),
					NRF_PWM_PIN_NOT_CONNECTED, NRF_PWM_PIN_NOT_CONNECTED);

	nrf_pwm_values_common_t pwm_duty_cycle_values[4];
	nrf_pwm_sequence_t pwm_sequence = {.values = {pwm_duty_cycle_values},
					   .length = ARRAY_SIZE(pwm_duty_cycle_values),
					   .repeats = 0,
					   .end_delay = 0};

	pwm_config.top_value = pulse_width * 2;
	for (uint32_t i = 0; i < ARRAY_SIZE(pwm_duty_cycle_values); i++) {
		pwm_duty_cycle_values[i] = pulse_width - 2 + i;
	}
	pwm_sequence.values = buf_prepare(pwm_duty_cycle_values, sizeof(pwm_duty_cycle_values));

	zassert_ok(nrfx_pwm_init(&pwm, &pwm_config, NULL, NULL));

	struct pulse_meas_config pulse_meas_cfg = {
		.num_of_meas = ARRAY_SIZE(pwm_duty_cycle_values),
		.mode = PULSE_MEAS_MODE_ONE_SHOT,
		.pull_config = NRF_GPIO_PIN_NOPULL,
		.latency = low_latency ? PULSE_MEAS_LATENCY_LOW : PULSE_MEAS_LATENCY_DEFAULT,
		.pulse_type = PULSE_MEAS_PULSE_POSITIVE,
	};

	configure_and_start_all(&pulse_meas_cfg);

	zassert_ok(nrfx_pwm_simple_playback(&pwm, &pwm_sequence, 1, NRFX_PWM_FLAG_STOP));

	for (size_t inst = 0; inst < NUM_INSTANCES; inst++) {
		uint32_t *data;

		wait_for_series(inst);

		zassert_ok(pulse_meas_get(pulse_meas_devs[inst], &data), "Instance %zu", inst);

		for (uint32_t i = 0; i < ARRAY_SIZE(pwm_duty_cycle_values); i++) {
			uint32_t expected = pwm_config.top_value - pwm_duty_cycle_values[i];

			zassert_within(data[i], expected, MEASUREMENT_PRECISION_US,
				       "Instance %zu, measurement %u", inst, i);
		}

		zassert_equal(pulse_meas_pending(pulse_meas_devs[inst]), 0, "Instance %zu", inst);
	}

	nrfx_pwm_uninit(&pwm);
}

ZTEST(test_pulse_meas, test_pulse_meas_short_pulse_low_latency)
{
	/* When low latency mode is used (which prevents read only memory from going to sleep),
	 * shorter pulses can be measured as interrupt handling latency is smaller.
	 * Pulse width need to be read from TIMER peripheral before the end of the next pulse.
	 * It is possible that without low latency mode read is correct but only because interrupt
	 * handler code is in cache. If not in cache and read only memory goes to sleep, interrupt
	 * latency is present (16 us on nRF54L series).
	 */
	test_pulse_meas_short_pulse(8, true);
}

ZTEST(test_pulse_meas, test_pulse_meas_short_pulse_default_latency)
{
	test_pulse_meas_short_pulse(12, false);
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

	pwm_sequence1.values = buf_prepare(pwm_duty_cycle_values1, sizeof(pwm_duty_cycle_values1));
	pwm_sequence2.values = buf_prepare(pwm_duty_cycle_values2, sizeof(pwm_duty_cycle_values2));

	zassert_ok(nrfx_pwm_init(&pwm, &pwm_config, NULL, NULL));

	struct pulse_meas_config pulse_meas_cfg = {
		.num_of_meas = ARRAY_SIZE(pwm_duty_cycle_values1),
		.pulse_type = PULSE_MEAS_PULSE_POSITIVE,
		.mode = PULSE_MEAS_MODE_CONTINUOUS,
		.pull_config = NRF_GPIO_PIN_NOPULL,
	};

	configure_and_start_all(&pulse_meas_cfg);

	zassert_ok(nrfx_pwm_simple_playback(&pwm, &pwm_sequence1, 1, NRFX_PWM_FLAG_STOP));

	for (size_t inst = 0; inst < NUM_INSTANCES; inst++) {
		uint32_t *data;

		wait_for_series(inst);

		zassert_equal(pulse_meas_pending(pulse_meas_devs[inst]), 1, "Instance %zu", inst);

		zassert_ok(pulse_meas_get(pulse_meas_devs[inst], &data), "Instance %zu", inst);

		for (uint32_t i = 0; i < ARRAY_SIZE(pwm_duty_cycle_values1); i++) {
			zassert_within(data[i], PWM_TOP_VALUE - pwm_duty_cycle_values1[i],
				       MEASUREMENT_PRECISION_US, "Instance %zu, measurement %u",
				       inst, i);
		}
	}

	zassert_ok(nrfx_pwm_simple_playback(&pwm, &pwm_sequence2, 1, NRFX_PWM_FLAG_STOP));

	for (size_t inst = 0; inst < NUM_INSTANCES; inst++) {
		uint32_t *data;

		wait_for_series(inst);

		zassert_equal(pulse_meas_pending(pulse_meas_devs[inst]), 1, "Instance %zu", inst);

		zassert_ok(pulse_meas_get(pulse_meas_devs[inst], &data), "Instance %zu", inst);

		for (uint32_t i = 0; i < ARRAY_SIZE(pwm_duty_cycle_values2); i++) {
			zassert_within(data[i], PWM_TOP_VALUE - pwm_duty_cycle_values2[i],
				       MEASUREMENT_PRECISION_US, "Instance %zu, measurement %u",
				       inst, i);
		}
	}

	for (size_t inst = 0; inst < NUM_INSTANCES; inst++) {
		pulse_meas_stop(pulse_meas_devs[inst], true);
	}

	zassert_ok(nrfx_pwm_simple_playback(&pwm, &pwm_sequence1, 1, NRFX_PWM_FLAG_STOP));

	/* No series is expected on any instance after the measurement was stopped. */
	k_sleep(K_MSEC(1000));

	for (size_t inst = 0; inst < NUM_INSTANCES; inst++) {
		zassert_equal(k_sem_take(&pulse_meas_sems[inst], K_NO_WAIT), -EBUSY,
			      "Instance %zu", inst);
		zassert_equal(pulse_meas_pending(pulse_meas_devs[inst]), 0, "Instance %zu", inst);
	}
}

static void *suite_setup(void)
{
	for (size_t i = 0; i < NUM_INSTANCES; i++) {
		zassert_true(device_is_ready(pulse_meas_devs[i]), "Instance %zu", i);
		k_sem_init(&pulse_meas_sems[i], 0, 1);
	}

#ifndef CONFIG_GPIO
	GPIOTE_IRQ_CONNECT(DT_NODELABEL(pulse_meas));
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(pulse_meas1))
	IF_DISABLED(DT_SAME_NODE(GPIOTE_INST(DT_NODELABEL(pulse_meas)),
				 GPIOTE_INST(DT_NODELABEL(pulse_meas1))),
		    (GPIOTE_IRQ_CONNECT(DT_NODELABEL(pulse_meas1));))
#endif
#endif

	return NULL;
}

static void teardown(void *fixture)
{
	ARG_UNUSED(fixture);

	for (size_t i = 0; i < NUM_INSTANCES; i++) {
		/* Make sure that the instance is in the state in which it can be reset, even if the
		 * test failed in the middle of the measurement.
		 */
		(void)pulse_meas_stop(pulse_meas_devs[i], true);
		(void)pulse_meas_reset(pulse_meas_devs[i]);
	}
}

ZTEST_SUITE(test_pulse_meas, NULL, suite_setup, NULL, teardown, NULL);
