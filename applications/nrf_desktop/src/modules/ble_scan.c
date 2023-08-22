/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <bluetooth/scan.h>
#include <bluetooth/gatt_dm.h>
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

#define SCAN_START_CHECK_MS	(1 * MSEC_PER_SEC)
#define SCAN_START_TIMEOUT_MS	(CONFIG_DESKTOP_BLE_SCAN_START_TIMEOUT_S * MSEC_PER_SEC)
#define SCAN_DURATION_MS	(CONFIG_DESKTOP_BLE_SCAN_DURATION_S * MSEC_PER_SEC)
#define FORCED_SCAN_DURATION_MS	(CONFIG_DESKTOP_BLE_FORCED_SCAN_DURATION_S * MSEC_PER_SEC)
#define SCAN_START_DELAY_MS	15

BUILD_ASSERT(SCAN_START_CHECK_MS > 0);
BUILD_ASSERT(SCAN_START_TIMEOUT_MS > 0);
BUILD_ASSERT(SCAN_DURATION_MS > 0, "");
BUILD_ASSERT(FORCED_SCAN_DURATION_MS >= 0, "");
BUILD_ASSERT(SCAN_START_DELAY_MS > 0, "");

#define SUBSCRIBED_PEERS_STORAGE_NAME "subscribers"

enum state {
	STATE_DISABLED,
	STATE_DISABLED_OFF,
	STATE_INITIALIZED,
	STATE_INITIALIZED_OFF,
	STATE_ACTIVE,
	STATE_FORCED_ACTIVE,
	STATE_DELAYED_ACTIVE,
	STATE_INACTIVE,
	STATE_OFF,
	STATE_ERROR,
};

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
static unsigned int scan_start_counter;
static struct k_work_delayable update_state_work;
static bool peers_only = !IS_ENABLED(CONFIG_DESKTOP_BLE_NEW_PEER_SCAN_ON_BOOT);
static bool scanning;

static enum state state;


static void store_subscribed_peers(void)
{
	if (IS_ENABLED(CONFIG_SETTINGS)) {
		char key[] = MODULE_NAME "/" SUBSCRIBED_PEERS_STORAGE_NAME;
		int err = settings_save_one(key, subscribed_peers, sizeof(subscribed_peers));

		if (err) {
			LOG_ERR("Problem storing subscribed_peers (err %d)", err);
		}
	}
}

static void reset_subscribers(bool settings_store)
{
	for (size_t i = 0; i < ARRAY_SIZE(subscribed_peers); i++) {
		struct subscribed_peer *sub = &subscribed_peers[i];

		bt_addr_le_copy(&sub->addr, BT_ADDR_LE_NONE);
		sub->peer_type = PEER_TYPE_COUNT;
		sub->llpm_support = false;
	}

	if (settings_store) {
		store_subscribed_peers();
	}
}

static void add_subscriber(const struct ble_discovery_complete_event *event)
{
	const bt_addr_le_t *peer_addr = bt_conn_get_dst(bt_gatt_dm_conn_get(event->dm));
	size_t i;

	for (i = 0; i < ARRAY_SIZE(subscribed_peers); i++) {
		struct subscribed_peer *sub = &subscribed_peers[i];

		/* Already saved. */
		if (!bt_addr_le_cmp(&sub->addr, peer_addr)) {
			break;
		}

		/* Save data about new subscriber. */
		if (!bt_addr_le_cmp(&sub->addr, BT_ADDR_LE_NONE)) {
			bt_addr_le_copy(&sub->addr, peer_addr);
			sub->peer_type = event->peer_type;
			sub->llpm_support = event->peer_llpm_support;

			store_subscribed_peers();
			break;
		}
	}

	/* Ensure peer was found. */
	__ASSERT_NO_MSG(i < ARRAY_SIZE(subscribed_peers));
}

static void peer_discovery_start(struct bt_conn *conn)
{
	__ASSERT_NO_MSG(!discovering_peer_conn);
	discovering_peer_conn = conn;
	bt_conn_ref(discovering_peer_conn);
}

static void peer_discovery_end(const struct ble_discovery_complete_event *event)
{
	if (event) {
		__ASSERT_NO_MSG(discovering_peer_conn == bt_gatt_dm_conn_get(event->dm));
		add_subscriber(event);
	}

	__ASSERT_NO_MSG(discovering_peer_conn);
	bt_conn_unref(discovering_peer_conn);
	discovering_peer_conn = NULL;
}

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

static int scan_stop(void)
{
	if (!scanning) {
		return 0;
	}

	int err = bt_scan_stop();

	if (!err) {
		LOG_INF("Scan stopped");
	} else if (err == -EALREADY) {
		LOG_WRN("Active scan already disabled");
		err = 0;
	} else {
		LOG_ERR("Stop LE scan failed (err: %d)", err);
	}

	if (!err) {
		scanning = false;
		broadcast_scan_state(scanning);
	}

	return err;
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

static int scan_start(void)
{
	int err = scan_stop();

	if (err) {
		LOG_ERR("Failed to stop scanning before restart (err %d)", err);
		return err;
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
		return err;
	}

	err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
	if (err) {
		LOG_ERR("Cannot start scanning (err %d)", err);
		return err;
	}

	LOG_INF("Scan started");
	scanning = true;
	broadcast_scan_state(scanning);

	return err;
}

static void update_state(enum state new_state);

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

	if (state != STATE_OFF) {
		update_state(STATE_DELAYED_ACTIVE);
	}
}

static void scan_connecting(struct bt_scan_device_info *device_info,
			    struct bt_conn *conn)
{
	LOG_INF("Connecting done");

	peer_discovery_start(conn);
}

BT_SCAN_CB_INIT(scan_cb, scan_filter_match, NULL, scan_connecting_error, scan_connecting);

static void update_state_work_fn(struct k_work *w);

static void scan_init(void)
{
	reset_subscribers(false);

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

	k_work_init_delayable(&update_state_work, update_state_work_fn);
}

static const char *state2str(enum state s)
{
	switch (s) {
	case STATE_DISABLED:
		return "DISABLED";

	case STATE_DISABLED_OFF:
		return "DISABLED_OFF";

	case STATE_INITIALIZED:
		return "INITIALIZED";

	case STATE_INITIALIZED_OFF:
		return "INITIALIZED_OFF";

	case STATE_ACTIVE:
		return "ACTIVE";

	case STATE_FORCED_ACTIVE:
		return "FORCED_ACTIVE";

	case STATE_DELAYED_ACTIVE:
		return "DELAYED_ACTIVE";

	case STATE_INACTIVE:
		return "INACTIVE";

	case STATE_OFF:
		return "OFF";

	case STATE_ERROR:
		return "ERROR";

	default:
		__ASSERT_NO_MSG(false);
		return "UNKNOWN";
	}
}

static enum state update_state_transition(enum state prev_state, enum state new_state, bool verbose)
{
	size_t conn_count = count_conn();
	size_t bond_count = count_bond();

	/* Skip forced active state if disabled in configuration. */
	if ((new_state == STATE_FORCED_ACTIVE) && (FORCED_SCAN_DURATION_MS == 0)) {
		new_state = STATE_ACTIVE;
	}

	if ((new_state == STATE_ACTIVE) || (new_state == STATE_FORCED_ACTIVE)) {
		if (IS_ENABLED(CONFIG_DESKTOP_BLE_NEW_PEER_SCAN_REQUEST) &&
		   (conn_count == bond_count) && peers_only) {
			if (verbose) {
				LOG_INF("All known peers connected - scanning disabled");
			}
			return STATE_INACTIVE;
		} else if (conn_count == CONFIG_BT_MAX_CONN) {
			if (verbose) {
				LOG_INF("Max number of peers connected - scanning disabled");
			}
			return STATE_INACTIVE;
		} else if (discovering_peer_conn) {
			if (verbose) {
				LOG_INF("Discovery in progress");
			}
			return STATE_INACTIVE;
		}
	} else if (new_state == STATE_INACTIVE) {
		__ASSERT_NO_MSG(prev_state != STATE_OFF);
		if (conn_count == 0) {
			if (verbose) {
				LOG_INF("No peer connected - keep scanning");
			}
			return STATE_ACTIVE;
		}
	}

	return new_state;
}

static bool is_module_init(enum state s)
{
	return (s != STATE_DISABLED) && (s != STATE_DISABLED_OFF);
}

/* The module only reports:
 * - ready state after initialization
 * - error state after a fatal error
 */
static void broadcast_module_state(enum state prev_state, enum state new_state)
{
	if (new_state == STATE_ERROR) {
		module_set_state(MODULE_STATE_ERROR);
		return;
	}

	if (!is_module_init(prev_state) && is_module_init(new_state)) {
		module_set_state(MODULE_STATE_READY);
	}
}

static int update_scan_control(enum state prev_state, enum state new_state)
{
	__ASSERT_NO_MSG(!is_module_init(prev_state) || is_module_init(new_state));

	if (!is_module_init(prev_state)) {
		if (!is_module_init(new_state) || (new_state == STATE_ERROR)) {
			/* Do not perform actions before initialized or in the error state. */
			return 0;
		}

		scan_init();
	}

	int err = 0;

	switch (new_state) {
	case STATE_ACTIVE:
	case STATE_FORCED_ACTIVE:
		err = scan_start();
		break;

	case STATE_DELAYED_ACTIVE:
	case STATE_INACTIVE:
	case STATE_OFF:
	case STATE_ERROR:
		err = scan_stop();
		break;

	default:
		break;
	}

	return err;
}

static void update_work(enum state prev_state, enum state new_state)
{
	switch (new_state) {
	case STATE_ACTIVE:
		/* Schedule work only if state transition is possible. */
		if (update_state_transition(STATE_ACTIVE, STATE_INACTIVE, false) ==
		    STATE_INACTIVE) {
			(void)k_work_reschedule(&update_state_work, K_MSEC(SCAN_DURATION_MS));
		} else {
			(void)k_work_cancel_delayable(&update_state_work);
		}
		break;

	case STATE_FORCED_ACTIVE:
		(void)k_work_reschedule(&update_state_work, K_MSEC(FORCED_SCAN_DURATION_MS));
		break;

	case STATE_DELAYED_ACTIVE:
		(void)k_work_reschedule(&update_state_work, K_MSEC(SCAN_START_DELAY_MS));
		break;

	case STATE_INACTIVE:
		/* Schedule work only if state transition is possible. */
		if (update_state_transition(STATE_INACTIVE, STATE_ACTIVE, false) ==
		    STATE_ACTIVE) {
			scan_start_counter = 0;
			(void)k_work_reschedule(&update_state_work, K_MSEC(SCAN_START_CHECK_MS));
		} else {
			(void)k_work_cancel_delayable(&update_state_work);
		}
		break;

	case STATE_OFF:
	case STATE_ERROR:
		(void)k_work_cancel_delayable(&update_state_work);
		break;

	default:
		/* Do nothing. */
		break;
	}
}

static int update_state_internal(enum state prev_state, enum state new_state)
{
	update_work(prev_state, new_state);
	broadcast_module_state(prev_state, new_state);
	return update_scan_control(prev_state, new_state);
}

static void update_state(enum state new_state)
{
	/* Unrecoverable error. */
	if (state == STATE_ERROR) {
		return;
	}

	/* Check if requested state transition is possible. Update new state if needed. */
	new_state = update_state_transition(state, new_state, true);
	int err = update_state_internal(state, new_state);

	if (err) {
		(void)update_state_internal(state, STATE_ERROR);
		state = STATE_ERROR;
		LOG_ERR("Fatal error (err: %d)", err);
	} else {
		state = new_state;
		LOG_DBG("State: %s, peers_only: %s", state2str(state),
			peers_only ? "true" : "false");
	}
}

static void update_state_work_fn(struct k_work *w)
{
	ARG_UNUSED(w);

	switch (state) {
	case STATE_ACTIVE:
		if (IS_ENABLED(CONFIG_DESKTOP_BLE_NEW_PEER_SCAN_REQUEST)) {
			peers_only = true;
		}
		update_state(STATE_INACTIVE);
		break;

	case STATE_FORCED_ACTIVE:
		update_state(STATE_ACTIVE);
		break;

	case STATE_DELAYED_ACTIVE:
		update_state(STATE_FORCED_ACTIVE);
		break;

	case STATE_INACTIVE:
		scan_start_counter += SCAN_START_CHECK_MS;
		if (scan_start_counter >= SCAN_START_TIMEOUT_MS) {
			scan_start_counter = 0;
			update_state(STATE_ACTIVE);
		} else {
			(void)k_work_reschedule(&update_state_work, K_MSEC(SCAN_START_CHECK_MS));
		}
		break;

	default:
		/* Should not happen. */
		__ASSERT_NO_MSG(false);
		break;
	}
}

static bool handle_hid_report_event(const struct hid_report_event *event)
{
	/* Ignore HID output reports. Subscriber is NULL for a HID output report. */
	if (!event->subscriber) {
		return false;
	}

	if (state == STATE_INACTIVE) {
		/* Avoid complete state update and only zero scan start counter to block scan start.
		 * This reduces number of operations done in HID report event handler and helps to
		 * avoid negatively affecting HID report rate.
		 */
		scan_start_counter = 0;
	} else if (state == STATE_ACTIVE) {
		update_state(STATE_INACTIVE);
	}

	return false;
}

static bool handle_module_state_event(const struct module_state_event *event)
{
	if (check_state(event, MODULE_ID(ble_state), MODULE_STATE_READY)) {
		switch (state) {
		case STATE_DISABLED:
			update_state(STATE_INITIALIZED);
			break;

		case STATE_DISABLED_OFF:
			update_state(STATE_INITIALIZED_OFF);
			break;

		default:
			/* Should not happen. */
			__ASSERT_NO_MSG(false);
			update_state(STATE_ERROR);
			break;
		}
	} else if (check_state(event, MODULE_ID(ble_bond), MODULE_STATE_READY)) {
		/* Settings need to be loaded on start before the scan begins.
		 * As the ble_bond module reports its READY state also on wake_up_event,
		 * to avoid multiple scan start triggering on wake-up scan will be started
		 * from here only on start-up.
		 */
		switch (state) {
		case STATE_INITIALIZED:
			update_state(STATE_FORCED_ACTIVE);
			break;

		case STATE_INITIALIZED_OFF:
			update_state(STATE_OFF);
			break;

		/* ble_scan module must be initialized before ble_bond module. */
		case STATE_DISABLED:
		case STATE_DISABLED_OFF:
			__ASSERT_NO_MSG(false);
			update_state(STATE_ERROR);
			break;

		default:
			/* Ignore the event. */
			break;
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
			peer_discovery_end(NULL);
		}

		if (state != STATE_OFF) {
			update_state(STATE_DELAYED_ACTIVE);
		}
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
		reset_subscribers(true);
		/* Fall-through */

	case PEER_OPERATION_SCAN_REQUEST:
		if (IS_ENABLED(CONFIG_BT_SCAN_CONN_ATTEMPTS_FILTER)) {
			bt_scan_conn_attempts_filter_clear();
		}
		if (IS_ENABLED(CONFIG_DESKTOP_BLE_NEW_PEER_SCAN_REQUEST)) {
			peers_only = false;
		}

		if (state != STATE_OFF) {
			update_state(STATE_FORCED_ACTIVE);
		}
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
	peer_discovery_end(event);
	if (IS_ENABLED(CONFIG_BT_SCAN_CONN_ATTEMPTS_FILTER)) {
		bt_scan_conn_attempts_filter_clear();
	}

	/* Cannot start scanning right after discovery - problems establishing security.
	 * Delay scan start to workaround the issue.
	 */
	if (state != STATE_OFF) {
		update_state(STATE_DELAYED_ACTIVE);
	}

	return false;
}

static bool handle_power_down_event(const struct power_down_event *event)
{
	switch (state) {
	case STATE_DISABLED:
		update_state(STATE_DISABLED_OFF);
		break;

	case STATE_INITIALIZED:
		update_state(STATE_INITIALIZED_OFF);
		break;

	case STATE_ACTIVE:
	case STATE_DELAYED_ACTIVE:
	case STATE_INACTIVE:
	case STATE_FORCED_ACTIVE:
		update_state(STATE_OFF);
		break;

	default:
		/* Ignore event. */
		break;
	}

	return false;
}

static bool handle_wake_up_event(const struct wake_up_event *event)
{
	switch (state) {
	case STATE_DISABLED_OFF:
		update_state(STATE_DISABLED);
		break;

	case STATE_INITIALIZED_OFF:
		update_state(STATE_INITIALIZED);
		break;

	case STATE_OFF:
		update_state(STATE_FORCED_ACTIVE);
		break;

	default:
		/* Ignore event. */
		break;
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
