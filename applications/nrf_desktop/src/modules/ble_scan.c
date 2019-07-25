/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/scan.h>
#include <string.h>

#define MODULE ble_scan
#include "module_state_event.h"
#include "hid_event.h"
#include "ble_event.h"

#include "ble_scan_def.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_BLE_SCANNING_LOG_LEVEL);

#define SCAN_TRIG_CHECK_MS    K_SECONDS(1)
#define SCAN_TRIG_TIMEOUT_MS  K_SECONDS(CONFIG_DESKTOP_BLE_SCAN_START_TIMEOUT_S)
#define SCAN_DURATION_MS      K_SECONDS(CONFIG_DESKTOP_BLE_SCAN_DURATION_S)

struct subscriber_data {
	u8_t conn_count;
	u8_t peer_count;
};

struct subscribed_peer {
	bt_addr_le_t addr;
	enum peer_type peer_type;
};

static struct subscribed_peer subscribed_peers[CONFIG_BT_MAX_CONN];

static struct bt_conn *discovering_peer_conn;
static unsigned int scan_counter;
static struct k_delayed_work scan_start_trigger;
static struct k_delayed_work scan_stop_trigger;


static void reset_subscribers(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(subscribed_peers); i++) {
		bt_addr_le_copy(&subscribed_peers[i].addr, BT_ADDR_LE_NONE);
		subscribed_peers[i].peer_type = PEER_TYPE_COUNT;
	}
}

static size_t count_conn(void)
{
	size_t conn_count = 0;

	for (size_t i = 0; i < ARRAY_SIZE(subscribed_peers); i++) {
		if (!bt_addr_le_cmp(&subscribed_peers[i].addr, BT_ADDR_LE_NONE)) {
			return conn_count;
		}
		struct bt_conn *conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT,
						&subscribed_peers[i].addr);

		if (conn) {
			conn_count++;
			bt_conn_unref(conn);
		}
	}

	return conn_count;
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

	if (count_conn() < CONFIG_BT_MAX_CONN) {
		scan_counter = 0;
		k_delayed_work_submit(&scan_start_trigger, SCAN_TRIG_CHECK_MS);
		k_delayed_work_cancel(&scan_stop_trigger);
	}
}

static int configure_filters(void)
{
	bt_scan_filter_remove_all();
	static_assert(CONFIG_BT_MAX_PAIRED == CONFIG_BT_MAX_CONN, "");
	static_assert(CONFIG_BT_MAX_PAIRED <= CONFIG_BT_SCAN_ADDRESS_CNT,
		      "Insufficient number of address filters");
	static_assert(ARRAY_SIZE(peer_type_short_name) <=
			CONFIG_BT_SCAN_SHORT_NAME_CNT,
		      "Insufficient number of short name filers");
	static_assert(ARRAY_SIZE(peer_type_short_name) == PEER_TYPE_COUNT, "");

	u8_t filter_mode = 0;
	int err;
	size_t i;

	for (i = 0; i < ARRAY_SIZE(subscribed_peers); i++) {
		if (!bt_addr_le_cmp(&subscribed_peers[i].addr, BT_ADDR_LE_NONE)) {
			break;
		}

		err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_ADDR,
					 &subscribed_peers[i].addr);
		if (err) {
			LOG_ERR("Address filter cannot be added (err %d)", err);
			return err;
		}

		char addr_str[BT_ADDR_LE_STR_LEN];

		bt_addr_le_to_str(&subscribed_peers[i].addr, addr_str,
				  sizeof(addr_str));
		LOG_INF("Address filter added %s", log_strdup(addr_str));
		filter_mode |= BT_SCAN_ADDR_FILTER;
	}

	if (i == CONFIG_BT_MAX_PAIRED) {
		goto filter_enable;
	}

	u8_t peers_mask = 0;

	for (size_t i = 0; i < ARRAY_SIZE(subscribed_peers); i++) {
		peers_mask |= BIT(subscribed_peers[i].peer_type);
	}

	/* Bluetooth scan filters are defined in separate header. */
	for (size_t i = 0; i < ARRAY_SIZE(peer_type_short_name); i++) {
		if (!(BIT(i) & (~peers_mask))) {
			continue;
		}

		const struct bt_scan_short_name filter = {
			.name = peer_type_short_name[i],
			.min_len = strlen(peer_type_short_name[i]),
		};

		err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_SHORT_NAME,
					 &filter);
		if (err) {
			LOG_ERR("Name filter cannot be added (err %d)", err);
			return err;
		}
		filter_mode |= BT_SCAN_SHORT_NAME_FILTER;
	}
	LOG_INF("Device type filters added");

filter_enable:
	err = bt_scan_filter_enable(filter_mode, false);
	if (err) {
		LOG_ERR("Filters cannot be turned on (err %d)", err);
		return err;
	}

	return 0;
}

static void scan_start(void)
{
	int err;

	if (count_conn() == CONFIG_BT_MAX_CONN) {
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

	if (count_conn() != 0) {
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
	reset_subscribers();
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
		case PEER_STATE_SECURED:
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
			reset_subscribers();
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
		size_t i;

		for (i = 0; i < ARRAY_SIZE(subscribed_peers); i++) {
			if (!bt_addr_le_cmp(&subscribed_peers[i].addr,
				  bt_conn_get_dst(discovering_peer_conn))) {
				break;
			}

			if (!bt_addr_le_cmp(&subscribed_peers[i].addr,
					    BT_ADDR_LE_NONE)) {
				bt_addr_le_copy(&subscribed_peers[i].addr,
					bt_conn_get_dst(discovering_peer_conn));
				subscribed_peers[i].peer_type =
					event->peer_type;
				break;
			}
		}
		__ASSERT_NO_MSG(i < ARRAY_SIZE(subscribed_peers));

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
EVENT_SUBSCRIBE(MODULE, ble_discovery_complete_event);
