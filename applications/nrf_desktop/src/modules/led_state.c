/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#define MODULE led_state
#include <caf/events/module_state_event.h>

#include <caf/events/led_event.h>
#include <caf/events/ble_common_event.h>
#include "battery_event.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_LED_STATE_LOG_LEVEL);

#include "led_state.h"
#include CONFIG_DESKTOP_LED_STATE_DEF_PATH


static enum led_system_state system_state = LED_SYSTEM_STATE_IDLE;

static uint8_t connected;
static bool peer_search;
static enum peer_operation peer_op = PEER_OPERATION_CANCEL;
static uint8_t cur_peer_id;


static void load_peer_state_led(void)
{
	enum led_peer_state state = LED_PEER_STATE_DISCONNECTED;
	uint8_t peer_id = cur_peer_id;

	switch (peer_op) {
	case PEER_OPERATION_SELECT:
		state = LED_PEER_STATE_CONFIRM_SELECT;
		break;
	case PEER_OPERATION_ERASE:
		state = LED_PEER_STATE_CONFIRM_ERASE;
		break;
	case PEER_OPERATION_ERASE_ADV:
		state = LED_PEER_STATE_ERASE_ADV;
		break;
	case PEER_OPERATION_SELECTED:
	case PEER_OPERATION_ERASE_ADV_CANCEL:
	case PEER_OPERATION_ERASED:
	case PEER_OPERATION_CANCEL:
	case PEER_OPERATION_SCAN_REQUEST:
		if (peer_search) {
			state = LED_PEER_STATE_PEER_SEARCH;
		} else if (connected > 0) {
			state = LED_PEER_STATE_CONNECTED;
		}
		break;
	default:
		__ASSERT_NO_MSG(false);
		break;
	}

	if (led_map[LED_ID_PEER_STATE] == LED_UNAVAILABLE) {
		return;
	}

	__ASSERT_NO_MSG(peer_id < LED_PEER_COUNT);
	__ASSERT_NO_MSG(state < LED_PEER_STATE_COUNT);

	struct led_event *event = new_led_event();

	event->led_id = led_map[LED_ID_PEER_STATE];
	event->led_effect = &led_peer_state_effect[peer_id][state];
	APP_EVENT_SUBMIT(event);
}

static void load_system_state_led(void)
{
	if (led_map[LED_ID_SYSTEM_STATE] == LED_UNAVAILABLE) {
		return;
	}

	struct led_event *event = new_led_event();

	event->led_id = led_map[LED_ID_SYSTEM_STATE];
	event->led_effect = &led_system_state_effect[system_state];
	APP_EVENT_SUBMIT(event);
}

static void set_system_state_led(enum led_system_state state)
{
	__ASSERT_NO_MSG(state < LED_SYSTEM_STATE_COUNT);

	if (system_state != LED_SYSTEM_STATE_ERROR) {
		system_state = state;
		load_system_state_led();
	}
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (IS_ENABLED(CONFIG_CAF_BLE_COMMON_EVENTS) && is_ble_peer_event(aeh)) {
		struct ble_peer_event *event = cast_ble_peer_event(aeh);

		switch (event->state)  {
		case PEER_STATE_CONNECTED:
			__ASSERT_NO_MSG(connected < UINT8_MAX);
			connected++;
			break;
		case PEER_STATE_DISCONNECTED:
			__ASSERT_NO_MSG(connected > 0);
			connected--;
			break;
		case PEER_STATE_SECURED:
		case PEER_STATE_CONN_FAILED:
		case PEER_STATE_DISCONNECTING:
			/* Ignore */
			break;
		default:
			__ASSERT_NO_MSG(false);
			break;
		}
		load_peer_state_led();

		return false;
	}

	if (IS_ENABLED(CONFIG_CAF_BLE_COMMON_EVENTS) && is_ble_peer_search_event(aeh)) {
		struct ble_peer_search_event *event = cast_ble_peer_search_event(aeh);

		peer_search = event->active;
		load_peer_state_led();

		return false;
	}

	if (IS_ENABLED(CONFIG_CAF_BLE_COMMON_EVENTS) && is_ble_peer_operation_event(aeh)) {
		struct ble_peer_operation_event *event =
			cast_ble_peer_operation_event(aeh);

		cur_peer_id = event->bt_app_id;
		peer_op = event->op;
		load_peer_state_led();

		return false;
	}

	if (is_battery_state_event(aeh)) {
		struct battery_state_event *event = cast_battery_state_event(aeh);

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

	if (is_module_state_event(aeh)) {
		struct module_state_event *event = cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			load_system_state_led();
			if (!IS_ENABLED(CONFIG_CAF_BLE_COMMON_EVENTS)) {
				/* BLE peer state will not be notified. Preload the LED. */
				load_peer_state_led();
			}
		} else if (event->state == MODULE_STATE_ERROR) {
			set_system_state_led(LED_SYSTEM_STATE_ERROR);
		}
		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
#ifdef CONFIG_CAF_BLE_COMMON_EVENTS
APP_EVENT_SUBSCRIBE(MODULE, ble_peer_event);
APP_EVENT_SUBSCRIBE(MODULE, ble_peer_search_event);
APP_EVENT_SUBSCRIBE(MODULE, ble_peer_operation_event);
#endif /* CONFIG_CAF_BLE_COMMON_EVENTS */
APP_EVENT_SUBSCRIBE(MODULE, battery_state_event);
