/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "led.h"

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <hal/nrf_gpio.h>
#include <string.h>
#include <stdlib.h>
#include <zephyr/device.h>

#include "macros_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(led, CONFIG_LOG_LED_LEVEL);

#define ACTIVE_LOW 1
#define ACTIVE_HIGH 0

#define BLINK_FREQ_MS 1000
/* Maximum number of LED_UNITS. 1 RGB LED = 1 UNIT of 3 LEDS */
#define LED_UNIT_MAX 10
#define NUM_COLORS_RGB 3
#define BASE_10 10

#define CORE_APP_STR "CORE_APP"
#define CORE_NET_STR "CORE_NET"

#define LABEL_AND_COMMA(node_id) DT_LABEL(node_id),
#define GPIO_AND_COMMA(node_id) DT_GPIO_PIN(node_id, gpios),
#define GPIO_LABEL_AND_COMMMA(node_id) DT_GPIO_LABEL(node_id, gpios),
#define GPIO_FLAGS_AND_COMMMA(node_id) DT_GPIO_FLAGS(node_id, gpios),

/* The following arrays are populated compile time from the .dts*/
const char *led_labels[] = { DT_FOREACH_CHILD(DT_PATH(leds), LABEL_AND_COMMA) };

const int led_pins[] = { DT_FOREACH_CHILD(DT_PATH(leds), GPIO_AND_COMMA) };

const char *led_gpio_labels[] = { DT_FOREACH_CHILD(DT_PATH(leds), GPIO_LABEL_AND_COMMMA) };

const int led_gpio_flags[] = { DT_FOREACH_CHILD(DT_PATH(leds), GPIO_FLAGS_AND_COMMMA) };

enum led_core_assigned {
	LED_CORE_APP,
	LED_CORE_NET,
};

enum led_type {
	LED_MONOCHROME,
	LED_COLOR,
};

struct pin_dev {
	int pin;
	const struct device *dev;
	bool pol;
};

struct user_config {
	bool blink;
	enum led_color color;
};

struct led_unit_cfg {
	uint8_t led_no;
	enum led_core_assigned core;
	enum led_type unit_type;
	union {
		struct pin_dev mono;
		struct pin_dev color[NUM_COLORS_RGB];
	} type;
	struct user_config user_cfg;
};

static uint8_t leds_num;
static bool initialized;
static struct led_unit_cfg led_units[LED_UNIT_MAX];

/**
 * @brief pin_configure wrapper. Initializes all LEDs to off.
 */
static int gpio_pin_cfg(const struct device **dev, uint8_t pin, bool pol)
{
	if (pol == ACTIVE_LOW) {
		return gpio_pin_configure(*dev, pin, GPIO_OUTPUT_HIGH);
	} else {
		return gpio_pin_configure(*dev, pin, GPIO_OUTPUT_LOW);
	}
}

/**
 * @brief Configures fields for a RGB LED
 */
static int configure_led_color(uint8_t led_unit, uint8_t led_color, uint8_t led)
{
	int ret;

	led_units[led_unit].type.color[led_color].pin = led_pins[led];
	led_units[led_unit].type.color[led_color].pol = (bool)led_gpio_flags[led];
	led_units[led_unit].type.color[led_color].dev = device_get_binding(led_gpio_labels[led]);

	if (led_units[led_unit].type.color[led_color].dev == NULL) {
		LOG_ERR("Could not get binding for %s", led_gpio_labels[led]);
		return -ENODEV;
	}

	led_units[led_unit].unit_type = LED_COLOR;
	ret = gpio_pin_cfg(&led_units[led_unit].type.color[led_color].dev,
			   led_units[led_unit].type.color[led_color].pin,
			   led_units[led_unit].type.color[led_color].pol);

	return ret;
}

/**
 * @brief Configures fields for a monochrome LED
 */
static int config_led_monochrome(uint8_t led_unit, uint8_t led)
{
	int ret;

	led_units[led_unit].type.mono.pin = led_pins[led];
	led_units[led_unit].type.mono.dev = device_get_binding(led_gpio_labels[led]);
	led_units[led_unit].type.mono.pol = (bool)led_gpio_flags[led];

	if (led_units[led_unit].type.mono.dev == NULL) {
		LOG_ERR("Could not get binding for %s", led_gpio_labels[led]);
		return -ENODEV;
	}

	led_units[led_unit].unit_type = LED_MONOCHROME;

	ret = gpio_pin_cfg(&led_units[led_unit].type.mono.dev, led_units[led_unit].type.mono.pin,
			   led_units[led_unit].type.mono.pol);

	return ret;
}

/**
 * @brief Gives a LED pin from APP to NET core.
 */
static int transfer_pin_to_net(uint8_t led)
{
	uint8_t pin_to_transfer;

	if (strcmp(led_gpio_labels[led], "GPIO_0") == 0) {
		pin_to_transfer = led_pins[led];
	} else if (strcmp(led_gpio_labels[led], "GPIO_1") == 0) {
		pin_to_transfer = led_pins[led] + P0_PIN_NUM;
	} else {
		LOG_ERR("Invalid GPIO device");
		return -ENODEV;
	}

	nrf_gpio_pin_mcu_select(pin_to_transfer, NRF_GPIO_PIN_MCUSEL_NETWORK);
	LOG_DBG("Pin %d transferred on device %s", led_pins[led], led_gpio_labels[led]);
	return 0;
}

/**
 * @brief Parses the device tree for LED settings.
 */
static int led_device_tree_parse(void)
{
	int ret;

	for (uint8_t i = 0; i < leds_num; i++) {
		char *end_ptr = NULL;
		uint32_t led_unit = strtoul(led_labels[i], &end_ptr, BASE_10);

		if (led_labels[i] == end_ptr) {
			LOG_ERR("No match for led unit. The dts is likely not properly formatted");
			return -ENXIO;
		}

		if (strstr(led_labels[i], CORE_APP_STR)) {
			led_units[led_unit].core = LED_CORE_APP;
			if (!IS_ENABLED(CONFIG_BOARD_NRF5340_AUDIO_DK_NRF5340_CPUAPP)) {
				LOG_DBG("NET core: LED belongs to APP core. Skipping");
				continue;
			}

		} else if (strstr(led_labels[i], CORE_NET_STR)) {
			led_units[led_unit].core = LED_CORE_NET;
			if (IS_ENABLED(CONFIG_BOARD_NRF5340_AUDIO_DK_NRF5340_CPUAPP)) {
				ret = transfer_pin_to_net(i);
				if (ret) {
					return ret;
				}
			}
			if (!IS_ENABLED(CONFIG_BOARD_NRF5340_AUDIO_DK_NRF5340_CPUNET)) {
				LOG_DBG("APP core: LED belongs to NET core. Skipping");
				continue;
			}
		} else {
			LOG_ERR("LED %d unit %d with no core identifier in the label", i, led_unit);
			return -ENODEV;
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
		} else {
			LOG_ERR("No color identifier for LED %d LED unit %d", i, led_unit);
			return -ENODEV;
		}
	}
	return 0;
}

/**
 * @brief Internal handling to turn a given LED on
 */
static int led_on_int(struct pin_dev const *const pin_dev)
{
	return gpio_pin_set(pin_dev->dev, pin_dev->pin, !pin_dev->pol);
}

/**
 * @brief Internal handling to turn a given LED off
 */
static int led_off_int(struct pin_dev const *const pin_dev)
{
	return gpio_pin_set(pin_dev->dev, pin_dev->pin, pin_dev->pol);
}

/**
 * @brief Internal handling to set the status of a led unit
 */
static int led_set_int(uint8_t led_unit, enum led_color color)
{
	int ret;

	if (!IS_ENABLED(CONFIG_BOARD_NRF5340_AUDIO_DK_NRF5340_CPUAPP) &&
	    !IS_ENABLED(CONFIG_BOARD_NRF5340_AUDIO_DK_NRF5340_CPUNET)) {
		LOG_ERR("No core is selected");
		return -ENXIO;
	}

	if (IS_ENABLED(CONFIG_BOARD_NRF5340_AUDIO_DK_NRF5340_CPUAPP) &&
	    led_units[led_unit].core != LED_CORE_APP) {
		LOG_ERR("LED unit %d assigned to the other core", led_unit);
		return -ENXIO;
	}

	if (IS_ENABLED(CONFIG_BOARD_NRF5340_AUDIO_DK_NRF5340_CPUNET) &&
	    led_units[led_unit].core != LED_CORE_NET) {
		LOG_ERR("LED unit %d assigned to the other core", led_unit);
		return -ENXIO;
	}

	if (led_units[led_unit].unit_type == LED_MONOCHROME) {
		if (color) {
			ret = led_on_int(&led_units[led_unit].type.mono);
			if (ret) {
				return ret;
			}
		} else {
			ret = led_off_int(&led_units[led_unit].type.mono);
			if (ret) {
				return ret;
			}
		}
	} else {
		for (uint8_t i = 0; i < NUM_COLORS_RGB; i++) {
			if (color & BIT(i)) {
				ret = led_on_int(&led_units[led_unit].type.color[i]);
				if (ret) {
					return ret;
				}
			} else {
				ret = led_off_int(&led_units[led_unit].type.color[i]);
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

	for (uint8_t i = 0; i < leds_num; i++) {
		if (led_units[i].user_cfg.blink) {
			if (on_phase) {
				ret = led_set_int(i, led_units[i].user_cfg.color);
				ERR_CHK(ret);
			} else {
				ret = led_set_int(i, LED_COLOR_OFF);
				ERR_CHK(ret);
			}
		}
	}

	on_phase = !on_phase;
}

static int led_set(uint8_t led_unit, enum led_color color, bool blink)
{
	int ret;

	if (!initialized) {
		return -EPERM;
	}

	ret = led_set_int(led_unit, color);
	if (ret) {
		return ret;
	}

	led_units[led_unit].user_cfg.blink = blink;
	led_units[led_unit].user_cfg.color = color;

	return 0;
}

int led_on(uint8_t led_unit, ...)
{
	if (led_units[led_unit].unit_type == LED_MONOCHROME) {
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

int led_blink(uint8_t led_unit, ...)
{
	if (led_units[led_unit].unit_type == LED_MONOCHROME) {
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

int led_off(uint8_t led_unit)
{
	return led_set(led_unit, LED_COLOR_OFF, LED_SOLID);
}

int led_init(void)
{
	int ret;

	if (initialized) {
		return -EPERM;
	}

	__ASSERT(ARRAY_SIZE(led_labels) != 0, "No LED labels found in dts");
	__ASSERT(ARRAY_SIZE(led_pins) != 0, "No LED pins found in dts");
	__ASSERT(ARRAY_SIZE(led_gpio_labels) != 0, "No LED GPIO labels found in dts");
	__ASSERT(ARRAY_SIZE(led_gpio_flags) != 0, "No LED GPIO flags found in dts");

	leds_num = ARRAY_SIZE(led_labels);

	ret = led_device_tree_parse();
	if (ret) {
		return ret;
	}

	k_timer_start(&led_blink_timer, K_MSEC(BLINK_FREQ_MS / 2), K_MSEC(BLINK_FREQ_MS / 2));
	initialized = true;
	return 0;
}
