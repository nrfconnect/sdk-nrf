/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>

#include "passkey_event.h"

#define MODULE ble_passkey
#include <caf/events/module_state_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_BLE_PASSKEY_LOG_LEVEL);

static bool passkey_input;
static struct bt_conn *active_conn;

BUILD_ASSERT(CONFIG_BT_MAX_CONN == 1);


static void send_passkey_req(bool active)
{
	if (passkey_input != active) {
		struct passkey_req_event *event = new_passkey_req_event();

		event->active = active;
		APP_EVENT_SUBMIT(event);

		passkey_input = active;
	}
}

static void connected(struct bt_conn *conn, uint8_t error)
{
	__ASSERT_NO_MSG(active_conn == NULL);

	if (!error) {
		active_conn = conn;
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	send_passkey_req(false);

	active_conn = NULL;
}

static void auth_passkey_entry(struct bt_conn *conn)
{
	send_passkey_req(true);
	LOG_INF("Passkey input started");
}

static void auth_cancel(struct bt_conn *conn)
{
	send_passkey_req(false);
	LOG_INF("Authentication cancelled");
}

static int ble_passkey_init(void)
{
	static struct bt_conn_cb conn_callbacks = {
		.connected = connected,
		.disconnected = disconnected,
	};
	bt_conn_cb_register(&conn_callbacks);

	static const struct bt_conn_auth_cb conn_auth_callbacks = {
		.passkey_entry = auth_passkey_entry,
		.cancel = auth_cancel,
	};

	return bt_conn_auth_cb_register(&conn_auth_callbacks);
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_module_state_event(aeh)) {
		const struct module_state_event *event =
			cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			if (ble_passkey_init()) {
				LOG_ERR("Cannot initialize");
				module_set_state(MODULE_STATE_ERROR);
			} else {
				module_set_state(MODULE_STATE_READY);
			}
		}

		return false;
	}

	if (is_passkey_input_event(aeh)) {
		const struct passkey_input_event *event = cast_passkey_input_event(aeh);

		if (passkey_input) {
			int err = bt_conn_auth_passkey_entry(active_conn, event->passkey);

			if (err) {
				LOG_ERR("Problem entering passkey (err %d)", err);
			}

			passkey_input = false;
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
APP_EVENT_SUBSCRIBE(MODULE, passkey_input_event);
