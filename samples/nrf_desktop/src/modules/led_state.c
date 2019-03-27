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

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_LED_STATE_LOG_LEVEL);

#include "led_state.h"
#include "led_state_def.h"


static enum led_system_state system_state = LED_SYSTEM_STATE_IDLE;

static bool connected;
static enum peer_operation peer_op = PEER_OPERATION_CANCEL;
static u8_t cur_peer_id;
static u8_t tmp_peer_id;

static void load_peer_state_led(void)
{
	enum led_peer_state state = LED_PEER_STATE_DISCONNECTED;
	u8_t peer_id = cur_peer_id;

	switch (peer_op) {
	case PEER_OPERATION_SELECT:
		peer_id = tmp_peer_id;
		state = LED_PEER_STATE_CONFIRM_SELECT;
		break;
	case PEER_OPERATION_ERASE:
		state = LED_PEER_STATE_CONFIRM_ERASE;
		break;
	case PEER_OPERATION_CANCEL:
		if (connected) {
			state = LED_PEER_STATE_CONNECTED;
		}
		break;
	case PEER_OPERATION_SELECTED:
	case PEER_OPERATION_ERASED:
	default:
		__ASSERT_NO_MSG(false);
		break;
	}

	struct led_event *event = new_led_event();

	event->led_id = led_map[LED_ID_PEER_STATE];
	event->mode   = led_peer_state_effect[state].mode;
	event->period = led_peer_state_effect[state].period;
	event->color  = led_peer_state_color[peer_id];
	EVENT_SUBMIT(event);
}


static void load_system_state_led(void)
{
	struct led_event *event = new_led_event();

	event->led_id = led_map[LED_ID_SYSTEM_STATE];
	event->mode   = led_system_state_effect[system_state].mode;
	event->period = led_system_state_effect[system_state].period;
	event->color  = led_system_state_color[system_state];
	EVENT_SUBMIT(event);
}

static void set_system_state_led(enum led_system_state state)
{
	__ASSERT_NO_MSG(state < LED_SYSTEM_STATE_COUNT);

	if (system_state != LED_SYSTEM_STATE_ERROR) {
		system_state = state;
		load_system_state_led();
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
		case PEER_STATE_CONN_FAILED:
			/* Ignore */
			break;
		default:
			__ASSERT_NO_MSG(false);
			break;
		}
		load_peer_state_led();

		return false;
	}

	if (is_ble_peer_operation_event(eh)) {
		struct ble_peer_operation_event *event =
			cast_ble_peer_operation_event(eh);

		switch (event->op)  {
		case PEER_OPERATION_SELECT:
			tmp_peer_id = event->arg;
		case PEER_OPERATION_ERASE:
			peer_op = event->op;
			break;
		case PEER_OPERATION_SELECTED:
			cur_peer_id = tmp_peer_id;
			/* Fall through */
		case PEER_OPERATION_ERASED:
		case PEER_OPERATION_CANCEL:
			peer_op = PEER_OPERATION_CANCEL;
			break;
		default:
			__ASSERT_NO_MSG(false);
			break;
		}
		load_peer_state_led();

		return false;
	}

	if (is_battery_state_event(eh)) {
		struct battery_state_event *event = cast_battery_state_event(eh);

		switch (event->state) {
		case BATTERY_STATE_CHARGING:
			set_system_state_led(LED_SYSTEM_STATE_CHARGING);
			break;
		case BATTERY_STATE_IDLE:
			set_system_state_led(LED_SYSTEM_STATE_IDLE);
			break;
		case BATTERY_STATE_ERROR:
			set_system_state_led(LED_SYSTEM_STATE_ERROR);
			break;
		default:
			__ASSERT_NO_MSG(false);
			break;
		}

		return false;
	}

	if (is_module_state_event(eh)) {
		struct module_state_event *event = cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			static_assert(LED_ID_COUNT <= CONFIG_DESKTOP_LED_COUNT,
				      "Not enough LEDs configured");
			load_system_state_led();
			load_peer_state_led();
		} else if (event->state == MODULE_STATE_ERROR) {
			set_system_state_led(LED_SYSTEM_STATE_ERROR);
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
EVENT_SUBSCRIBE(MODULE, ble_peer_operation_event);
EVENT_SUBSCRIBE(MODULE, battery_state_event);
