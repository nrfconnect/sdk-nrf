/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#include <dk_buttons_and_leds.h>

#include "mosh_defines.h"
#include "mosh_print.h"

#define GPIO_PIN_INVALID -1

static const struct device *const gpio_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));

static struct gpio_callback gpio_callback;
static uint32_t gpio_pulse_count;
static int32_t gpio_pin = GPIO_PIN_INVALID;

static void gpio_handler(const struct device *port, struct gpio_callback *cb, gpio_port_pins_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(cb);

	int value;

	if (gpio_pin == GPIO_PIN_INVALID || pins != gpio_callback.pin_mask) {
		return;
	}

	value = gpio_pin_get(gpio_dev, gpio_pin);
	if (value) {
		gpio_pulse_count++;
#if defined(CONFIG_DK_LIBRARY) && !defined(CONFIG_BOARD_THINGY91_NRF9160_NS)
		dk_set_led_on(GPIO_STATUS_LED);
	} else {
		dk_set_led_off(GPIO_STATUS_LED);
#endif
	}
}

int gpio_count_enable(uint8_t pin)
{
	int err;

	if (gpio_pin != GPIO_PIN_INVALID) {
		mosh_error("GPIO pin pulse counting already enabled");

		return -ENOEXEC;
	}

	gpio_pin = pin;
	gpio_pulse_count = 0;

	err = gpio_pin_configure(gpio_dev, gpio_pin, GPIO_INPUT | GPIO_PULL_DOWN);
	if (err) {
		mosh_error("Failed to configure GPIO pin, error: %d\n", err);

		return -ENOEXEC;
	}

	err = gpio_pin_interrupt_configure(gpio_dev, gpio_pin, GPIO_INT_EDGE_BOTH);
	if (err) {
		mosh_error("Failed to configure GPIO pin interrupt, error: %d\n", err);

		return -ENOEXEC;
	}

	gpio_init_callback(&gpio_callback, gpio_handler, 1U << gpio_pin);

	err = gpio_add_callback(gpio_dev, &gpio_callback);
	if (err) {
		mosh_error("Failed to add GPIO callback, error: %d\n", err);

		return -ENOEXEC;
	}

	return 0;
}

int gpio_count_disable(void)
{
	int err;

	if (gpio_pin == GPIO_PIN_INVALID) {
		mosh_error("GPIO pin pulse counting not enabled");

		return -ENOEXEC;
	}

	err = gpio_remove_callback(gpio_dev, &gpio_callback);
	if (err) {
		mosh_error("Failed to remove GPIO callback, error: %d", err);
	}

	gpio_pin = GPIO_PIN_INVALID;

	return 0;
}

uint32_t gpio_count_get(void)
{
	return gpio_pulse_count;
}

void gpio_count_reset(void)
{
	gpio_pulse_count = 0;
}
