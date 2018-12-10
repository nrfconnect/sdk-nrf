/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/common/gatt_dm.h>
#include <bluetooth/common/scan.h>

#define MODULE ble_scan
#include "module_state_event.h"
#include "button_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_BLE_SCANNING_LOG_LEVEL);


#define DEVICE_NAME	CONFIG_DESKTOP_BLE_SCANNING_DEVICE_NAME
#define DEVICE_NAME_LEN	(sizeof(DEVICE_NAME) - 1)


static void scan_filter_match(struct bt_scan_device_info *device_info,
			      struct bt_scan_filter_match *filter_match,
			      bool connectable)
{
	int err;
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(device_info->addr, addr, sizeof(addr));

	LOG_INF("Filters matched. %s %sconnectable",
		log_strdup(addr), connectable ? "" : "non");

	err = bt_scan_stop();
	if (err) {
		LOG_ERR("Stop LE scan failed (err %d)", err);
	}
}

static void scan_connecting_error(struct bt_scan_device_info *device_info)
{
	LOG_WRN("Connecting failed");
}

static void scan_connecting(struct bt_scan_device_info *device_info,
			    struct bt_conn *conn)
{
	LOG_INF("Connecting done");
}

static void discovery_completed(struct bt_gatt_dm *dm, void *context)
{
	LOG_INF("The discovery procedure succeeded");

	bt_gatt_dm_data_print(dm);

	int err = bt_gatt_dm_data_release(dm);
	if (err) {
		LOG_ERR("Could not release the discovery data (err %d)", err);
	}
}

static void discovery_service_not_found(struct bt_conn *conn, void *context)
{
	LOG_WRN("Cannot find service during the discovery");
}

static void discovery_error_found(struct bt_conn *conn, int err, void *context)
{
	LOG_WRN("The discovery failed (err %d)", err);
}


static void connected(struct bt_conn *conn, u8_t conn_err)
{
	if (conn_err) {
		char addr[BT_ADDR_LE_STR_LEN];

		bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

		LOG_ERR("Cannot connect %s (%u)", log_strdup(addr), conn_err);
		return;
	}

	static const struct bt_gatt_dm_cb discovery_cb = {
		.completed = discovery_completed,
		.service_not_found = discovery_service_not_found,
		.error_found = discovery_error_found,
	};

	int err = bt_gatt_dm_start(conn,
				   BT_UUID_HIDS,
				   &discovery_cb,
				   NULL);
	if (err) {
		LOG_ERR("Cannot start the discovery (err %d)", err);
	}
}

static void disconnected(struct bt_conn *conn, u8_t reason)
{
	LOG_INF("Disconnected");

	int err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
	if (err) {
		LOG_ERR("Cannot start scanning (err %d)", err);
	}
}

static int scan_init(void)
{
	static const struct bt_scan_init_param scan_init = {
		.connect_if_match = true,
		.scan_param = NULL,
	};

	bt_scan_init(&scan_init);

	static struct bt_scan_cb scan_cb = {
		.filter_match = scan_filter_match,
		.connecting_error = scan_connecting_error,
		.connecting = scan_connecting
	};

	bt_scan_cb_register(&scan_cb);

	int err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, BT_UUID_HIDS);
	if (err) {
		LOG_ERR("UUID filter cannot be added (err %d)", err);
		goto error;
	}

	static_assert(DEVICE_NAME_LEN > 0, "Invalid device name");
	err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_NAME, DEVICE_NAME);
	if (err) {
		LOG_ERR("Name filter cannot be added (err %d)", err);
		goto error;
	}

	err = bt_scan_filter_enable(BT_SCAN_UUID_FILTER | BT_SCAN_NAME_FILTER,
				    false);
	if (err) {
		LOG_ERR("Filters cannot be turned on (err %d)", err);
		goto error;
	}

	static struct bt_conn_cb conn_callbacks = {
		.connected = connected,
		.disconnected = disconnected,
	};

	bt_conn_cb_register(&conn_callbacks);

	err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
	if (err) {
		LOG_ERR("Scanning failed to start (err %d)", err);
	}

error:
	return err;
}


static bool event_handler(const struct event_header *eh)
{
	if (is_module_state_event(eh)) {
		const struct module_state_event *event =
			cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(ble_state),
				MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			int err = scan_init();
			if (err) {
				module_set_state(MODULE_STATE_ERROR);
			} else {
				module_set_state(MODULE_STATE_READY);
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
