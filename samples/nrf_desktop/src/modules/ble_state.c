/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>

#include <misc/reboot.h>

#include <settings/settings.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>

#include "ble_event.h"

#define MODULE ble_state
#include "module_state_event.h"

#define SYS_LOG_DOMAIN	MODULE_NAME
#define SYS_LOG_LEVEL	CONFIG_DESKTOP_SYS_LOG_BLE_STATE_LEVEL
#include <logging/sys_log.h>


static void connected(struct bt_conn *conn, u8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		SYS_LOG_ERR("Failed to connect to %s (%u)", addr, err);
		return;
	}

	SYS_LOG_INF("Connected to %s", addr);

	err = bt_conn_security(conn, BT_SECURITY_MEDIUM);
	if (err) {
		SYS_LOG_ERR("Failed to set security");
	}
}

static void disconnected(struct bt_conn *conn, u8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	SYS_LOG_INF("Disconnected from %s (reason %u)", addr, reason);

	struct ble_peer_event *event = new_ble_peer_event();

	if (event) {
		event->address = *bt_conn_get_dst(conn);
		event->connected = false;
		EVENT_SUBMIT(event);
	}
}

static void security_changed(struct bt_conn *conn, bt_security_t level)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	SYS_LOG_INF("Security with %s level %u", addr, level);

	struct ble_peer_event *event = new_ble_peer_event();

	if (event) {
		event->address = *bt_conn_get_dst(conn);
		event->connected = true;
		EVENT_SUBMIT(event);
	}
}

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	SYS_LOG_INF("Pairing cancelled: %s", addr);
}

static void bt_ready(int err)
{
	if (err) {
		SYS_LOG_ERR("Bluetooth initialization failed (err %d)", err);
		sys_reboot(SYS_REBOOT_WARM);
	}

	SYS_LOG_INF("Bluetooth initialized");

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		if (settings_load()) {
			SYS_LOG_ERR("Cannot load settings");
			sys_reboot(SYS_REBOOT_WARM);
		}
		SYS_LOG_INF("Settings loaded");
	}

	module_set_state(MODULE_STATE_READY);
}

static int ble_state_init(void)
{
	static struct bt_conn_cb conn_callbacks = {
		.connected = connected,
		.disconnected = disconnected,
		.security_changed = security_changed,
	};
	bt_conn_cb_register(&conn_callbacks);

	static struct bt_conn_auth_cb auth_cb_display = {
		.passkey_display = NULL,
		.passkey_entry = NULL,
		.cancel = auth_cancel,
	};
	int err = bt_conn_auth_cb_register(&auth_cb_display);

	if (err) {
		__ASSERT_NO_MSG(false);
	}

	return bt_enable(bt_ready);
}

static bool event_handler(const struct event_header *eh)
{
	if (is_module_state_event(eh)) {
		struct module_state_event *event = cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			if (ble_state_init()) {
				SYS_LOG_ERR("cannot initialize");
			}
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
