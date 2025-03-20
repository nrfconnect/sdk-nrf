/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "led.h"

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <zephyr/device.h>

#include "macros_common.h"
#include "led_assignments.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(led, CONFIG_MODULE_LED_LOG_LEVEL);

#define BLINK_FREQ_MS 1000

/* Maximum number of LED_UNITS. 1 RGB LED = 1 UNIT of 3 LEDS */
#define LED_MONOCHROME 1
#define NUM_COLORS_RGB 3

struct user_config {
	bool blink;
	enum led_color color;
};

struct led_unit_data {
	size_t num_gpios;
	const struct gpio_dt_spec *gpio;
	struct user_config user_cfg;
};

struct led_units_cfg {
	size_t num_units;
	struct led_unit_data *led;
};

#if DT_NODE_EXISTS(DT_NODELABEL(audioleds))
#define LED DT_NODELABEL(audioleds)
#else
#error "No DT_NODELABEL(audioleds) found in overlay"
#endif

/* clang-format off */
#define LED_GPIO_DT_SPEC(led_node_id)                                                              \
	static const struct gpio_dt_spec led_dt_gpio_spec_##led_node_id[] = {                      \
		DT_FOREACH_PROP_ELEM_SEP(led_node_id, gpios, GPIO_DT_SPEC_GET_BY_IDX, (,)),       \
	};
/* clang-format on */

#define LED_GPIO(led_node_id)                                                                      \
	{                                                                                          \
		.num_gpios = ARRAY_SIZE(led_dt_gpio_spec_##led_node_id),                           \
		.gpio = led_dt_gpio_spec_##led_node_id,                                            \
	},

DT_FOREACH_CHILD(LED, LED_GPIO_DT_SPEC)

static struct led_unit_data led_units_data[] = {DT_FOREACH_CHILD(LED, LED_GPIO)};

static const struct led_units_cfg led_units = {.num_units = ARRAY_SIZE(led_units_data),
					       .led = led_units_data};

static bool initialized;

/**
 * @brief Internal handling to set the status of a led unit
 */
static int led_set_int(uint8_t led, enum led_color color)
{
	int ret;

	if (led == LED_AUDIO_NOT_ASSIGNED) {
		LOG_WRN("LED unit not assigned");
		return 0;
	}

	struct led_unit_data *led_unit = &led_units.led[led];

	if (led_unit->num_gpios == LED_MONOCHROME) {
		if (color) {
			ret = gpio_pin_set_dt(led_unit->gpio, 1);
			if (ret) {
				return ret;
			}
		} else {
			ret = gpio_pin_set_dt(led_unit->gpio, 0);
			if (ret) {
				return ret;
			}
		}
	} else {
		for (uint8_t i = 0; i < NUM_COLORS_RGB; i++) {
			if (color & BIT(i)) {
				ret = gpio_pin_set_dt(&led_unit->gpio[i], 1);
				if (ret) {
					return ret;
				}
			} else {
				ret = gpio_pin_set_dt(&led_unit->gpio[i], 0);
				if (ret) {
					return ret;
				}
			}
		}
	}

	return 0;
}

static void led_blink_work_handler(struct k_work *work);

K_WORK_DEFINE(led_blink_work, led_blink_work_handler);

/**
 * @brief Submit a k_work on timer expiry.
 */
void led_blink_timer_handler(struct k_timer *dummy)
{
	k_work_submit(&led_blink_work);
}

K_TIMER_DEFINE(led_blink_timer, led_blink_timer_handler, NULL);

/**
 * @brief Periodically invoked by the timer to blink LEDs.
 */
static void led_blink_work_handler(struct k_work *work)
{
	int ret;
	static bool on_phase;

	for (uint8_t i = 0; i < led_units.num_units; i++) {
		if (led_units.led[i].user_cfg.blink) {
			if (on_phase) {
				ret = led_set_int(i, led_units.led[i].user_cfg.color);
				ERR_CHK(ret);
			} else {
				ret = led_set_int(i, LED_COLOR_OFF);
				ERR_CHK(ret);
			}
		}
	}

	on_phase = !on_phase;
}

static int led_set(uint8_t led, enum led_color color, bool blink)
{
	int ret;

	if (!initialized) {
		return -EPERM;
	}

	if (led == LED_AUDIO_NOT_ASSIGNED) {
		LOG_WRN("LED unit not assigned");
		return 0;
	}

	ret = led_set_int(led, color);
	if (ret) {
		return ret;
	}

	led_units.led[led].user_cfg.blink = blink;
	led_units.led[led].user_cfg.color = color;

	return 0;
}

int led_on(uint8_t led, ...)
{
	if (led == LED_AUDIO_NOT_ASSIGNED) {
		LOG_WRN("LED on unit not assigned: %d", led);
		return 0;
	}

	if (led_units.led[led].num_gpios == LED_MONOCHROME) {
		return led_set(led, LED_ON, LED_SOLID);
	}

	va_list args;

	va_start(args, led);
	int color = va_arg(args, int);

	va_end(args);

	if (color <= 0 || color >= LED_COLOR_NUM) {
		LOG_ERR("Invalid color code %d", color);
		return -EINVAL;
	}

	return led_set(led, color, LED_SOLID);
}

int led_off(uint8_t led)
{
	if (led == LED_AUDIO_NOT_ASSIGNED) {
		LOG_WRN("LED off unit not assigned: %d", led);
		return 0;
	}

	return led_set(led, LED_COLOR_OFF, LED_SOLID);
}

int led_blink(uint8_t led, ...)
{
	if (led == LED_AUDIO_NOT_ASSIGNED) {
		LOG_WRN("LED blink unit not assigned: %d", led);
		return 0;
	}

	if (led_units.led[led].num_gpios == LED_MONOCHROME) {
		return led_set(led, LED_ON, LED_BLINK);
	}

	va_list args;

	va_start(args, led);

	int color = va_arg(args, int);

	va_end(args);

	if (color <= 0 || color >= LED_COLOR_NUM) {
		LOG_ERR("Invalid color code %d", color);
		return -EINVAL;
	}

	return led_set(led, color, LED_BLINK);
}

int led_init(void)
{
	if (initialized) {
		return -EPERM;
	}

	__ASSERT(led_units.num_units != 0, "No LEDs found in dts");

	for (size_t i = 0; i < led_units.num_units; i++) {
		int ret;
		struct led_unit_data *led_unit = &led_units.led[i];

		for (size_t j = 0; j < led_unit->num_gpios; j++) {
			if (device_is_ready(led_unit->gpio[j].port)) {
				ret = gpio_pin_configure_dt(&led_unit->gpio[j],
							    GPIO_OUTPUT_INACTIVE);
				if (ret) {
					LOG_ERR("Cannot configure GPIO (ret %d)", ret);
					return ret;
				}
			} else {
				LOG_ERR("GPIO device not ready");
				return -ENODEV;
			}
		}
	}

	k_timer_start(&led_blink_timer, K_MSEC(BLINK_FREQ_MS / 2), K_MSEC(BLINK_FREQ_MS / 2));

	initialized = true;

	return 0;
}
