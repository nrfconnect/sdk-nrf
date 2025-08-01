/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(idle_pwm_led, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/cache.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/pm/device_runtime.h>


static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led), gpios);
static const struct pwm_dt_spec pwm_led = PWM_DT_SPEC_GET(DT_ALIAS(pwm_led0));

#define PWM_STEPS_PER_SEC	(50)
#define SHM_START_ADDR		(DT_REG_ADDR(DT_NODELABEL(cpuapp_cpurad_ipc_shm)))
volatile static uint32_t *shared_var = (volatile uint32_t *) SHM_START_ADDR;
#define HOST_IS_READY	(1)
#define REMOTE_IS_READY	(2)

int main(void)
{
	int ret;
	unsigned int cnt = 0;
	uint32_t pwm_period;
	uint32_t pulse_min;
	uint32_t pulse_max;
	int32_t pulse_step;
	uint32_t current_pulse_width;
	int test_repetitions = 3;

	if (!gpio_is_ready_dt(&led)) {
		LOG_ERR("GPIO Device not ready");
		return 0;
	}

	if (gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE) != 0) {
		LOG_ERR("Could not configure led GPIO");
		return 0;
	}

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

	LOG_INF("Multicore idle_pwm_led test on %s", CONFIG_BOARD_TARGET);
	LOG_INF("Core will sleep for %d ms", CONFIG_TEST_SLEEP_DURATION_MS);
	LOG_INF("Shared memory at %p", (void *) shared_var);

	/* Synchronize Remote core with Host core */
#if !defined(CONFIG_TEST_ROLE_REMOTE)
	LOG_DBG("HOST starts");
	*shared_var = HOST_IS_READY;
	sys_cache_data_flush_range((void *) shared_var, sizeof(*shared_var));
	LOG_DBG("HOST wrote HOST_IS_READY: %u", *shared_var);
	while (*shared_var != REMOTE_IS_READY) {
		k_msleep(1);
		sys_cache_data_invd_range((void *) shared_var, sizeof(*shared_var));
		LOG_DBG("shared_var is: %u", *shared_var);
	}
	LOG_DBG("HOST continues");
#else
	LOG_DBG("REMOTE starts");
	while (*shared_var != HOST_IS_READY) {
		k_msleep(1);
		sys_cache_data_invd_range((void *) shared_var, sizeof(*shared_var));
		LOG_DBG("shared_var is: %u", *shared_var);
	}
	LOG_DBG("REMOTE found that HOST_IS_READY");
	*shared_var = REMOTE_IS_READY;
	sys_cache_data_flush_range((void *) shared_var, sizeof(*shared_var));
	LOG_DBG("REMOTE wrote REMOTE_IS_READY: %u", *shared_var);
	LOG_DBG("REMOTE continues");
#endif

#if defined(CONFIG_COVERAGE)
	printk("Coverage analysis enabled\n");
	while (test_repetitions--)
#else
	while (test_repetitions)
#endif
	{
		LOG_INF("Multicore idle_pwm_led test iteration %u", cnt++);

#if defined(CONFIG_PM_DEVICE_RUNTIME)
		/* API for PWM doesn't provide function for device enable/disable.
		 * Thus PM for PWM has to be managed manually.
		 */
		pm_device_runtime_get(pwm_led.dev);
#endif

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

#if defined(CONFIG_PM_DEVICE_RUNTIME)
		/* API for PWM doesn't provide function for device enable/disable.
		 * Thus PM for PWM has to be managed manually.
		 */
		pm_device_runtime_put(pwm_led.dev);
#endif

		/* Sleep / enter low power state */
		gpio_pin_set_dt(&led, 0);
		k_msleep(CONFIG_TEST_SLEEP_DURATION_MS);
		gpio_pin_set_dt(&led, 1);
	}

#if defined(CONFIG_COVERAGE)
	printk("Coverage analysis start\n");
#endif
	return 0;
}
