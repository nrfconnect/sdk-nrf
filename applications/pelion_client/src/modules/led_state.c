/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>

#define MODULE led_state
#include <caf/events/module_state_event.h>
#include <caf/events/led_event.h>

#include "net_event.h"
#include "pelion_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_PELION_CLIENT_LED_STATE_LOG_LEVEL);

#include "led_state.h"
#include "led_state_def.h"


static enum net_state net_state = NET_STATE_DISABLED;
static enum pelion_state pelion_state = PELION_STATE_DISABLED;
static bool system_error;


static void load_pelion_state_led(void)
{
	if (led_map[LED_ID_PELION_STATE] == LED_UNAVAILABLE) {
		return;
	}

	struct led_event *event = new_led_event();

	event->led_id = led_map[LED_ID_PELION_STATE];
	if (!system_error) {
		event->led_effect = &led_pelion_state_effect[pelion_state];
	} else {
		event->led_effect = &led_system_error_effect;
	}
	EVENT_SUBMIT(event);
}

static void load_net_state_led(void)
{
	if (led_map[LED_ID_NET_STATE] == LED_UNAVAILABLE) {
		return;
	}

	struct led_event *event = new_led_event();

	event->led_id = led_map[LED_ID_NET_STATE];
	if (!system_error) {
		event->led_effect = &led_net_state_effect[net_state];
	} else {
		event->led_effect = &led_system_error_effect;
	}
	EVENT_SUBMIT(event);
}

static bool event_handler(const struct event_header *eh)
{
	if (is_module_state_event(eh)) {
		struct module_state_event *event = cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(leds), MODULE_STATE_READY)) {
			load_pelion_state_led();
			load_net_state_led();
		} else if (event->state == MODULE_STATE_ERROR) {
			if (!system_error) {
				system_error = true;
				load_pelion_state_led();
				load_net_state_led();
			}
		}
		return false;
	}

	if (is_net_state_event(eh)) {
		const struct net_state_event *event = cast_net_state_event(eh);

		net_state = event->state;
		load_net_state_led();

		return false;
	}

	if (is_pelion_state_event(eh)) {
		const struct pelion_state_event *event = cast_pelion_state_event(eh);

		pelion_state = event->state;
		load_pelion_state_led();

		return false;
	}

	/* Event not handled but subscribed. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE(MODULE, net_state_event);
EVENT_SUBSCRIBE(MODULE, pelion_state_event);
