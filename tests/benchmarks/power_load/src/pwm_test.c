/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "pwm_test.h"

LOG_MODULE_REGISTER(pwm_test, LOG_LEVEL_INF);

extern atomic_t started_threads;

#define PWM_SPEC(node_id, prop, idx) PWM_DT_SPEC_GET_BY_IDX(node_id, idx),

static const struct pwm_dt_spec pwm_devices[] = {
	DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), pwms, PWM_SPEC)};

#define PWM_DEVICES_COUNT ARRAY_SIZE(pwm_devices)

static int setup(void)
{
	int ret;

	for (uint8_t pwm_id = 0; pwm_id < PWM_DEVICES_COUNT; pwm_id++) {
		ret = pwm_is_ready_dt(&pwm_devices[pwm_id]);
		if (ret != 1) {
			LOG_ERR("PWM device %u not ready: %d", pwm_id, ret);
			return -1;
		}
	}

	return 0;
}

static void pwm_load_thread_worker(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	int ret;
	uint32_t pwm_pulse_width = PWM_MSEC(10);
	uint32_t pwm_periods[] = {PWM_MSEC(20), PWM_MSEC(30), PWM_MSEC(40), PWM_MSEC(50)};
	uint8_t pwm_period_id = 0;

	ret = setup();
	if (ret != 0) {
		LOG_ERR("PWM setup failed: %d\n", ret);
		return;
	}

	atomic_inc(&started_threads);

	LOG_INF("PWM load thread started");
	while (1) {
		for (uint8_t pwm_id = 0; pwm_id < PWM_DEVICES_COUNT; pwm_id++) {
			ret = pwm_set_dt(&pwm_devices[pwm_id], pwm_periods[pwm_period_id],
					 pwm_pulse_width);
			if (ret != 0) {
				LOG_ERR("Failed to set PWM %u period: %d", pwm_id, ret);
			}
		}
		pwm_period_id = (pwm_period_id + 1) % ARRAY_SIZE(pwm_periods);
		k_msleep(PWM_THREAD_SLEEP);
	}
}

K_THREAD_DEFINE(pwm_load_thread, PWM_THREAD_STACKSIZE, pwm_load_thread_worker, NULL, NULL, NULL,
		K_PRIO_PREEMPT(PWM_THREAD_PRIORITY), 0, 0);
