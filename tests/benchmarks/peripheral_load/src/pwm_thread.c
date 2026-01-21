/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "common.h"

#if DT_NODE_HAS_STATUS(DT_ALIAS(pwm_led0), okay)
#include <zephyr/drivers/pwm.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pwm, LOG_LEVEL_INF);


static const struct pwm_dt_spec pwm_led0 = PWM_DT_SPEC_GET(DT_ALIAS(pwm_led0));

/* PWM thread */
static void pwm_thread(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	int ret;
	bool dir_up;
	uint32_t pwm_period;
	uint32_t pulse_min;
	uint32_t pulse_max;
	int32_t pulse_step, pulse_step_min, pulse_step_diff;
	uint32_t current_pulse_width;

	atomic_inc(&started_threads);

	if (!pwm_is_ready_dt(&pwm_led0)) {
		LOG_ERR("Device %s is not ready.", pwm_led0.dev->name);
		atomic_inc(&completed_threads);
		return;
	}

	/*
	 * In case the default pwm_period value cannot be set for
	 * some PWM hardware, decrease its value until it can.
	 */
	LOG_INF("Detecting PWM period for channel %d", pwm_led0.channel);
	pwm_period = PWM_USEC(200U);
	while (pwm_set_dt(&pwm_led0, pwm_period, pwm_period) != 0) {
		pwm_period /= 2U;
		LOG_INF("Decreasing period to %u", pwm_period);
	}

	/*
	 * There is no distinct change in LED brightness for high PWM duty cycles.
	 * Thus limit duty cycle to [0; max/2].
	 * Also, brightness is NOT a linear function of voltage on LED's terminals.
	 */
	pulse_min = 0;
	pulse_max = pwm_period / 2;
	pulse_step_min = (pulse_max - pulse_min) / 78U;
	pulse_step_diff = (pulse_max - pulse_min) / 3000U;
	/* Apply default values */
	pulse_step = pulse_step_min;
	current_pulse_width = pulse_min;
	dir_up = true;
	LOG_INF("Period is %u nsec", pwm_period);
	LOG_INF("PWM pulse width varies from %u to %u with %u step (variable)",
		pulse_min, pulse_max, pulse_step);

	for (int i = 0; i < (PWM_THREAD_COUNT_MAX * PWM_THREAD_OVERSAMPLING); i++) {
		/* set pulse width */
		ret = pwm_set_dt(&pwm_led0, pwm_period, current_pulse_width);
		if (ret) {
			LOG_ERR("pwm_set_dt(%u, %u) returned %d",
				pwm_period, current_pulse_width, ret);
			atomic_inc(&completed_threads);
			return;
		}

		if (dir_up) {
			/* increase brightness */
			current_pulse_width += pulse_step;
			if (current_pulse_width >= pulse_max) {
				/* allow current_pulse_width be higher than pulse_max */
				/* current_pulse_width = pulse_max; */
				dir_up = false;
			} else {
				/* increase step size */
				pulse_step += pulse_step_diff;
			}
		} else {
			/* decrease brightness */
			if (current_pulse_width >= pulse_step) {
				/* prevent value overflow */
				current_pulse_width -= pulse_step;
			} else {
				current_pulse_width = 0;
			}
			/* with decreasing step */
			if (pulse_step >= pulse_step_diff) {
				/* prevent value overflow */
				pulse_step -= pulse_step_diff;
			} else {
				pulse_step = pulse_step_min;
			}
			if (current_pulse_width <= pulse_min) {
				/* At lowest brightnest, restore defaul values */
				current_pulse_width = pulse_min;
				pulse_step = pulse_step_min;
				dir_up = true;
			}
		}

		/* Print current_pulse_width */
		if ((i % PWM_THREAD_OVERSAMPLING) == 0) {
			LOG_INF("pwm_period = %u, current_pulse_width = %u, pulse_step = %u",
				pwm_period, current_pulse_width, pulse_step);
		}

		k_msleep(PWM_THREAD_SLEEP / PWM_THREAD_OVERSAMPLING);
	}
	LOG_INF("PWM thread has completed");

	atomic_inc(&completed_threads);
}

K_THREAD_DEFINE(thread_pwm_id, PWM_THREAD_STACKSIZE, pwm_thread, NULL, NULL, NULL,
	K_PRIO_PREEMPT(PWM_THREAD_PRIORITY), 0, 0);

#else
#pragma message("PWM thread skipped due to missing node in the DTS")
#endif
