/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <drivers/pwm.h>
#include <drivers/gpio.h>

#include "ui_led.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(ui_led, CONFIG_UI_LOG_LEVEL);

#define LED_PWM_NODE				DT_NODELABEL(pwm0)
#define LED_PWM_CHANNEL(channel)	DT_PROP(LED_PWM_NODE, ch##channel##_pin)
#define LED_PWM_FLAGS				DT_PWMS_FLAGS(LED_PWM_NODE)
#define LED_PWM_DEV_LABEL			DT_LABEL(LED_PWM_NODE)

#define PWM_PERIOD_USEC				(USEC_PER_SEC / 100U)

static const struct device *led_dev;
static uint32_t pulse_width[NUM_LEDS];
static bool state[NUM_LEDS];

static const struct gpio_dt_spec leds[] = {
	GPIO_DT_SPEC_GET_OR(DT_ALIAS(led0), gpios, {}),
	GPIO_DT_SPEC_GET_OR(DT_ALIAS(led1), gpios, {}),
	GPIO_DT_SPEC_GET_OR(DT_ALIAS(led2), gpios, {}),
	GPIO_DT_SPEC_GET_OR(DT_ALIAS(led3), gpios, {}),
};

static int get_pwm_channel(uint8_t led_num, uint32_t *pwm_channel)
{
	switch (led_num) {
	case 0:
		*pwm_channel = LED_PWM_CHANNEL(0);
		break;
#if defined(CONFIG_BOARD_THINGY91_NRF9160_NS)
	case 1:
		*pwm_channel = LED_PWM_CHANNEL(1);
		break;
	case 2:
		*pwm_channel = LED_PWM_CHANNEL(2);
		break;
#endif
	default:
		LOG_ERR("PWM channel %u not supported (%d)", led_num, -ENOTSUP);
		return -ENOTSUP;
	}

	return 0;
}

int ui_led_pwm_on_off(uint8_t led_num, bool new_state)
{
	int ret;
	uint32_t pwm_channel;

	state[led_num] = new_state;

	ret = get_pwm_channel(led_num, &pwm_channel);
	if (ret) {
		return ret;
	}
	ret = pwm_pin_set_usec(led_dev, pwm_channel, PWM_PERIOD_USEC,
			pulse_width[led_num] * new_state, LED_PWM_FLAGS);
	if (ret) {
		LOG_ERR("Set LED PWM pin %u failed  (%d)", led_num, ret);
		return ret;
	}

	return 0;
}

static uint32_t calculate_pulse_width(uint8_t led_intensity)
{
	return PWM_PERIOD_USEC * led_intensity / UINT8_MAX;
}

int ui_led_pwm_set_intensity(uint8_t led_num, uint8_t led_intensity)
{
	int ret;
	uint32_t pwm_channel;

	pulse_width[led_num] = calculate_pulse_width(led_intensity);

	ret = get_pwm_channel(led_num, &pwm_channel);
	if (ret) {
		return ret;
	}
	ret = pwm_pin_set_usec(led_dev, pwm_channel, PWM_PERIOD_USEC,
				pulse_width[led_num] * state[led_num], LED_PWM_FLAGS);
	if (ret) {
		LOG_ERR("Set LED PWM pin %u failed (%d)", led_num, ret);
		return ret;
	}

	return 0;
}

int ui_led_pwm_init(void)
{
	led_dev = device_get_binding(LED_PWM_DEV_LABEL);
	if (!led_dev) {
		LOG_DBG("Could not bind to LED PWM device (%d)", -ENODEV);
		return -ENODEV;
	}

	return 0;
}

int ui_led_gpio_on_off(uint8_t led_num, bool new_state)
{
	int ret;

	if (led_num >= ARRAY_SIZE(leds) || leds[led_num].port == NULL) {
		return -EINVAL;
	}

	ret = gpio_pin_set_dt(&leds[led_num], new_state);
	if (ret) {
		LOG_ERR("Set LED %u pin failed (%d)", led_num, ret);
		return ret;
	}

	return 0;
}

int ui_led_gpio_init(void)
{
	int ret;

	for (size_t i = 0; i < ARRAY_SIZE(leds); ++i) {
		if (leds[i].port != NULL && !device_is_ready(leds[i].port)) {
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&leds[i], GPIO_OUTPUT_INACTIVE);
		if (ret) {
			LOG_ERR("Configure LED %d failed (%d)", i, ret);
			return ret;
		}
	}

	return 0;
}
