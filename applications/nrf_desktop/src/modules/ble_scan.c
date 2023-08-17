/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <bluetooth/scan.h>
#include <zephyr/settings/settings.h>

#include <string.h>

#define MODULE ble_scan
#include <caf/events/module_state_event.h>
#include "hid_event.h"
#include <caf/events/ble_common_event.h>
#include <caf/events/power_event.h>

#include "ble_scan_def.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_BLE_SCAN_LOG_LEVEL);

#define SCAN_TRIG_CHECK_MS	(1 * MSEC_PER_SEC)
#define SCAN_TRIG_TIMEOUT_MS	(CONFIG_DESKTOP_BLE_SCAN_START_TIMEOUT_S * MSEC_PER_SEC)
#define SCAN_DURATION_MS	(CONFIG_DESKTOP_BLE_SCAN_DURATION_S * MSEC_PER_SEC)
#define SCAN_START_DELAY_MS	15

#define SUBSCRIBED_PEERS_STORAGE_NAME "subscribers"

struct subscriber_data {
	uint8_t conn_count;
	uint8_t peer_count;
};

struct subscribed_peer {
	bt_addr_le_t addr;
	enum peer_type peer_type;
	bool llpm_support;
};

static struct subscribed_peer subscribed_peers[CONFIG_BT_MAX_PAIRED];

static struct bt_conn *discovering_peer_conn;
static unsigned int scan_counter;
static struct k_work_delayable scan_start_trigger;
static struct k_work_delayable scan_stop_trigger;
static bool peers_only = !IS_ENABLED(CONFIG_DESKTOP_BLE_NEW_PEER_SCAN_ON_BOOT);
static bool scanning;
static bool scan_blocked;
static bool ble_bond_ready;


static void verify_bond(const struct bt_bond_info *info, void *user_data)
{
	for (size_t i = 0; i < ARRAY_SIZE(subscribed_peers); i++) {
		if (!bt_addr_le_cmp(&subscribed_peers[i].addr, &info->addr)) {
			return;
		}
	}

	LOG_WRN("Peer data inconsistency. Removing unknown peer.");
	int err = bt_unpair(BT_ID_DEFAULT, &info->addr);

	if (err) {
		LOG_ERR("Cannot unpair peer (err %d)", err);
		module_set_state(MODULE_STATE_ERROR);
	}
}

static int settings_set(const char *key, size_t len_rd,
			settings_read_cb read_cb, void *cb_arg)
{
	if (!strcmp(key, SUBSCRIBED_PEERS_STORAGE_NAME)) {
		ssize_t len = read_cb(cb_arg, &subscribed_peers, sizeof(subscribed_peers));

		if ((len != sizeof(subscribed_peers)) || (len != len_rd)) {
			LOG_ERR("Can't read subscribed_peers from storage");
			module_set_state(MODULE_STATE_ERROR);
			return len;
		}
	}

	return 0;
}

static int verify_subscribed_peers(void)
{
	/* On commit we should verify data to prevent inconsistency.
	 * Inconsistency could be caused e.g. by reset after secure,
	 * but before storing peer type in ble_scan module.
	 */
	bt_foreach_bond(BT_ID_DEFAULT, verify_bond, NULL);

	return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(ble_scan, MODULE_NAME, NULL, settings_set,
			       verify_subscribed_peers, NULL);

static void conn_cnt_foreach(struct bt_conn *conn, void *data)
{
	size_t *cur_cnt = data;

	(*cur_cnt)++;
}

static size_t count_conn(void)
{
	size_t conn_count = 0;

	bt_conn_foreach(BT_CONN_TYPE_LE, conn_cnt_foreach, &conn_count);
	__ASSERT_NO_MSG(conn_count <= CONFIG_BT_MAX_CONN);

	return conn_count;
}

static size_t count_bond(void)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(subscribed_peers); i++) {
		if (!bt_addr_le_cmp(&subscribed_peers[i].addr, BT_ADDR_LE_NONE)) {
			break;
		}
	}

	return i;
}

static void broadcast_scan_state(bool active)
{
	struct ble_peer_search_event *event = new_ble_peer_search_event();
	event->active = active;
	APP_EVENT_SUBMIT(event);
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

	scanning = false;
	broadcast_scan_state(scanning);

	/* Cancel cannot fail if executed from another work's context. */
	(void)k_work_cancel_delayable(&scan_stop_trigger);

	if (count_conn() < CONFIG_BT_MAX_CONN) {
		scan_counter = 0;
		(void)k_work_reschedule(&scan_start_trigger, K_MSEC(SCAN_TRIG_CHECK_MS));
	}
}

static int configure_address_filters(uint8_t *filter_mode)
{
	int err = 0;

	for (size_t i = 0; i < ARRAY_SIZE(subscribed_peers); i++) {
		const bt_addr_le_t *addr = &subscribed_peers[i].addr;

		if (!bt_addr_le_cmp(addr, BT_ADDR_LE_NONE)) {
			break;
		}

		struct bt_conn *conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, addr);

		if (conn) {
			bt_conn_unref(conn);
			continue;
		}

		err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_ADDR, addr);
		if (err) {
			LOG_ERR("Address filter cannot be added (err %d)", err);
			break;
		}

		char addr_str[BT_ADDR_LE_STR_LEN];

		bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
		LOG_INF("Address filter added %s", addr_str);
		*filter_mode |= BT_SCAN_ADDR_FILTER;
	}

	return err;
}

static int configure_name_filters(uint8_t *filter_mode)
{
	uint8_t peer_cnt[PEER_TYPE_COUNT] = {0};
	static const uint8_t peer_limit[PEER_TYPE_COUNT] = {
		[PEER_TYPE_MOUSE] = CONFIG_DESKTOP_BLE_SCAN_MOUSE_LIMIT,
		[PEER_TYPE_KEYBOARD] = CONFIG_DESKTOP_BLE_SCAN_KEYBOARD_LIMIT,
	};
	int err = 0;

	for (size_t i = 0; i < ARRAY_SIZE(subscribed_peers); i++) {
		enum peer_type type = subscribed_peers[i].peer_type;

		if (type == PEER_TYPE_COUNT) {
			continue;
		}
		__ASSERT_NO_MSG(peer_cnt[type] < peer_limit[type]);
		peer_cnt[type]++;
	}

	/* Bluetooth scan filters are defined in separate header. */
	for (size_t i = 0; i < ARRAY_SIZE(peer_name); i++) {
		if ((peer_cnt[i] == peer_limit[i]) || (peer_name[i] == NULL)) {
			continue;
		}

		err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_NAME, peer_name[i]);
		if (err) {
			LOG_ERR("Name filter cannot be added (err %d)", err);
			break;
		}
		*filter_mode |= BT_SCAN_NAME_FILTER;
	}

	if (!err) {
		LOG_INF("Device name filters added");
	}

	return err;
}

static int configure_filters(void)
{
	BUILD_ASSERT(CONFIG_BT_MAX_PAIRED >= CONFIG_BT_MAX_CONN, "");
	BUILD_ASSERT(CONFIG_BT_MAX_PAIRED <= CONFIG_BT_SCAN_ADDRESS_CNT,
		     "Insufficient number of address filters");
	BUILD_ASSERT(ARRAY_SIZE(peer_name) <= CONFIG_BT_SCAN_NAME_CNT,
		     "Insufficient number of name filers");
	BUILD_ASSERT(ARRAY_SIZE(peer_name) == PEER_TYPE_COUNT, "");
	bt_scan_filter_remove_all();

	uint8_t filter_mode = 0;
	int err = configure_address_filters(&filter_mode);

	bool use_name_filters = true;

	if (IS_ENABLED(CONFIG_DESKTOP_BLE_NEW_PEER_SCAN_REQUEST) && peers_only) {
		use_name_filters = false;
	}

	if (!err && use_name_filters && (count_bond() < CONFIG_BT_MAX_PAIRED)) {
		err = configure_name_filters(&filter_mode);
	}

	if (!err) {
		err = bt_scan_filter_enable(filter_mode, false);
	} else {
		LOG_ERR("Filters cannot be turned on (err %d)", err);
	}

	return err;
}

static bool is_llpm_peer_connected(void)
{
	bool llpm_peer_connected = false;

	__ASSERT_NO_MSG(IS_ENABLED(CONFIG_CAF_BLE_USE_LLPM));

	for (size_t i = 0; i < ARRAY_SIZE(subscribed_peers); i++) {
		const bt_addr_le_t *addr = &subscribed_peers[i].addr;

		if (!bt_addr_le_cmp(addr, BT_ADDR_LE_NONE)) {
			break;
		}

		struct bt_conn *conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, addr);

		if (conn) {
			bt_conn_unref(conn);
			if (subscribed_peers[i].llpm_support) {
				llpm_peer_connected = true;
				break;
			}
		}
	}

	return llpm_peer_connected;
}

static void update_init_conn_params(bool llpm_peer_connected)
{
	struct bt_le_conn_param cp = {
		.latency = 0,
		.timeout = 400,
	};

	/* In case LLPM peer is already connected, the next peer has to be
	 * connected with 10 ms connection interval instead of 7.5 ms.
	 * Connecting with 7.5 ms may cause Bluetooth scheduling issues.
	 */
	if (llpm_peer_connected) {
		cp.interval_min = 8;
		cp.interval_max = 8;
	} else {
		cp.interval_min = 6;
		cp.interval_max = 6;
	}

	bt_scan_update_init_conn_params(&cp);
}

static void scan_start(void)
{
	size_t conn_count = count_conn();
	size_t bond_count = count_bond();
	int err;

	if (scanning) {
		scan_stop();
	}

	if (IS_ENABLED(CONFIG_DESKTOP_BLE_SCAN_PM_EVENTS) && scan_blocked) {
		LOG_INF("Power down mode - scanning blocked");
		return;
	} else if (IS_ENABLED(CONFIG_DESKTOP_BLE_NEW_PEER_SCAN_REQUEST) &&
		   (conn_count == bond_count) && peers_only) {
		LOG_INF("All known peers connected - scanning disabled");
		return;
	} else if (conn_count == CONFIG_BT_MAX_CONN) {
		LOG_INF("Max number of peers connected - scanning disabled");
		return;
	} else if (discovering_peer_conn) {
		LOG_INF("Discovery in progress");
		return;
	}

	if (IS_ENABLED(CONFIG_CAF_BLE_USE_LLPM) && (CONFIG_BT_MAX_CONN == 2)) {
		/* If the central supports the LLPM and more than two
		 * simultaneous Bluetooth connections, the BLE peers use the
		 * connection interval of 10 ms instead of 7.5 ms and there is
		 * no need to update the initial connection parameters.
		 */
		update_init_conn_params(is_llpm_peer_connected());
	}

	err = configure_filters();
	if (err) {
		LOG_ERR("Cannot set filters (err %d)", err);
		goto error;
	}

	err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
	if (err) {
		LOG_ERR("Cannot start scanning (err %d)", err);
		goto error;
	} else {
		LOG_INF("Scan started");
	}

	scanning = true;
	broadcast_scan_state(scanning);

	(void)k_work_reschedule(&scan_stop_trigger, K_MSEC(SCAN_DURATION_MS));

	/* Cancel cannot fail if executed from another work's context. */
	(void)k_work_cancel_delayable(&scan_start_trigger);

	return;

error:
	module_set_state(MODULE_STATE_ERROR);
}

static void scan_start_trigger_fn(struct k_work *w)
{
	BUILD_ASSERT(SCAN_START_DELAY_MS > 0, "");
	BUILD_ASSERT(SCAN_TRIG_CHECK_MS > 0, "");
	BUILD_ASSERT(SCAN_TRIG_TIMEOUT_MS > SCAN_TRIG_CHECK_MS, "");

	scan_counter += SCAN_TRIG_CHECK_MS;
	if (scan_counter > SCAN_TRIG_TIMEOUT_MS) {
		scan_counter = 0;
		scan_start();
	} else {
		(void)k_work_reschedule(&scan_start_trigger, K_MSEC(SCAN_TRIG_CHECK_MS));
	}
}

static void scan_stop_trigger_fn(struct k_work *w)
{
	BUILD_ASSERT(SCAN_DURATION_MS > 0, "");

	if (count_conn() != 0) {
		peers_only = true;
		scan_stop();
	}
}

static void scan_filter_match(struct bt_scan_device_info *device_info,
			      struct bt_scan_filter_match *filter_match,
			      bool connectable)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));
	LOG_INF("Filters matched. %s %sconnectable", addr, connectable ? "" : "non");

	/* Scanning will be stopped by nrf scan module. */
}

static void scan_connecting_error(struct bt_scan_device_info *device_info)
{
	LOG_WRN("Connecting failed");
	scan_counter = SCAN_TRIG_TIMEOUT_MS;

	(void)k_work_reschedule(&scan_start_trigger, K_MSEC(SCAN_START_DELAY_MS));
}

static void scan_connecting(struct bt_scan_device_info *device_info,
			    struct bt_conn *conn)
{
	LOG_INF("Connecting done");
	__ASSERT_NO_MSG(!discovering_peer_conn);
	discovering_peer_conn = conn;
	bt_conn_ref(discovering_peer_conn);
}

static int store_subscribed_peers(void)
{
	if (IS_ENABLED(CONFIG_SETTINGS)) {
		char key[] = MODULE_NAME "/" SUBSCRIBED_PEERS_STORAGE_NAME;
		int err = settings_save_one(key, subscribed_peers, sizeof(subscribed_peers));

		if (err) {
			LOG_ERR("Problem storing subscribed_peers (err %d)", err);
			return err;
		}
	}

	return 0;
}

static void reset_subscribers(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(subscribed_peers); i++) {
		bt_addr_le_copy(&subscribed_peers[i].addr, BT_ADDR_LE_NONE);
		subscribed_peers[i].peer_type = PEER_TYPE_COUNT;
		subscribed_peers[i].llpm_support = false;
	}
}

BT_SCAN_CB_INIT(scan_cb, scan_filter_match, NULL, scan_connecting_error, scan_connecting);

static void scan_init(void)
{
	reset_subscribers();

	static const struct bt_le_scan_param sp = {
		.type = BT_LE_SCAN_TYPE_ACTIVE,
		.options = BT_LE_SCAN_OPT_FILTER_DUPLICATE,
		.interval = BT_GAP_SCAN_FAST_INTERVAL,
		.window = BT_GAP_SCAN_FAST_WINDOW,
	};

	struct bt_le_conn_param cp = {
		.latency = 0,
		.timeout = 400,
	};

	if (IS_ENABLED(CONFIG_CAF_BLE_USE_LLPM) && (CONFIG_BT_MAX_CONN >= 2)) {
		cp.interval_min = 8;
		cp.interval_max = 8;
	} else {
		cp.interval_min = 6;
		cp.interval_max = 6;
	}

	struct bt_scan_init_param scan_init = {
		.connect_if_match = true,
		.scan_param = &sp,
		.conn_param = &cp,
	};

	bt_scan_init(&scan_init);
	bt_scan_cb_register(&scan_cb);

	k_work_init_delayable(&scan_start_trigger, scan_start_trigger_fn);
	k_work_init_delayable(&scan_stop_trigger, scan_stop_trigger_fn);
}

static bool handle_hid_report_event(const struct hid_report_event *event)
{
	/* Ignore HID output reports. Subscriber is NULL for a HID output report. */
	if (!event->subscriber) {
		return false;
	}

	/* Do not scan when devices are in use. */
	scan_counter = 0;

	if (scanning) {
		scan_stop();
	}

	return false;
}

static bool handle_module_state_event(const struct module_state_event *event)
{
	if (check_state(event, MODULE_ID(ble_state), MODULE_STATE_READY)) {
		static bool initialized;

		__ASSERT_NO_MSG(!initialized);
		initialized = true;

		scan_init();

		module_set_state(MODULE_STATE_READY);
	} else if (check_state(event, MODULE_ID(ble_bond), MODULE_STATE_READY)) {
		/* Settings need to be loaded on start before the scan begins.
		 * As the ble_bond module reports its READY state also on wake_up_event,
		 * to avoid multiple scan start triggering on wake-up scan will be started
		 * from here only on start-up.
		 */
		if (!ble_bond_ready) {
			ble_bond_ready = true;
			scan_start();
		}
	}

	return false;
}

static bool handle_ble_peer_event(const struct ble_peer_event *event)
{
	switch (event->state) {
	case PEER_STATE_CONNECTED:
	case PEER_STATE_SECURED:
	case PEER_STATE_DISCONNECTING:
		/* Ignore */
		break;

	case PEER_STATE_CONN_FAILED:
	case PEER_STATE_DISCONNECTED:
		if (discovering_peer_conn == event->id) {
			bt_conn_unref(discovering_peer_conn);
			discovering_peer_conn = NULL;
		}
		/* ble_state keeps reference to connection object.
		 * Cannot create new connection now.
		 */
		scan_counter = SCAN_TRIG_TIMEOUT_MS;
		(void)k_work_reschedule(&scan_start_trigger, K_MSEC(SCAN_START_DELAY_MS));
		break;

	default:
		__ASSERT_NO_MSG(false);
		break;
	}

	return false;
}

static bool handle_ble_peer_operation_event(const struct ble_peer_operation_event *event)
{
	switch (event->op) {
	case PEER_OPERATION_ERASED:
		reset_subscribers();
		store_subscribed_peers();
		/* Fall-through */

	case PEER_OPERATION_SCAN_REQUEST:
		if (IS_ENABLED(CONFIG_BT_SCAN_CONN_ATTEMPTS_FILTER)) {
			bt_scan_conn_attempts_filter_clear();
		}
		peers_only = false;
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

static bool handle_ble_discovery_complete_event(const struct ble_discovery_complete_event *event)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(subscribed_peers); i++) {
		/* Already saved. */
		if (!bt_addr_le_cmp(&subscribed_peers[i].addr,
			bt_conn_get_dst(discovering_peer_conn))) {
			break;
		}

		/* Save data about new subscriber. */
		if (!bt_addr_le_cmp(&subscribed_peers[i].addr, BT_ADDR_LE_NONE)) {
			bt_addr_le_copy(&subscribed_peers[i].addr,
				bt_conn_get_dst(discovering_peer_conn));
			subscribed_peers[i].peer_type =
				event->peer_type;
			subscribed_peers[i].llpm_support = event->peer_llpm_support;
			store_subscribed_peers();
			break;
		}
	}

	__ASSERT_NO_MSG(i < ARRAY_SIZE(subscribed_peers));

	bt_conn_unref(discovering_peer_conn);
	discovering_peer_conn = NULL;

	/* Cannot start scanning right after discovery - problems
	 * establishing security - using delayed work as workaround.
	 */
	scan_counter = SCAN_TRIG_TIMEOUT_MS;
	(void)k_work_reschedule(&scan_start_trigger, K_MSEC(SCAN_TRIG_TIMEOUT_MS));

	if (IS_ENABLED(CONFIG_BT_SCAN_CONN_ATTEMPTS_FILTER)) {
		bt_scan_conn_attempts_filter_clear();
	}

	return false;
}

static bool handle_power_down_event(const struct power_down_event *event)
{
	if (scanning) {
		scan_stop();
	}

	scan_blocked = true;
	(void)k_work_cancel_delayable(&scan_start_trigger);

	return false;
}

static bool handle_wake_up_event(const struct wake_up_event *event)
{
	if (scan_blocked) {
		scan_blocked = false;
		if (ble_bond_ready) {
			scan_start();
		}
	}

	return false;
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_hid_report_event(aeh)) {
		return handle_hid_report_event(cast_hid_report_event(aeh));
	}

	if (is_module_state_event(aeh)) {
		return handle_module_state_event(cast_module_state_event(aeh));
	}

	if (is_ble_peer_event(aeh)) {
		return handle_ble_peer_event(cast_ble_peer_event(aeh));
	}

	if (is_ble_peer_operation_event(aeh)) {
		return handle_ble_peer_operation_event(cast_ble_peer_operation_event(aeh));
	}

	if (is_ble_discovery_complete_event(aeh)) {
		return handle_ble_discovery_complete_event(cast_ble_discovery_complete_event(aeh));
	}

	if (IS_ENABLED(CONFIG_DESKTOP_BLE_SCAN_PM_EVENTS) &&
	    is_power_down_event(aeh)) {
		return handle_power_down_event(cast_power_down_event(aeh));
	}

	if (IS_ENABLED(CONFIG_DESKTOP_BLE_SCAN_PM_EVENTS) &&
	    is_wake_up_event(aeh)) {
		return handle_wake_up_event(cast_wake_up_event(aeh));
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
APP_EVENT_SUBSCRIBE(MODULE, ble_peer_event);
APP_EVENT_SUBSCRIBE(MODULE, ble_peer_operation_event);
APP_EVENT_SUBSCRIBE(MODULE, ble_discovery_complete_event);
APP_EVENT_SUBSCRIBE(MODULE, hid_report_event);
#ifdef CONFIG_DESKTOP_BLE_SCAN_PM_EVENTS
APP_EVENT_SUBSCRIBE(MODULE, power_down_event);
APP_EVENT_SUBSCRIBE(MODULE, wake_up_event);
#endif
