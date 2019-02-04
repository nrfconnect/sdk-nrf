/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/gatt_dm.h>
#include <bluetooth/scan.h>

#include <settings/settings.h>

#define MODULE ble_scan
#include "module_state_event.h"

#include "ble_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_BLE_SCANNING_LOG_LEVEL);


#define DEVICE_NAME	CONFIG_DESKTOP_BLE_SHORT_NAME
#define DEVICE_NAME_LEN	(sizeof(DEVICE_NAME) - 1)

static bt_addr_le_t target_addr;


static void save_address(struct bt_conn *conn)
{
	struct bt_conn_info info;

	int err = bt_conn_get_info(conn, &info);

	if (err) {
		LOG_ERR("Cannot get conn info");
		return;
	}

	if (bt_addr_le_cmp(&target_addr, info.le.dst)) {
		LOG_INF("Store target address");
		bt_addr_le_copy(&target_addr, info.le.dst);
		settings_save_one(MODULE_NAME "/taddr", &target_addr,
				  sizeof(target_addr));
	}
}

static int settings_set(int argc, char **argv, void *val_ctx)
{
	if (argc == 1) {
		if (!strcmp(argv[0], "taddr")) {
			int len = settings_val_read_cb(val_ctx, &target_addr,
						       sizeof(target_addr));

			if (len < 0) {
				LOG_ERR("Can't read target address (err:%d)",
					len);
			}
		}

		return 0;
	}

	return -ENOENT;
}

static int enable_settings(void)
{
	int err = 0;

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		static struct settings_handler sh = {
			.name = MODULE_NAME,
			.h_set = settings_set,
		};

		err = settings_register(&sh);
		if (err) {
			LOG_ERR("Cannot register settings handler (err %d)", err);
			goto error;
		}
	}

error:
	return err;
}

static int configure_filters(void)
{
	bt_scan_filter_remove_all();

	static_assert(DEVICE_NAME_LEN > 0, "Invalid device name");

	static const struct bt_scan_short_name short_name = {
		.name = DEVICE_NAME,
		.min_len = DEVICE_NAME_LEN,
	};
	int err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_SHORT_NAME, &short_name);
	if (err) {
		LOG_ERR("Name filter cannot be added (err %d)", err);
		goto error;
	}
	u8_t filter_mode = BT_SCAN_SHORT_NAME_FILTER;

	err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, BT_UUID_HIDS);
	if (err) {
		LOG_ERR("UUID filter cannot be added (err %d)", err);
		goto error;
	}
	filter_mode |= BT_SCAN_UUID_FILTER;

	if (target_addr.a.val[0] != 0) {
		err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_ADDR, &target_addr);
		if (err) {
			LOG_ERR("Address filter cannot be added (err %d)", err);
			goto error;
		}
		filter_mode |= BT_SCAN_ADDR_FILTER;

		LOG_INF("Address filter added");
	}

	err = bt_scan_filter_enable(filter_mode, true);
	if (err) {
		LOG_ERR("Filters cannot be turned on (err %d)", err);
		goto error;
	}

error:
	return err;
}

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
	LOG_WRN("Cannot find service during the discovery");
}

static void discovery_error_found(struct bt_conn *conn, int err, void *context)
{
	LOG_WRN("The discovery failed (err %d)", err);
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
	if (err) {
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

static int scan_init(void)
{
	int err = enable_settings();
	if (err) {
		LOG_ERR("Cannot enable settings (err %d)", err);
		goto error;
	}

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

	static struct bt_conn_cb conn_callbacks = {
		.le_param_req = le_param_req,
	};

	bt_conn_cb_register(&conn_callbacks);

error:
	return err;
}

static void scan_start(void)
{
	int err = configure_filters();
	if (err) {
		LOG_ERR("Cannot set filters (err %d)", err);
		goto error;
	}

	err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
	if (err) {
		LOG_ERR("Cannot start scanning (err %d)", err);
		goto error;
	}

	LOG_INF("Scan started");
	return;

error:
	LOG_ERR("Scanning failed to start (err %d)", err);
	module_set_state(MODULE_STATE_ERROR);
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
		} else if (check_state(event, MODULE_ID(config), MODULE_STATE_READY)) {
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
			/* Ignore */
			break;
		case PEER_STATE_DISCONNECTED:
			scan_start();
			break;
		case PEER_STATE_SECURED:
			save_address(event->id);
			start_discovery(event->id);
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

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE(MODULE, ble_peer_event);
EVENT_SUBSCRIBE_FINAL(MODULE, ble_discovery_complete_event);
