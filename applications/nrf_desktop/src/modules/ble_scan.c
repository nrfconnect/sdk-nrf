/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/gatt_dm.h>
#include <bluetooth/scan.h>
#include <string.h>

#define MODULE ble_scan
#include "module_state_event.h"
#include "hid_event.h"

#include "ble_event.h"
#include "ble_scan.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_BLE_SCANNING_LOG_LEVEL);

#define SCAN_TRIG_CHECK_MS    K_SECONDS(1)
#define SCAN_TRIG_TIMEOUT_MS  K_SECONDS(CONFIG_DESKTOP_BLE_SCAN_START_TIMEOUT_S)
#define SCAN_DURATION_MS      K_SECONDS(CONFIG_DESKTOP_BLE_SCAN_DURATION_S)

struct bond_find_data {
	bt_addr_le_t peer_address[CONFIG_BT_MAX_PAIRED];
	u8_t conn_count;
	u8_t peer_count;
};


static int scan_counter;
static struct bt_conn *discovering_peer_conn;
static struct k_delayed_work scan_start_trigger;
static struct k_delayed_work scan_stop_trigger;

static void bond_find(const struct bt_bond_info *info, void *user_data)
{
	struct bond_find_data *bond_find_data = user_data;

	bt_addr_le_copy(&bond_find_data->peer_address[bond_find_data->peer_count],
			&info->addr);

	struct bt_conn *conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT,
						      &info->addr);
	if (conn) {
		bond_find_data->conn_count++;
		bt_conn_unref(conn);
	}

	__ASSERT_NO_MSG(bond_find_data->peer_count < UCHAR_MAX);
	bond_find_data->peer_count++;
}

static void scan_stop(void)
{
	int err = bt_scan_stop();

	if (err == -EALREADY) {
		LOG_WRN("Active scan already disabled");
	} else if (err) {
		LOG_ERR("Stop LE scan failed (err %d)", err);
		module_set_state(MODULE_STATE_ERROR);
		return;
	} else {
		LOG_INF("Scan stopped");
	}

	struct bond_find_data bond_find_data = {
		.conn_count = 0,
		.peer_count = 0,
	};

	bt_foreach_bond(BT_ID_DEFAULT, bond_find,
			&bond_find_data);
	if (bond_find_data.conn_count < CONFIG_BT_MAX_CONN) {
		scan_counter = 0;
		k_delayed_work_submit(&scan_start_trigger, SCAN_TRIG_CHECK_MS);
		k_delayed_work_cancel(&scan_stop_trigger);
	}
}

static int configure_filters(void)
{
	bt_scan_filter_remove_all();
	static_assert(CONFIG_BT_MAX_PAIRED <= CONFIG_BT_SCAN_ADDRESS_CNT,
		      "Insufficient number of address filters");
	static_assert(ARRAY_SIZE(bt_peripherals) <= CONFIG_BT_SCAN_SHORT_NAME_CNT,
		      "Insufficient number of short name filers");

	struct bond_find_data bond_find_data = {
		.conn_count = 0,
		.peer_count = 0,
	};
	bt_foreach_bond(BT_ID_DEFAULT, bond_find, &bond_find_data);

	u8_t filter_mode = 0;
	int err;

	if (bond_find_data.peer_count == CONFIG_BT_MAX_PAIRED) {
		for (size_t i = 0; i < bond_find_data.peer_count; i++) {
			err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_ADDR,
						 &bond_find_data.peer_address[i]);
			if (err) {
				LOG_ERR("Address filter cannot be added (err %d)", err);
				goto error;
			}

			char addr_str[BT_ADDR_LE_STR_LEN];
			bt_addr_le_to_str(&bond_find_data.peer_address[i],
					  addr_str, sizeof(addr_str));
			LOG_INF("Address filter added %s", log_strdup(addr_str));
		}
		filter_mode |= BT_SCAN_ADDR_FILTER;
	} else {
		LOG_INF("Device type filters added");
		/* Bluetooth scan filters are defined in separate header. */
		for (size_t i = 0; i < ARRAY_SIZE(bt_peripherals); i++) {
			const struct bt_scan_short_name filter = {
				.name = bt_peripherals[i],
				.min_len = strlen(bt_peripherals[i]),
			};

			err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_SHORT_NAME,
						 &filter);
			if (err) {
				LOG_ERR("Name filter cannot be added (err %d)",
					err);
				goto error;
			}
		}
		filter_mode |= BT_SCAN_SHORT_NAME_FILTER;

		err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, BT_UUID_HIDS);
		if (err) {
			LOG_ERR("UUID filter cannot be added (err %d)", err);
			goto error;
		}
		filter_mode |= BT_SCAN_UUID_FILTER;
	}

	err = bt_scan_filter_enable(filter_mode, true);
	if (err) {
		LOG_ERR("Filters cannot be turned on (err %d)", err);
		goto error;
	}

error:
	return err;
}

static void scan_start(void)
{
	int err;

	struct bond_find_data bond_find_data = {
		.conn_count = 0,
		.peer_count = 0,
	};

	bt_foreach_bond(BT_ID_DEFAULT, bond_find,
			&bond_find_data);
	if (bond_find_data.conn_count == CONFIG_BT_MAX_CONN) {
		LOG_INF("Max number of peers connected - scanning disabled");
		return;
	} else if (discovering_peer_conn) {
		LOG_INF("Discovery in progress - scanning disabled");
		return;
	}

	err = configure_filters();
	if (err) {
		LOG_ERR("Cannot set filters (err %d)", err);
		goto error;
	}

	err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
	if (err == -EALREADY) {
		LOG_WRN("Scanning already enabled");
	} else if (err) {
		LOG_ERR("Cannot start scanning (err %d)", err);
		goto error;
	} else {
		LOG_INF("Scan started");
	}

	k_delayed_work_submit(&scan_stop_trigger, SCAN_DURATION_MS);
	k_delayed_work_cancel(&scan_start_trigger);

	return;

error:
	module_set_state(MODULE_STATE_ERROR);
}

static void scan_start_trigger_fn(struct k_work *w)
{
	static_assert((SCAN_TRIG_TIMEOUT_MS > SCAN_TRIG_CHECK_MS) &&
		      (SCAN_TRIG_CHECK_MS > 0), "");

	scan_counter += SCAN_TRIG_CHECK_MS;
	if (scan_counter > SCAN_TRIG_TIMEOUT_MS) {
		scan_counter = 0;
		scan_start();
	} else {
		k_delayed_work_submit(&scan_start_trigger, SCAN_TRIG_CHECK_MS);
	}
}

static void scan_stop_trigger_fn(struct k_work *w)
{
	static_assert(SCAN_DURATION_MS > 0, "");

	struct bond_find_data bond_find_data = {
		.conn_count = 0,
		.peer_count = 0,
	};

	bt_foreach_bond(BT_ID_DEFAULT, bond_find,
			&bond_find_data);
	if (bond_find_data.conn_count != 0) {
		scan_stop();
	}
}

static void scan_filter_match(struct bt_scan_device_info *device_info,
			      struct bt_scan_filter_match *filter_match,
			      bool connectable)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(device_info->addr, addr, sizeof(addr));

	LOG_INF("Filters matched. %s %sconnectable",
		log_strdup(addr), connectable ? "" : "non");

	scan_stop();
}

static void scan_connecting_error(struct bt_scan_device_info *device_info)
{
	LOG_WRN("Connecting failed");
	/* Restarting scan. */
	scan_start();
}

static void scan_connecting(struct bt_scan_device_info *device_info,
			    struct bt_conn *conn)
{
	LOG_INF("Connecting done");
	__ASSERT_NO_MSG(!discovering_peer_conn);
	discovering_peer_conn = conn;
	bt_conn_ref(discovering_peer_conn);
}

static void discovery_completed(struct bt_gatt_dm *dm, void *context)
{
	LOG_INF("The discovery procedure succeeded");

	bt_gatt_dm_data_print(dm);

	struct ble_discovery_complete_event *event =
		new_ble_discovery_complete_event();

	event->dm = dm;

	EVENT_SUBMIT(event);

	/* This module is the last one to process this event and release
	 * the discovery data.
	 */
}

static void discovery_service_not_found(struct bt_conn *conn, void *context)
{
	LOG_ERR("Cannot find service during the discovery");
	/* discovering_peer_conn updated on PEER_DISCONNECTED event */
	__ASSERT_NO_MSG(discovering_peer_conn == conn);
	bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
}

static void discovery_error_found(struct bt_conn *conn, int err, void *context)
{
	LOG_WRN("The discovery failed (err %d)", err);
	/* discovering_peer_conn updated on PEER_DISCONNECTED event */
	__ASSERT_NO_MSG(discovering_peer_conn == conn);
	bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
}

static void start_discovery(struct bt_conn *conn)
{
	static const struct bt_gatt_dm_cb discovery_cb = {
		.completed = discovery_completed,
		.service_not_found = discovery_service_not_found,
		.error_found = discovery_error_found,
	};

	int err = bt_gatt_dm_start(conn,
				   BT_UUID_HIDS,
				   &discovery_cb,
				   NULL);

	if (err == -EALREADY) {
		LOG_ERR("Discovery already in progress");
	} else if (err) {
		LOG_ERR("Cannot start the discovery (err %d)", err);
	}
}

extern bool bt_le_conn_params_valid(const struct bt_le_conn_param *param);

static bool le_param_req(struct bt_conn *conn, struct bt_le_conn_param *param)
{
	LOG_INF("Connection parameter update request");

	param->interval_min = 1;
	param->interval_max = 1;

	if (bt_le_conn_params_valid(param)) {
		LOG_INF("Low latency operation");
	} else {
		LOG_INF("Normal operation");
		param->interval_min = 6;
		param->interval_max = 6;
	}

	return true;
}

static void scan_init(void)
{
	static const struct bt_le_scan_param sp = {
		.type = BT_HCI_LE_SCAN_ACTIVE,
		.filter_dup = BT_HCI_LE_SCAN_FILTER_DUP_ENABLE,
		.interval = BT_GAP_SCAN_FAST_INTERVAL,
		.window = BT_GAP_SCAN_FAST_WINDOW,
	};

	static const struct bt_scan_init_param scan_init = {
		.connect_if_match = true,
		.scan_param = &sp,
		.conn_param = NULL,
	};

	bt_scan_init(&scan_init);

	static struct bt_scan_cb scan_cb = {
		.filter_match = scan_filter_match,
		.connecting_error = scan_connecting_error,
		.connecting = scan_connecting
	};

	bt_scan_cb_register(&scan_cb);

	static struct bt_conn_cb conn_callbacks = {
		.le_param_req = le_param_req,
	};

	bt_conn_cb_register(&conn_callbacks);
	k_delayed_work_init(&scan_start_trigger, scan_start_trigger_fn);
	k_delayed_work_init(&scan_stop_trigger, scan_stop_trigger_fn);
}

static bool event_handler(const struct event_header *eh)
{
	if ((IS_ENABLED(CONFIG_DESKTOP_HID_MOUSE) && is_hid_mouse_event(eh)) ||
	    (IS_ENABLED(CONFIG_DESKTOP_HID_KEYBOARD) && is_hid_keyboard_event(eh)) ||
	    (IS_ENABLED(CONFIG_DESKTOP_HID_CONSUMER_CTRL) && is_hid_consumer_ctrl_event(eh))) {
		/* Do not enable scan when devices are in use. */
		scan_counter = 0;

		return false;
	}

	if (is_module_state_event(eh)) {
		const struct module_state_event *event =
			cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(ble_state),
				MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			scan_init();

			module_set_state(MODULE_STATE_READY);
		} else if (check_state(event, MODULE_ID(ble_bond), MODULE_STATE_READY)) {
			static bool started;

			__ASSERT_NO_MSG(!started);

			/* Settings need to be loaded before scan start */
			scan_start();
			started = true;
		}

		return false;
	}

	if (is_ble_peer_event(eh)) {
		const struct ble_peer_event *event =
			cast_ble_peer_event(eh);

		switch (event->state) {
		case PEER_STATE_CONNECTED:
		case PEER_STATE_CONN_FAILED:
			/* Ignore */
			break;
		case PEER_STATE_DISCONNECTED:
			if (discovering_peer_conn == event->id) {
				bt_conn_unref(discovering_peer_conn);
				discovering_peer_conn = NULL;
			}
			scan_stop();
			scan_start();
			break;
		case PEER_STATE_SECURED:
			start_discovery(event->id);
			break;
		default:
			__ASSERT_NO_MSG(false);
			break;
		}
		return false;
	}

	if (is_ble_peer_operation_event(eh)) {
		const struct ble_peer_operation_event *event =
			cast_ble_peer_operation_event(eh);

		switch (event->op) {

		case PEER_OPERATION_ERASED:
			scan_stop();
			scan_start();
			break;

		case PEER_OPERATION_ERASE_ADV:
		case PEER_OPERATION_ERASE_ADV_CANCEL:
		case PEER_OPERATION_SELECT:
		case PEER_OPERATION_SELECTED:
		case PEER_OPERATION_ERASE:
		case PEER_OPERATION_CANCEL:
			/* Ignore */
			break;
		default:
			__ASSERT_NO_MSG(false);
			break;
		}

		return false;
	}

	if (is_ble_discovery_complete_event(eh)) {
		const struct ble_discovery_complete_event *event =
			cast_ble_discovery_complete_event(eh);

		int err = bt_gatt_dm_data_release(event->dm);
		if (err) {
			LOG_ERR("Discovery data release failed (err %d)", err);
		}
		bt_conn_unref(discovering_peer_conn);
		discovering_peer_conn = NULL;
		/* Cannot start scanning right after discovery - problems
		 * establishing security - using delayed work as workaround.
		 */
		k_delayed_work_submit(&scan_start_trigger,
				      SCAN_TRIG_TIMEOUT_MS);
		scan_counter = SCAN_TRIG_TIMEOUT_MS;

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
EVENT_SUBSCRIBE(MODULE, hid_mouse_event);
EVENT_SUBSCRIBE(MODULE, hid_keyboard_event);
EVENT_SUBSCRIBE(MODULE, hid_consumer_ctrl_event);
EVENT_SUBSCRIBE_FINAL(MODULE, ble_discovery_complete_event);
