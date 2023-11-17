/*
 * Copyright (c) 2018-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>

#include <caf/events/ble_common_event.h>

#ifdef CONFIG_CAF_BLE_USE_LLPM
#include "sdc_hci_vs.h"
#endif /* CONFIG_CAF_BLE_USE_LLPM */

#define MODULE ble_state
#include <caf/events/module_state_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_CAF_BLE_STATE_LOG_LEVEL);


struct bond_find_data {
	const bt_addr_le_t *peer_address;
	bool peer_bonded;
	uint8_t bond_cnt;
};

static struct bt_conn *active_conn[CONFIG_BT_MAX_CONN];


static void bond_check_cb(const struct bt_bond_info *info, void *user_data)
{
	struct bond_find_data *data = user_data;

	data->bond_cnt++;
	if (!bt_addr_le_cmp(&info->addr, data->peer_address)) {
		data->peer_bonded = true;
	}

	if (IS_ENABLED(CONFIG_LOG)) {
		char addr_str[BT_ADDR_LE_STR_LEN];

		bt_addr_le_to_str(&info->addr, addr_str, sizeof(addr_str));
		LOG_INF("Already bonded to %s", addr_str);
	}
}

static void disconnect_peer(struct bt_conn *conn)
{
	int err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);

	if (err && (err != -ENOTCONN)) {
		LOG_ERR("Failed to disconnect peer (err=%d)", err);
		module_set_state(MODULE_STATE_ERROR);
	} else {
		LOG_INF("Peer disconnected");
	}
}

static void broadcast_init_conn_params(struct bt_conn *conn)
{
	struct bt_conn_info info;
	int err = bt_conn_get_info(conn, &info);

	if (err) {
		LOG_ERR("Cannot get conn info (%d)", err);
	} else {
		struct ble_peer_conn_params_event *event = new_ble_peer_conn_params_event();

		event->id = conn;
		event->interval_min = info.le.interval;
		event->interval_max = info.le.interval;
		event->latency = info.le.latency;
		event->timeout = info.le.timeout;
		event->updated = true;

		APP_EVENT_SUBMIT(event);
	}
}

static void connected(struct bt_conn *conn, uint8_t error)
{
	/* Make sure that connection will remain valid. */
	bt_conn_ref(conn);

	if (error) {
		struct ble_peer_event *event = new_ble_peer_event();

		event->id = conn;
		event->state = PEER_STATE_CONN_FAILED;
		APP_EVENT_SUBMIT(event);

		if (IS_ENABLED(CONFIG_LOG)) {
			char addr_str[BT_ADDR_LE_STR_LEN];

			bt_addr_le_to_str(bt_conn_get_dst(conn), addr_str, sizeof(addr_str));
			LOG_WRN("Failed to connect to %s (%u)", addr_str, error);
		}

		return;
	}

	if (IS_ENABLED(CONFIG_LOG)) {
		char addr_str[BT_ADDR_LE_STR_LEN];

		bt_addr_le_to_str(bt_conn_get_dst(conn), addr_str, sizeof(addr_str));
		LOG_INF("Connected to %s", addr_str);
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
	APP_EVENT_SUBMIT(event);

	broadcast_init_conn_params(conn);

	struct bt_conn_info info;

	int err = bt_conn_get_info(conn, &info);

	if (err) {
		LOG_WRN("Cannot get conn info");
		goto disconnect;
	}

	if (IS_ENABLED(CONFIG_BT_PERIPHERAL) &&
	    (info.role == BT_CONN_ROLE_PERIPHERAL)) {
		struct bond_find_data bond_find_data = {
			.peer_address = bt_conn_get_dst(conn),
			.peer_bonded = false,
			.bond_cnt = 0
		};

		bt_foreach_bond(info.id, bond_check_cb, &bond_find_data);

		LOG_INF("Identity %" PRIu8 " has %" PRIu8 " bonds",
			info.id, bond_find_data.bond_cnt);

		if (!bond_find_data.peer_bonded &&
		    (bond_find_data.bond_cnt >= CONFIG_CAF_BLE_STATE_MAX_LOCAL_ID_BONDS)) {
			goto disconnect;
		}
	}

	if (IS_ENABLED(CONFIG_CAF_BLE_STATE_SECURITY_REQ)) {
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
	}

	return;

disconnect:
	disconnect_peer(conn);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	if (IS_ENABLED(CONFIG_LOG)) {
		char addr_str[BT_ADDR_LE_STR_LEN];

		bt_addr_le_to_str(bt_conn_get_dst(conn), addr_str, sizeof(addr_str));
		LOG_INF("Disconnected from %s (reason %u)", addr_str, reason);
	}

	size_t i;

	for (i = 0; i < ARRAY_SIZE(active_conn); i++) {
		if (active_conn[i] == conn) {
			break;
		}
	}

	if (i == ARRAY_SIZE(active_conn)) {
		__ASSERT_NO_MSG(false);
		return;
	}

	active_conn[i] = NULL;

	struct ble_peer_event *event = new_ble_peer_event();

	event->id = conn;
	event->state = PEER_STATE_DISCONNECTED;
	APP_EVENT_SUBMIT(event);
}

static struct bt_gatt_exchange_params exchange_params;

static void exchange_func(struct bt_conn *conn, uint8_t err,
			  struct bt_gatt_exchange_params *params)
{
	LOG_INF("MTU exchange done");
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err bt_err)
{
	if (IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
		__ASSERT_NO_MSG(active_conn[0] == conn);
	}

	int err;

	if (IS_ENABLED(CONFIG_LOG)) {
		char addr_str[BT_ADDR_LE_STR_LEN];

		bt_addr_le_to_str(bt_conn_get_dst(conn), addr_str, sizeof(addr_str));

		if (!bt_err && (level >= BT_SECURITY_L2)) {
			LOG_INF("Security with %s level %u", addr_str, level);
		} else {
			LOG_WRN("Security with %s failed, level %u err %d",
				addr_str, level, bt_err);
		}
	}

	if (bt_err || (level < BT_SECURITY_L2)) {
		if (IS_ENABLED(CONFIG_CAF_BLE_STATE_SECURITY_REQ)) {
			disconnect_peer(conn);
		}

		return;
	}

	struct ble_peer_event *event = new_ble_peer_event();
	event->id = conn;
	event->state = PEER_STATE_SECURED;
	APP_EVENT_SUBMIT(event);

	if (IS_ENABLED(CONFIG_CAF_BLE_STATE_EXCHANGE_MTU)) {
		exchange_params.func = exchange_func;
		err = bt_gatt_exchange_mtu(conn, &exchange_params);
		if (err) {
			LOG_ERR("MTU exchange failed");
		}
	}
}

static bool le_param_req(struct bt_conn *conn, struct bt_le_conn_param *param)
{
	struct ble_peer_conn_params_event *event =
		new_ble_peer_conn_params_event();

	event->id = conn;
	event->interval_min = param->interval_min;
	event->interval_max = param->interval_max;
	event->latency = param->latency;
	event->timeout = param->timeout;
	event->updated = false;

	APP_EVENT_SUBMIT(event);

	return false;
}

static void le_param_updated(struct bt_conn *conn, uint16_t interval,
			     uint16_t latency, uint16_t timeout)
{
	struct ble_peer_conn_params_event *event =
		new_ble_peer_conn_params_event();

	event->id = conn;
	event->interval_min = interval;
	event->interval_max = interval;
	event->latency = latency;
	event->timeout = timeout;
	event->updated = true;

	APP_EVENT_SUBMIT(event);
}

static void bt_ready(int err)
{
	if (err) {
		LOG_ERR("Bluetooth initialization failed (err %d)", err);
		sys_reboot(SYS_REBOOT_WARM);
	}

	LOG_INF("Bluetooth initialized");

#ifdef CONFIG_CAF_BLE_USE_LLPM
	sdc_hci_cmd_vs_llpm_mode_set_t *p_cmd_enable;

	struct net_buf *buf = bt_hci_cmd_create(SDC_HCI_OPCODE_CMD_VS_LLPM_MODE_SET,
						sizeof(*p_cmd_enable));

	p_cmd_enable = net_buf_add(buf, sizeof(*p_cmd_enable));
	p_cmd_enable->enable = 1;

	err = bt_hci_cmd_send_sync(SDC_HCI_OPCODE_CMD_VS_LLPM_MODE_SET, buf, NULL);
	if (err) {
		LOG_ERR("Error enabling LLPM (err: %d)", err);
	} else {
		LOG_INF("LLPM enabled");
	}
#endif /* CONFIG_CAF_BLE_USE_LLPM */

	module_set_state(MODULE_STATE_READY);
}

static int ble_state_init(void)
{
	BUILD_ASSERT(!IS_ENABLED(CONFIG_BT_PERIPHERAL) ||
		     (ARRAY_SIZE(active_conn) == 1));

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

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_module_state_event(aeh)) {
		const struct module_state_event *event =
			cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			if (ble_state_init()) {
				LOG_ERR("Cannot initialize");
				module_set_state(MODULE_STATE_ERROR);
			}
		}

		return false;
	}

	if (is_ble_peer_event(aeh)) {
		const struct ble_peer_event *event = cast_ble_peer_event(aeh);

		switch (event->state) {
		case PEER_STATE_CONN_FAILED:
		case PEER_STATE_DISCONNECTED:
			/* Connection object is no longer in use. */
			bt_conn_unref(event->id);
			break;

		default:
			/* Ignore. */
			break;
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
APP_EVENT_SUBSCRIBE_FINAL(MODULE, ble_peer_event);
