/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>
#include <misc/reboot.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>

#include "ble_event.h"

#define MODULE ble_state
#include "module_state_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_BLE_STATE_LOG_LEVEL);

#ifdef CONFIG_BT_PERIPHERAL
#define SECURITY_FAIL_TIMEOUT_MS \
	K_SECONDS(CONFIG_DESKTOP_BLE_SECURITY_FAIL_TIMEOUT_S)
#else
#define SECURITY_FAIL_TIMEOUT_MS 0
#endif

struct bond_find_data {
	bt_addr_le_t peer_address;
	u8_t peer_id;
	u8_t peer_count;
};

static struct k_delayed_work security_timeout;
static struct bt_conn *active_conn[CONFIG_BT_MAX_CONN];


static void security_timeout_fn(struct k_work *w)
{
	BUILD_ASSERT_MSG(!IS_ENABLED(CONFIG_BT_PERIPHERAL) ||
			 (ARRAY_SIZE(active_conn) == 1), "");
	BUILD_ASSERT_MSG(!IS_ENABLED(CONFIG_BT_PERIPHERAL) ||
			 (SECURITY_FAIL_TIMEOUT_MS > 0), "");
	__ASSERT_NO_MSG(active_conn[0]);
	bt_conn_disconnect(active_conn[0],
			   BT_HCI_ERR_REMOTE_USER_TERM_CONN);

	LOG_ERR("Security establishment failed - device disconnected");
}

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
		/* Assert one local identity holds exactly one bond.
		 * One local identity is unused.
		 */
		__ASSERT_NO_MSG(CONFIG_BT_MAX_PAIRED == CONFIG_BT_ID_MAX - 1);

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

	size_t i;

	for (i = 0; i < ARRAY_SIZE(active_conn); i++) {
		if (!active_conn[i]) {
			break;
		}
	}
	if (i >= ARRAY_SIZE(active_conn)) {
		k_panic();
	}
	active_conn[i] = conn;

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
		err = bt_conn_set_security(conn, BT_SECURITY_L2);
		if (err) {
			LOG_ERR("Failed to set security");
			goto disconnect;
		}
		k_delayed_work_submit(&security_timeout,
				      SECURITY_FAIL_TIMEOUT_MS);
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

	size_t i;

	for (i = 0; i < ARRAY_SIZE(active_conn); i++) {
		if (active_conn[i] == conn) {
			break;
		}
	}

	if (i == ARRAY_SIZE(active_conn)) {
		return;
	}

	active_conn[i] = NULL;

	struct ble_peer_event *event = new_ble_peer_event();

	event->id = conn;
	event->state = PEER_STATE_DISCONNECTED;
	EVENT_SUBMIT(event);

	if (IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
		k_delayed_work_cancel(&security_timeout);
	}
}

static struct bt_gatt_exchange_params exchange_params;

static void exchange_func(struct bt_conn *conn, u8_t err,
			  struct bt_gatt_exchange_params *params)
{
	LOG_INF("MTU exchange done");
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err bt_err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!bt_err) {
		LOG_INF("Security with %s level %u", log_strdup(addr), level);
	} else {
		LOG_ERR("Security failed: %s level %u err %d", log_strdup(addr),
			level, bt_err);
		return;		/* Let the timeout handle the failure. */
	}

	if (IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
		__ASSERT_NO_MSG(active_conn[0] == conn);
		k_delayed_work_cancel(&security_timeout);
	}

	struct ble_peer_event *event = new_ble_peer_event();
	event->id = conn;
	event->state = PEER_STATE_SECURED;
	EVENT_SUBMIT(event);

	struct bt_conn_info info;

	int err = bt_conn_get_info(conn, &info);
	if (err) {
		LOG_WRN("Cannot get conn info");
	} else {
		if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
		    (info.role == BT_CONN_ROLE_MASTER)) {
			exchange_params.func = exchange_func;
			err = bt_gatt_exchange_mtu(conn, &exchange_params);
			if (err) {
				LOG_ERR("MTU exchange failed");
			}
		}
	}
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

			if (IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
				k_delayed_work_init(&security_timeout,
						    security_timeout_fn);
			}
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
