/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>

#define MODULE led_state
#include "module_state_event.h"

#include "led_event.h"
#include "ble_event.h"
#include "battery_event.h"

#define SYS_LOG_DOMAIN	MODULE_NAME
#define SYS_LOG_LEVEL	CONFIG_DESKTOP_SYS_LOG_STATE_STATE_LEVEL
#include <logging/sys_log.h>

#include "led_state.h"


static bool error;
static bool connected;
static bool charging;

static void set_leds(void)
{
	enum system_state state = SYSTEM_STATE_ERROR;

	if (!error) {
		if (!connected && !charging) {
			state = SYSTEM_STATE_DISCONNECTED;
		} else if (connected && !charging) {
			state = SYSTEM_STATE_CONNECTED;
		} else if (!connected && charging) {
			state = SYSTEM_STATE_DISCONNECTED_CHARGING;
		} else if (connected && charging) {
			state = SYSTEM_STATE_CONNECTED_CHARGING;
		}
	}

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

		switch (event->state)  {
		case PEER_STATE_SECURED:
		case PEER_STATE_CONNECTED:
			connected = true;
			break;
		case PEER_STATE_DISCONNECTED:
			connected = false;
			break;
		default:
			error = true;
			break;
		}
		set_leds();

		return false;
	}

	if (is_battery_state_event(eh)) {
		struct battery_state_event *event = cast_battery_state_event(eh);

		switch (event->state) {
		case BATTERY_STATE_CHARGING:
			charging = true;
			break;
		case BATTERY_STATE_IDLE:
			charging = false;
			break;
		default:
			error = true;
			break;
		}
		set_leds();

		return false;
	}

	if (is_module_state_event(eh)) {
		struct module_state_event *event = cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			set_leds();
		} else if (event->state == MODULE_STATE_ERROR) {
			error = true;
			set_leds();
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
EVENT_SUBSCRIBE(MODULE, battery_state_event);
