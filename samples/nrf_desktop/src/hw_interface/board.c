/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

/* This is a temporary file utilizing SR3 Shield HW. */

#include <zephyr.h>

#include <misc/printk.h>

#include <device.h>
#include <gpio.h>

#include "module_state_event.h"

#include "power_event.h"


#define MODULE		board
#define MODULE_NAME	STRINGIFY(MODULE)

#define SYS_LOG_DOMAIN	MODULE_NAME
#define SYS_LOG_LEVEL	CONFIG_DESKTOP_SYS_LOG_BOARD_MODULE_LEVEL
#include <logging/sys_log.h>


static bool active;


static int turn_board_on(void)
{
	int err = 0;

	struct device *gpio_dev =
		device_get_binding(CONFIG_GPIO_NRF5_P0_DEV_NAME);

	if (!gpio_dev) {
		SYS_LOG_ERR("cannot get GPIO device");
		return -1;
	}

	/* TODO below configures SR3 Shield */

	/* Turn on IO VCC */
	err |= gpio_pin_configure(gpio_dev, 15, GPIO_DIR_OUT);
	err |= gpio_pin_write(gpio_dev, 15, 0);
	if (err) {
		return -1;
	}

	/* IR LED */
	err |= gpio_pin_configure(gpio_dev, 23, GPIO_DIR_OUT);
	err |= gpio_pin_write(gpio_dev, 23, 0);
	if (err) {
		return -1;
	}

	/* indicator LED */
	err |= gpio_pin_configure(gpio_dev,  4, GPIO_DIR_OUT); /* led1 */
	err |= gpio_pin_write(gpio_dev, 4, 0);

	/* Disable unused pins */
	err |= gpio_pin_configure(gpio_dev, 24, GPIO_DIR_IN); /* wu irq */
	err |= gpio_pin_configure(gpio_dev, 16, GPIO_DIR_IN); /* led2 */
	err |= gpio_pin_configure(gpio_dev,  2, GPIO_DIR_IN); /* i2c 2 */
	err |= gpio_pin_configure(gpio_dev, 12, GPIO_DIR_IN); /* i2c 2 */
	if (err) {
		return -1;
	}

	module_set_state("ready");

	return 0;
}

static int turn_board_off(void)
{
	int err = 0;

	struct device *gpio_dev
		= device_get_binding(CONFIG_GPIO_NRF5_P0_DEV_NAME);

	if (!gpio_dev) {
		SYS_LOG_ERR("cannot get GPIO device");
		return -1;
	}

	/* TODO below configures SR3 Shield */

	/* Turn off IO VCC */
	err |= gpio_pin_configure(gpio_dev, 15, GPIO_DIR_OUT);
	err |= gpio_pin_write(gpio_dev, 15, 1);
	if (err) {
		return -1;
	}

	/* IR LED */
	err |= gpio_pin_configure(gpio_dev, 23, GPIO_DIR_OUT);
	err |= gpio_pin_write(gpio_dev, 23, 0);
	if (err) {
		return -1;
	}

	/* Disable unused pins */
	err |= gpio_pin_configure(gpio_dev, 24, GPIO_DIR_IN); /* wu irq */
	err |= gpio_pin_configure(gpio_dev,  4, GPIO_DIR_IN); /* led1 */
	err |= gpio_pin_configure(gpio_dev, 16, GPIO_DIR_IN); /* led2 */
	err |= gpio_pin_configure(gpio_dev,  2, GPIO_DIR_IN); /* i2c 2 */
	err |= gpio_pin_configure(gpio_dev, 12, GPIO_DIR_IN); /* i2c 2 */
	if (err) {
		return -1;
	}

	module_set_state("off");

	return 0;
}

static bool event_handler(const struct event_header *eh)
{
	if (is_module_state_event(eh)) {
		struct module_state_event *event = cast_module_state_event(eh);

		if (check_state(event, "main", "ready")) {
			active = true;
			if (turn_board_on()) {
				SYS_LOG_ERR("cannot initialize board");
			}
		}
		return false;
	}

	if (is_power_down_event(eh)) {
		if (active) {
			if (turn_board_off()) {
				SYS_LOG_ERR("cannot suspend board");
			}
			active = false;
		}
		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE_EARLY(MODULE, module_state_event);
EVENT_SUBSCRIBE(MODULE, power_down_event);
