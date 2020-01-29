/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>

#include <device.h>
#include <drivers/gpio.h>

#include "port_state.h"
#include "port_state_def.h"

#include "power_event.h"

#define MODULE board
#include "module_state_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_BOARD_LOG_LEVEL);


static int port_setup(const char *name,
		      const struct pin_state pin_state[],
		      size_t cnt)
{
	struct device *gpio_dev = device_get_binding(name);
	int err = 0;

	if (!gpio_dev) {
		LOG_ERR("Cannot bind %s", name);
		return -ENXIO;
	}

	for (size_t i = 0; i < cnt; i++) {
		err = gpio_pin_configure(gpio_dev,
					 pin_state[i].pin,
					 GPIO_DIR_OUT);
		if (!err) {
			err = gpio_pin_write(gpio_dev,
					     pin_state[i].pin,
					     pin_state[i].val);
		} else {
			LOG_ERR("Cannot configure pin %u on %s",
				    pin_state[i].pin, name);
			break;
		}
	}

	return err;
}

static int ports_setup(const struct port_state port_state[], size_t cnt)
{
	int err = 0;

	for (size_t i = 0; i < cnt; i++) {
		err = port_setup(port_state[i].name,
				 port_state[i].ps,
				 port_state[i].ps_count);
		if (err) {
			break;
		}
	}

	return err;
}

static void turn_board_on(void)
{
	int err = ports_setup(port_state_on, ARRAY_SIZE(port_state_on));
	if (err) {
		goto error;
	}

	module_set_state(MODULE_STATE_READY);

	return;

error:
	LOG_ERR("Cannot initialize board");
	module_set_state(MODULE_STATE_ERROR);
}

static void turn_board_off(void)
{
	int err = ports_setup(port_state_off, ARRAY_SIZE(port_state_off));
	if (err) {
		goto error;
	}

	module_set_state(MODULE_STATE_OFF);

	return;

error:
	LOG_ERR("Cannot suspend board");
	module_set_state(MODULE_STATE_ERROR);
}

static bool event_handler(const struct event_header *eh)
{
	static bool initialized;

	if (is_module_state_event(eh)) {
		const struct module_state_event *event = cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			turn_board_on();
		}

		return false;
	}

	if (is_wake_up_event(eh)) {
		if (!initialized) {
			initialized = true;

			turn_board_on();
		}
		return false;
	}

	if (is_power_down_event(eh)) {
		const struct power_down_event *event = cast_power_down_event(eh);

		/* Do not cut off leds power on error */
		if (event->error) {
			return false;
		}

		if (initialized) {
			initialized = false;

			turn_board_off();
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, power_down_event);
EVENT_SUBSCRIBE_EARLY(MODULE, module_state_event);
EVENT_SUBSCRIBE(MODULE, wake_up_event);
