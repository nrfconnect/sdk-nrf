/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#include <zephyr.h>

#include <device.h>
#include <board.h>
#include <gpio.h>

#include "power_event.h"

#define MODULE board
#include "module_state_event.h"

#define SYS_LOG_DOMAIN	MODULE_NAME
#define SYS_LOG_LEVEL	CONFIG_DESKTOP_SYS_LOG_BOARD_MODULE_LEVEL
#include <logging/sys_log.h>


static int turn_board_on(void)
{
	int err = 0;

	struct device *gpio_dev =
		device_get_binding(CONFIG_GPIO_NRF5_P0_DEV_NAME);

	if (!gpio_dev) {
		SYS_LOG_ERR("cannot get GPIO device");
		return -1;
	}

	/* Turn on LED indicator */
	err |= gpio_pin_configure(gpio_dev, 26, GPIO_DIR_OUT);
	err |= gpio_pin_write(gpio_dev, 26, 1);
	if (err) {
		return -1;
	}

	module_set_state(MODULE_STATE_READY);

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

	/* Turn off LED indicator */
	err |= gpio_pin_configure(gpio_dev, 26, GPIO_DIR_OUT);
	err |= gpio_pin_write(gpio_dev, 26, 0);
	if (err) {
		return -1;
	}

	module_set_state(MODULE_STATE_OFF);

	return 0;
}

static bool event_handler(const struct event_header *eh)
{
	static bool initialized;

	if (is_module_state_event(eh)) {
		struct module_state_event *event = cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			if (turn_board_on()) {
				SYS_LOG_ERR("cannot initialize board");
			}
		}
		return false;
	}

	if (is_power_down_event(eh)) {
		__ASSERT_NO_MSG(initialized);
		initialized = false;

		if (turn_board_off()) {
			SYS_LOG_ERR("cannot suspend board");
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
