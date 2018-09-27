/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#include <zephyr.h>

#define MODULE led_state
#include "module_state_event.h"

#include "led_event.h"
#include "ble_event.h"

#define SYS_LOG_DOMAIN	MODULE_NAME
#define SYS_LOG_LEVEL	CONFIG_DESKTOP_SYS_LOG_STATE_STATE_LEVEL
#include <logging/sys_log.h>

#include "led_state.h"

static void set_leds(enum system_state state)
{
	const struct led_config *cfg = led_config[state];

	for (size_t i = 0; i < CONFIG_DESKTOP_LED_COUNT; i++) {
		struct led_event *event = new_led_event();

		if (event) {
			event->led_id = i;
			event->mode = cfg[i].mode;
			event->color = cfg[i].color;
			event->period = cfg[i].period;
			EVENT_SUBMIT(event);
		}
	}
}

static bool event_handler(const struct event_header *eh)
{
	if (is_ble_peer_event(eh)) {
		struct ble_peer_event *event = cast_ble_peer_event(eh);

		if (event->state == PEER_STATE_CONNECTED) {
			set_leds(SYSTEM_STATE_CONNECTED);
		} else if (event->state == PEER_STATE_DISCONNECTED) {
			set_leds(SYSTEM_STATE_DISCONNECTED);
		}
		return false;
	}

	if (is_module_state_event(eh)) {
		struct module_state_event *event = cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			set_leds(SYSTEM_STATE_DISCONNECTED);
		}
		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE(MODULE, ble_peer_event);
