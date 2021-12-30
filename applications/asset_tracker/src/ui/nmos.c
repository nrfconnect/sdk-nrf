/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <sys/util.h>
#include <drivers/gpio.h>
#include <drivers/pwm.h>
#include <pm/device.h>

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

static const struct device *gpio_dev;
static const struct device *pwm_dev;

static uint32_t current_period_us;

static struct nmos_config nmos_pins[] = {
	[UI_NMOS_1] = NMOS_CONFIG(CONFIG_UI_NMOS_1_PIN),
	[UI_NMOS_2] = NMOS_CONFIG(CONFIG_UI_NMOS_2_PIN),
	[UI_NMOS_3] = NMOS_CONFIG(CONFIG_UI_NMOS_3_PIN),
	[UI_NMOS_4] = NMOS_CONFIG(CONFIG_UI_NMOS_4_PIN),
};

static int pwm_out(uint32_t pin, uint32_t period_us, uint32_t duty_cycle_us)
{

	/* Applying workaround due to limitations in PWM driver that doesn't
	 * allow changing period while PWM is running. Setting pulse to 0
	 * disables the PWM, but not before the current period is finished.
	 */
	if (current_period_us != period_us) {
		pwm_pin_set_usec(pwm_dev, pin, current_period_us, 0, 0);
		k_sleep(K_MSEC(MAX((current_period_us / USEC_PER_MSEC), 1)));
	}

	current_period_us = period_us;

	return pwm_pin_set_usec(pwm_dev, pin, period_us, duty_cycle_us, 0);
}

#ifdef CONFIG_PM_DEVICE
static bool pwm_is_in_use(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(nmos_pins); i++) {
		if (nmos_pins[i].mode == NMOS_MODE_PWM) {
			return true;
		}
	}

	return false;
}
#endif /* CONFIG_PM_DEVICE */

static void nmos_pwm_disable(uint32_t nmos_idx)
{
	pwm_out(nmos_pins[nmos_idx].pin, current_period_us, 0);

	nmos_pins[nmos_idx].mode = NMOS_MODE_GPIO;

#ifdef CONFIG_PM_DEVICE
	if (pwm_is_in_use()) {
		return;
	}

	int err = pm_device_action_run(pwm_dev, PM_DEVICE_ACTION_SUSPEND);
	if (err) {
		LOG_WRN("PWM disable failed");
	}
#endif /* CONFIG_PM_DEVICE */
}

static int nmos_pwm_enable(size_t nmos_idx)
{
	int err = 0;

	nmos_pins[nmos_idx].mode = NMOS_MODE_PWM;

#ifdef CONFIG_PM_DEVICE
	uint32_t power_state;

	pm_device_state_get(pwm_dev, &power_state);

	if (power_state == PM_DEVICE_STATE_ACTIVE) {
		return 0;
	}

	err = pm_device_action_run(pwm_dev, PM_DEVICE_ACTION_RESUME);
	if (err) {
		LOG_ERR("PWM enable failed");
		return err;
	}
#endif /* CONFIG_PM_DEVICE */

	return err;
}

static int configure_gpio(uint32_t pin)
{
	int err;

	err = gpio_pin_configure(gpio_dev, pin, GPIO_OUTPUT);
	if (err) {
		return err;
	}

	err = gpio_pin_set_raw(gpio_dev, pin, 0);
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

	gpio_dev = device_get_binding(DT_LABEL(DT_NODELABEL(gpio0)));
	if (!gpio_dev) {
		LOG_ERR("Could not bind to device %s",
			log_strdup(DT_LABEL(DT_NODELABEL(gpio0))));
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
				 0,
				 0);
	}

	current_period_us = DEFAULT_PERIOD_US;

	return err;
}

int ui_nmos_pwm_set(size_t nmos_idx, uint32_t period, uint32_t pulse)
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

int ui_nmos_write(size_t nmos_idx, uint8_t value)
{
	int err;

	if (nmos_idx > (ARRAY_SIZE(nmos_pins) - 1)) {
		LOG_ERR("Invalid NMOS instance: %d", nmos_idx);
		return -EINVAL;
	}

	nmos_gpio_enable(nmos_idx);
	nmos_pwm_disable(nmos_idx);

	value = (value == 0) ? 0 : 1;

	err = gpio_pin_set_raw(gpio_dev, nmos_pins[nmos_idx].pin, value);
	if (err) {
		LOG_ERR("Setting GPIO state failed, error: %d", err);
		return err;
	}

	return pwm_out(nmos_pins[nmos_idx].pin,
		       current_period_us,
		       current_period_us * value);
}
