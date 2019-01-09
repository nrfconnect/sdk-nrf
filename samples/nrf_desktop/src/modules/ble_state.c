/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>
#include <misc/reboot.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>

#include "ble_event.h"

#define MODULE ble_state
#include "module_state_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_BLE_STATE_LOG_LEVEL);


static void connected(struct bt_conn *conn, u8_t error)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (error) {
		struct ble_peer_event *event = new_ble_peer_event();
		event->id = conn;
		event->state = PEER_STATE_CONN_FAILED;
		EVENT_SUBMIT(event);

		LOG_WRN("Failed to connect to %s (%u)", log_strdup(addr),
			error);
		return;
	}

	LOG_INF("Connected to %s", log_strdup(addr));

	struct ble_peer_event *event = new_ble_peer_event();
	event->id = conn;
	event->state = PEER_STATE_CONNECTED;
	EVENT_SUBMIT(event);

	struct bt_conn_info info;

	int err = bt_conn_get_info(conn, &info);
	if (err) {
		LOG_ERR("Cannot get conn info");
		return;
	}

	if (info.role == BT_CONN_ROLE_SLAVE) {
		LOG_INF("Set security level");
		err = bt_conn_security(conn, BT_SECURITY_MEDIUM);
		if (err) {
			LOG_ERR("Failed to set security");
		}
	}
}

static void disconnected(struct bt_conn *conn, u8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Disconnected from %s (reason %u)", log_strdup(addr), reason);

	struct ble_peer_event *event = new_ble_peer_event();
	event->id = conn;
	event->state = PEER_STATE_DISCONNECTED;
	EVENT_SUBMIT(event);
}

static void security_changed(struct bt_conn *conn, bt_security_t level)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Security with %s level %u", log_strdup(addr), level);

	struct ble_peer_event *event = new_ble_peer_event();
	event->id = conn;
	event->state = PEER_STATE_SECURED;
	EVENT_SUBMIT(event);
}

static bool le_param_req(struct bt_conn *conn, struct bt_le_conn_param *param)
{
	LOG_INF("Conn parameters request:"
		"\n\tinterval (0x%04x, 0x%04x)\n\tsl %d\n\ttimeout %d",
		param->interval_min, param->interval_max,
		param->latency, param->timeout);

	/* Accept the request */
	return true;
}

static void le_param_updated(struct bt_conn *conn, u16_t interval,
			     u16_t latency, u16_t timeout)
{
	LOG_INF("Conn parameters updated:"
		"\n\tinterval 0x%04x\n\tlat %d\n\ttimeout %d\n",
		interval, latency, timeout);
}

static void bt_ready(int err)
{
	if (err) {
		LOG_ERR("Bluetooth initialization failed (err %d)", err);
		sys_reboot(SYS_REBOOT_WARM);
	}

	LOG_INF("Bluetooth initialized");

	module_set_state(MODULE_STATE_READY);
}

static int ble_state_init(void)
{
	static struct bt_conn_cb conn_callbacks = {
		.connected = connected,
		.disconnected = disconnected,
		.security_changed = security_changed,
		.le_param_req = le_param_req,
		.le_param_updated = le_param_updated,
	};
	bt_conn_cb_register(&conn_callbacks);

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
				LOG_ERR("Cannot initialize");
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
