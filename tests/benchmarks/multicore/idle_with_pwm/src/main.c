/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(idle_with_pwm, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/pm/device_runtime.h>


#if IS_ENABLED(CONFIG_SOC_NRF54H20_CPUAPP_COMMON)
/* Alias pwm-led0 = &pwm_led2 */
static const struct pwm_dt_spec pwm_led = PWM_DT_SPEC_GET(DT_ALIAS(pwm_led0));
#elif IS_ENABLED(CONFIG_SOC_NRF54H20_CPURAD_COMMON)
/* Alias pwm-led0 = &pwm_led3 */
static const struct pwm_dt_spec pwm_led = PWM_DT_SPEC_GET(DT_ALIAS(pwm_led0));
#else
#error "Invalid core selected."
#endif

#define PWM_STEPS_PER_SEC	(50)


int main(void)
{
	int ret;
	unsigned int cnt = 0;
	uint32_t pwm_period;
	uint32_t pulse_min;
	uint32_t pulse_max;
	int32_t pulse_step;
	uint32_t current_pulse_width;

	if (!pwm_is_ready_dt(&pwm_led)) {
		LOG_ERR("Device %s is not ready.", pwm_led.dev->name);
		return -ENODEV;
	}

	/*
	 * In case the default pwm_period value cannot be set for
	 * some PWM hardware, decrease its value until it can.
	 */
	pwm_period = PWM_USEC(200U);
	LOG_INF("Testing PWM period of %d us for channel %d",
		pwm_period, pwm_led.channel);
	while (pwm_set_dt(&pwm_led, pwm_period, pwm_period) != 0) {
		pwm_period /= 2U;
		LOG_INF("Decreasing period to %u", pwm_period);
	}

	/*
	 * There is no distinct change in LED brightness for high PWM duty cycles.
	 * Thus limit duty cycle to [0; max/2].
	 */
	pulse_min = 0;
	pulse_max = pwm_period / 2;
	pulse_step = (pulse_max - pulse_min) / PWM_STEPS_PER_SEC;
	LOG_DBG("Period is %u nsec", pwm_period);
	LOG_DBG("PWM pulse width varies from %u to %u with %u step",
		pulse_min, pulse_max, pulse_step);

	LOG_INF("Multicore idle_with_pwm test on %s", CONFIG_BOARD_TARGET);
	LOG_INF("Core will sleep for %d ms", CONFIG_TEST_SLEEP_DURATION_MS);

	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		pm_device_runtime_enable(pwm_led.dev);
	}

	while (1) {
		LOG_INF("Multicore idle_with_pwm test iteration %u", cnt++);

		/* Light up LED */
		current_pulse_width = pulse_min;
		for (int i = 0; i < PWM_STEPS_PER_SEC; i++) {
			/* set pulse width */
			ret = pwm_set_dt(&pwm_led, pwm_period, current_pulse_width);
			if (ret) {
				LOG_ERR("pwm_set_dt(%u, %u) returned %d",
					pwm_period, current_pulse_width, ret);
				return ret;
			}
			current_pulse_width += pulse_step;
			k_msleep(1000 / PWM_STEPS_PER_SEC);
		}

		/* Disable PWM / LED OFF */
		ret = pwm_set_dt(&pwm_led, 0, 0);
		if (ret) {
			LOG_ERR("pwm_set_dt(%u, %u) returned %d",
				0, 0, ret);
			return ret;
		}

		/* Sleep / enter low power state */
		k_msleep(CONFIG_TEST_SLEEP_DURATION_MS);
	}

	return 0;
}
