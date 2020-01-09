/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <sys/util.h>
#include <gpio.h>
#include <pwm.h>

#include "ui.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(ui_nmos, CONFIG_UI_LOG_LEVEL);

/*
 * Period to use for always-on/always-off before any PWM period is specified.
 * Needs to be supported by the PWM prescaler setting
 */
#define DEFAULT_PERIOD_US 20000

#define NMOS_CONFIG_DEFAULT(_pin)	(.pin = _pin)
#define NMOS_CONFIG_UNUSED		(.pin = -1)
#define NMOS_CONFIG_PIN(_pin)		COND_CODE_0(_pin, NMOS_CONFIG_UNUSED,  \
						NMOS_CONFIG_DEFAULT(_pin))
#define NMOS_CONFIG(_pin)		{ NMOS_CONFIG_PIN(_pin),	       \
						.mode = NMOS_MODE_GPIO }


struct nmos_config {
	int pin;
	enum {
		NMOS_MODE_GPIO,
		NMOS_MODE_PWM
	} mode;
};

static struct device *gpio_dev;
static struct device *pwm_dev;

static u32_t current_period_us;

static struct nmos_config nmos_pins[] = {
	[UI_NMOS_1] = NMOS_CONFIG(CONFIG_UI_NMOS_1_PIN),
	[UI_NMOS_2] = NMOS_CONFIG(CONFIG_UI_NMOS_2_PIN),
	[UI_NMOS_3] = NMOS_CONFIG(CONFIG_UI_NMOS_3_PIN),
	[UI_NMOS_4] = NMOS_CONFIG(CONFIG_UI_NMOS_4_PIN),
};

static int pwm_out(u32_t pin, u32_t period_us, u32_t duty_cycle_us)
{

	/* Applying workaround due to limitations in PWM driver that doesn't
	 * allow changing period while PWM is running. Setting pulse to 0
	 * disables the PWM, but not before the current period is finished.
	 */
	if (current_period_us != period_us) {
		pwm_pin_set_usec(pwm_dev, pin, current_period_us, 0);
		k_sleep(MAX(K_MSEC(current_period_us / USEC_PER_MSEC),
			    K_MSEC(1)));
	}

	current_period_us = period_us;

	return pwm_pin_set_usec(pwm_dev, pin, period_us, duty_cycle_us);
}

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
static bool pwm_is_in_use(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(nmos_pins); i++) {
		if (nmos_pins[i].mode == NMOS_MODE_PWM) {
			return true;
		}
	}

	return false;
}
#endif /* CONFIG_DEVICE_POWER_MANAGEMENT */

static void nmos_pwm_disable(u32_t nmos_idx)
{
	pwm_out(nmos_pins[nmos_idx].pin, current_period_us, 0);

	nmos_pins[nmos_idx].mode = NMOS_MODE_GPIO;

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	if (pwm_is_in_use()) {
		return;
	}

	int err = device_set_power_state(pwm_dev,
					 DEVICE_PM_SUSPEND_STATE,
					 NULL, NULL);
	if (err) {
		LOG_WRN("PWM disable failed");
	}
#endif /* CONFIG_DEVICE_POWER_MANAGEMENT */
}

static int nmos_pwm_enable(size_t nmos_idx)
{
	int err = 0;

	nmos_pins[nmos_idx].mode = NMOS_MODE_PWM;

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	u32_t power_state;

	device_get_power_state(pwm_dev, &power_state);

	if (power_state == DEVICE_PM_ACTIVE_STATE) {
		return 0;
	}

	err = device_set_power_state(pwm_dev,
					 DEVICE_PM_ACTIVE_STATE,
					 NULL, NULL);
	if (err) {
		LOG_ERR("PWM enable failed");
		return err;
	}
#endif /* CONFIG_DEVICE_POWER_MANAGEMENT */

	return err;
}

static int configure_gpio(u32_t pin)
{
	int err;

	err = gpio_pin_configure(gpio_dev, pin, GPIO_DIR_OUT);
	if (err) {
		return err;
	}

	err = gpio_pin_write(gpio_dev, pin, 0);
	if (err) {
		return err;
	}

	return err;
}

static int nmos_gpio_enable(size_t nmos_idx)
{
	int err = 0;

	if (nmos_pins[nmos_idx].mode == NMOS_MODE_PWM) {
		nmos_pwm_disable(nmos_idx);

		err = configure_gpio(nmos_pins[nmos_idx].pin);
		if (err) {
			LOG_ERR("Could not configure GPIO, error: %d", err);
			return err;
		}
	}

	return err;
}

int ui_nmos_init(void)
{
	int err = 0;

	gpio_dev = device_get_binding(DT_NORDIC_NRF_GPIO_GPIO_0_LABEL);
	if (!gpio_dev) {
		LOG_ERR("Could not bind to device %s",
			log_strdup(DT_NORDIC_NRF_GPIO_GPIO_0_LABEL));
		return -ENODEV;
	}

	for (size_t i = 0; i < ARRAY_SIZE(nmos_pins); i++) {
		if ((nmos_pins[i].pin < 0) ||
		    (nmos_pins[i].mode != NMOS_MODE_GPIO)) {
			continue;
		}

		err = configure_gpio(nmos_pins[i].pin);
		if (err) {
			LOG_ERR("Could not configure pin %d, error: %d",
				nmos_pins[i].pin, err);
			continue;
		}
	}

	pwm_dev = device_get_binding(CONFIG_UI_NMOS_PWM_DEV_NAME);
	if (!pwm_dev) {
		LOG_ERR("Could not bind to device %s",
			log_strdup(CONFIG_UI_NMOS_PWM_DEV_NAME));
		return -ENODEV;
	}

	for (size_t i = 0; i < ARRAY_SIZE(nmos_pins); i++) {
		pwm_pin_set_usec(pwm_dev,
				 nmos_pins[i].pin,
				 DEFAULT_PERIOD_US,
				 0);
	}

	current_period_us = DEFAULT_PERIOD_US;

	return err;
}

int ui_nmos_pwm_set(size_t nmos_idx, u32_t period, u32_t pulse)
{
	if ((pulse > period) || (period == 0)) {
		LOG_ERR("Period has to be non-zero and period >= duty cycle");
		return -EINVAL;
	}

	if (nmos_idx > (ARRAY_SIZE(nmos_pins) - 1)) {
		LOG_ERR("Invalid NMOS instance: %d", nmos_idx);
		return -EINVAL;
	}

	if (nmos_pins[nmos_idx].mode != NMOS_MODE_PWM) {
		int err = nmos_pwm_enable(nmos_idx);

		if (err) {
			LOG_ERR("Could not enable PWM for pin %d, error: %d",
				nmos_pins[nmos_idx].pin, err);
			return err;
		}
	}

	return pwm_out(nmos_pins[nmos_idx].pin, period, pulse);
}

int ui_nmos_write(size_t nmos_idx, u8_t value)
{
	int err;

	if (nmos_idx > (ARRAY_SIZE(nmos_pins) - 1)) {
		LOG_ERR("Invalid NMOS instance: %d", nmos_idx);
		return -EINVAL;
	}

	nmos_gpio_enable(nmos_idx);
	nmos_pwm_disable(nmos_idx);

	value = (value == 0) ? 0 : 1;

	err = gpio_pin_write(gpio_dev, nmos_pins[nmos_idx].pin, value);
	if (err) {
		LOG_ERR("Setting GPIO state failed, error: %d", err);
		return err;
	}

	return pwm_out(nmos_pins[nmos_idx].pin,
		       current_period_us,
		       current_period_us * value);
}
