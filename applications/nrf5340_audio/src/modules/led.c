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

#define BLINK_FREQ_MS			    1000
/* Maximum number of LED_UNITS. 1 RGB LED = 1 UNIT of 3 LEDS */
#define LED_UNIT_MAX			    10
#define NUM_COLORS_RGB			    3
#define BASE_10				    10
#define DT_LABEL_AND_COMMA(node_id)	    DT_PROP(node_id, label),
#define GPIO_DT_SPEC_GET_AND_COMMA(node_id) GPIO_DT_SPEC_GET(node_id, gpios),

/* The following arrays are populated compile time from the .dts*/
static const char *const led_labels[] = {DT_FOREACH_CHILD(DT_PATH(leds), DT_LABEL_AND_COMMA)};

static const struct gpio_dt_spec leds[] = {
	DT_FOREACH_CHILD(DT_PATH(leds), GPIO_DT_SPEC_GET_AND_COMMA)};

enum led_type {
	LED_MONOCHROME,
	LED_COLOR,
};

struct user_config {
	bool blink;
	enum led_color color;
};

struct led_unit_cfg {
	uint8_t led_no;
	enum led_type unit_type;
	union {
		const struct gpio_dt_spec *mono;
		const struct gpio_dt_spec *color[NUM_COLORS_RGB];
	} type;
	struct user_config user_cfg;
};

static uint8_t leds_num;
static bool initialized;
static struct led_unit_cfg led_units[LED_UNIT_MAX];
static struct led_unit_cfg *led_units_map[LED_UNIT_MAX] = {NULL};
static uint8_t led_units_num;

/**
 * @brief Configures fields for a RGB LED
 */
static int configure_led_color(enum led_unit_assignment led_unit, uint8_t led_color, uint8_t led)
{
	if (!device_is_ready(leds[led].port)) {
		LOG_ERR("LED GPIO controller not ready");
		return -ENODEV;
	}

	led_units[led_unit].type.color[led_color] = &leds[led];
	led_units[led_unit].unit_type = LED_COLOR;

	return gpio_pin_configure_dt(led_units[led_unit].type.color[led_color],
				     GPIO_OUTPUT_INACTIVE);
}

/**
 * @brief Configures fields for a monochrome LED
 */
static int config_led_monochrome(enum led_unit_assignment led_unit, uint8_t led)
{
	if (!device_is_ready(leds[led].port)) {
		LOG_ERR("LED GPIO controller not ready");
		return -ENODEV;
	}

	led_units[led_unit].type.mono = &leds[led];
	led_units[led_unit].unit_type = LED_MONOCHROME;

	return gpio_pin_configure_dt(led_units[led_unit].type.mono, GPIO_OUTPUT_INACTIVE);
}

static void led_units_mapping(enum led_unit_assignment led_unit, uint8_t led)
{
	if (led == LED_AUDIO_DEVICE_TYPE_INDEX) {
		led_units_map[LED_AUDIO_DEVICE_TYPE] = &led_units[led_unit];
		led_units_num++;
	} else if (led == LED_AUDIO_NET_STATUS_INDEX) {
		led_units_map[LED_AUDIO_NET_STATUS] = &led_units[led_unit];
		led_units_num++;
	} else if (led == LED_AUDIO_CONN_STATUS_INDEX) {
		led_units_map[LED_AUDIO_CONN_STATUS] = &led_units[led_unit];
		led_units_num++;
	} else if (led == LED_AUDIO_SYNC_STATUS_INDEX) {
		led_units_map[LED_AUDIO_SYNC_STATUS] = &led_units[led_unit];
		led_units_num++;
	} else if (led == LED_AUDIO_APP_STATUS_INDEX) {
		led_units_map[LED_AUDIO_APP_STATUS] = &led_units[led_unit];
		led_units_num++;
	} else {
		LOG_DBG("LED %d not assigned", led);
	}
}

static int find_unit_number(const char *label, uint8_t *unit)
{
	uint8_t len = strlen(label);

	for (uint8_t i = 0; i < len; i++) {
		char c = *label;

		if (isdigit(c)) {
			*unit = strtoul(label, NULL, BASE_10);
			return 0;
		}

		label++;
	}

	return -EINVAL;
}

/**
 * @brief Parses the device tree for LED settings.
 */
static int led_device_tree_parse(void)
{
	int ret;

	for (uint8_t i = 0; i < leds_num; i++) {

		int8_t led_unit;

		ret = find_unit_number(led_labels[i], &led_unit);
		if (ret) {
			LOG_ERR("No match for led unit. The dts is likely not properly formatted");
			return -ENXIO;
		}

		if (strstr(led_labels[i], "LED_RGB_RED")) {
			ret = configure_led_color(led_unit, RED, i);
			if (ret) {
				return ret;
			}
		} else if (strstr(led_labels[i], "LED_RGB_GREEN")) {
			ret = configure_led_color(led_unit, GRN, i);
			if (ret) {
				return ret;
			}
		} else if (strstr(led_labels[i], "LED_RGB_BLUE")) {
			ret = configure_led_color(led_unit, BLU, i);
			if (ret) {
				return ret;
			}
		} else if (strstr(led_labels[i], "LED_MONO")) {
			ret = config_led_monochrome(led_unit, i);
			if (ret) {
				return ret;
			}
		} else if (strstr(led_labels[i], "Green LED")) {
			ret = config_led_monochrome(led_unit, i);
			if (ret) {
				return ret;
			}
		} else {
			LOG_ERR("No color identifier for LED %d LED unit %d", i, led_unit);
			return -ENODEV;
		}

		led_units_mapping(led_unit, i);
	}

	return 0;
}

/**
 * @brief Internal handling to set the status of a led unit
 */
static int led_set_int(enum led_unit_assignment led_unit, enum led_color color)
{
	int ret;

	if (led_units_map[led_unit]->unit_type == LED_MONOCHROME) {
		if (color) {
			ret = gpio_pin_set_dt(led_units_map[led_unit]->type.mono, 1);
			if (ret) {
				return ret;
			}
		} else {
			ret = gpio_pin_set_dt(led_units_map[led_unit]->type.mono, 0);
			if (ret) {
				return ret;
			}
		}
	} else {
		for (uint8_t i = 0; i < NUM_COLORS_RGB; i++) {
			if (color & BIT(i)) {
				ret = gpio_pin_set_dt(led_units_map[led_unit]->type.color[i], 1);
				if (ret) {
					return ret;
				}
			} else {
				ret = gpio_pin_set_dt(led_units_map[led_unit]->type.color[i], 0);
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

	for (uint8_t i = 0; i < LED_AUDIO_ASSIGN_NUM; i++) {
		if (led_units_map[i] != NULL && led_units_map[i]->user_cfg.blink) {
			if (on_phase) {
				ret = led_set_int(i, led_units_map[i]->user_cfg.color);
				ERR_CHK(ret);
			} else {
				ret = led_set_int(i, LED_COLOR_OFF);
				ERR_CHK(ret);
			}
		}
	}

	on_phase = !on_phase;
}

static int led_set(enum led_unit_assignment led_unit, enum led_color color, bool blink)
{
	int ret;

	if (!initialized) {
		return -EPERM;
	}

	ret = led_set_int(led_unit, color);
	if (ret) {
		return ret;
	}

	led_units_map[led_unit]->user_cfg.blink = blink;
	led_units_map[led_unit]->user_cfg.color = color;

	return 0;
}

int led_on(enum led_unit_assignment led_unit, ...)
{
	if (led_units_map[led_unit] == NULL) {
		LOG_WRN("LED unit %d is not assigned to an LED(s)", led_unit);
		return 0;
	}

	if (led_units_map[led_unit]->unit_type == LED_MONOCHROME) {
		return led_set(led_unit, LED_ON, LED_SOLID);
	}

	va_list args;

	va_start(args, led_unit);
	int color = va_arg(args, int);

	va_end(args);

	if (color <= 0 || color >= LED_COLOR_NUM) {
		LOG_ERR("Invalid color code %d", color);
		return -EINVAL;
	}

	return led_set(led_unit, color, LED_SOLID);
}

int led_blink(enum led_unit_assignment led_unit, ...)
{
	if (led_units_map[led_unit] == NULL) {
		LOG_WRN("LED unit %d is not assigned to an LED(s)", led_unit);
		return 0;
	}

	if (led_units_map[led_unit]->unit_type == LED_MONOCHROME) {
		return led_set(led_unit, LED_ON, LED_BLINK);
	}

	va_list args;

	va_start(args, led_unit);

	int color = va_arg(args, int);

	va_end(args);

	if (color <= 0 || color >= LED_COLOR_NUM) {
		LOG_ERR("Invalid color code %d", color);
		return -EINVAL;
	}

	return led_set(led_unit, color, LED_BLINK);
}

int led_off(enum led_unit_assignment led_unit)
{
	if (led_units_map[led_unit] == NULL) {
		LOG_WRN("LED unit %d is not assigned to an LED(s)", led_unit);
		return 0;
	}

	return led_set(led_unit, LED_COLOR_OFF, LED_SOLID);
}

int led_init(void)
{
	int ret;

	if (initialized) {
		return -EPERM;
	}

	__ASSERT(ARRAY_SIZE(leds) != 0, "No LEDs found in dts");

	leds_num = ARRAY_SIZE(leds);

	ret = led_device_tree_parse();
	if (ret) {
		return ret;
	}

	k_timer_start(&led_blink_timer, K_MSEC(BLINK_FREQ_MS / 2), K_MSEC(BLINK_FREQ_MS / 2));
	initialized = true;

	return 0;
}
