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


struct bond_find_data {
	bt_addr_le_t peer_address;
	u8_t peer_id;
	u8_t peer_count;
};


static const struct bt_conn *active_conn;

static void bond_find(const struct bt_bond_info *info, void *user_data)
{
	struct bond_find_data *bond_find_data = user_data;

	if (bond_find_data->peer_id == bond_find_data->peer_count) {
		bt_addr_le_copy(&bond_find_data->peer_address, &info->addr);
	}

	__ASSERT_NO_MSG(bond_find_data->peer_count < UCHAR_MAX);
	bond_find_data->peer_count++;
}

static void connected(struct bt_conn *conn, u8_t error)
{
	char addr_str[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr_str, sizeof(addr_str));

	if (error) {
		struct ble_peer_event *event = new_ble_peer_event();
		event->id = conn;
		event->state = PEER_STATE_CONN_FAILED;
		EVENT_SUBMIT(event);

		LOG_WRN("Failed to connect to %s (%u)", log_strdup(addr_str),
			error);
		return;
	}

	LOG_INF("Connected to %s", log_strdup(addr_str));

	struct bt_conn_info info;

	int err = bt_conn_get_info(conn, &info);
	if (err) {
		LOG_WRN("Cannot get conn info");
		goto disconnect;
	}

	if (IS_ENABLED(CONFIG_BT_PERIPHERAL) &&
	    (info.role == BT_CONN_ROLE_SLAVE)) {
		/* Assert one identity holds exactly one bond. */
		__ASSERT_NO_MSG(CONFIG_BT_MAX_PAIRED == CONFIG_BT_ID_MAX);

		struct bond_find_data bond_find_data = {
			.peer_id = 0,
			.peer_count = 0,
		};
		bt_foreach_bond(info.id, bond_find, &bond_find_data);

		LOG_INF("Identity %u has %u bonds", info.id,
			bond_find_data.peer_count);
		if ((bond_find_data.peer_count > 0) &&
		    bt_addr_le_cmp(bt_conn_get_dst(conn),
				   &bond_find_data.peer_address)) {
			bt_addr_le_to_str(&bond_find_data.peer_address, addr_str,
					sizeof(addr_str));
			LOG_INF("Already bonded to %s", log_strdup(addr_str));
			goto disconnect;
		}
	}

	__ASSERT_NO_MSG(!active_conn);
	active_conn = conn;

	struct ble_peer_event *event = new_ble_peer_event();
	event->id = conn;
	event->state = PEER_STATE_CONNECTED;
	EVENT_SUBMIT(event);

	if (IS_ENABLED(CONFIG_BT_PERIPHERAL) &&
	    (info.role == BT_CONN_ROLE_SLAVE)) {
		/* Security must be enabled after peer event is sent.
		 * This is to make sure notification events are propagated
		 * in the right order.
		 */

		LOG_INF("Set security level");
		err = bt_conn_security(conn, BT_SECURITY_MEDIUM);
		if (err) {
			LOG_ERR("Failed to set security");
			goto disconnect;
		}
	}

	return;

disconnect:
	LOG_WRN("Disconnect");
	bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
}

static void disconnected(struct bt_conn *conn, u8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Disconnected from %s (reason %u)", log_strdup(addr), reason);

	if (conn == active_conn) {
		struct ble_peer_event *event = new_ble_peer_event();
		event->id = conn;
		event->state = PEER_STATE_DISCONNECTED;
		EVENT_SUBMIT(event);

		active_conn = NULL;
	}
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
