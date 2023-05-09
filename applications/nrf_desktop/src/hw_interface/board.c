/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#include "port_state.h"
#include "port_state_def.h"

#include <caf/events/power_event.h>

#define MODULE board
#include <caf/events/module_state_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_BOARD_LOG_LEVEL);


static int port_setup(const struct device *port,
		      const struct pin_state *pin_state,
		      size_t cnt)
{
	int err = 0;

	if (!device_is_ready(port)) {
		LOG_ERR("Port %s device not ready", port->name);
		return -ENODEV;
	}

	for (size_t i = 0; i < cnt; i++) {
		err = gpio_pin_configure(port,
					 pin_state[i].pin,
					 GPIO_OUTPUT);
		if (!err) {
			err = gpio_pin_set_raw(port,
					       pin_state[i].pin,
					       pin_state[i].val);
		} else {
			LOG_ERR("Cannot configure pin %u on %s",
				    pin_state[i].pin, port->name);
			break;
		}
	}

	return err;
}

static int ports_setup(const struct port_state *port_state, size_t cnt)
{
	int err = 0;

	for (size_t i = 0; i < cnt; i++) {
		err = port_setup(port_state[i].port,
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

static bool app_event_handler(const struct app_event_header *aeh)
{
	static bool initialized;

	if (is_module_state_event(aeh)) {
		const struct module_state_event *event = cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			turn_board_on();
		}

		return false;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_BOARD_PM_EVENTS) &&
	    is_wake_up_event(aeh)) {
		if (!initialized) {
			initialized = true;

			turn_board_on();
		}
		return false;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_BOARD_PM_EVENTS) &&
	    is_power_down_event(aeh)) {
		const struct power_down_event *event = cast_power_down_event(aeh);

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
APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE_EARLY(MODULE, module_state_event);
#if CONFIG_DESKTOP_BOARD_PM_EVENTS
APP_EVENT_SUBSCRIBE(MODULE, power_down_event);
APP_EVENT_SUBSCRIBE(MODULE, wake_up_event);
#endif
