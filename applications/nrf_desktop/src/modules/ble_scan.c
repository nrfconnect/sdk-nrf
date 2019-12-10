/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/scan.h>
#include <settings/settings.h>
#include <bluetooth/gatt_dm.h>

#include <string.h>

#define MODULE ble_scan
#include "module_state_event.h"
#include "hid_event.h"
#include "ble_event.h"

#include "ble_scan_def.h"

#include "ble_controller_hci_vs.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_BLE_SCANNING_LOG_LEVEL);

#define SCAN_TRIG_CHECK_MS    K_SECONDS(1)
#define SCAN_TRIG_TIMEOUT_MS  K_SECONDS(CONFIG_DESKTOP_BLE_SCAN_START_TIMEOUT_S)
#define SCAN_DURATION_MS      K_SECONDS(CONFIG_DESKTOP_BLE_SCAN_DURATION_S)

#define SUBSCRIBED_PEERS_STORAGE_NAME "subscribers"

struct subscriber_data {
	u8_t conn_count;
	u8_t peer_count;
};

struct subscribed_peer {
	bt_addr_le_t addr;
	enum peer_type peer_type;
};

static struct subscribed_peer subscribed_peers[CONFIG_BT_MAX_PAIRED];

static struct bt_conn *discovering_peer_conn;
static unsigned int scan_counter;
static struct k_delayed_work scan_start_trigger;
static struct k_delayed_work scan_stop_trigger;
static bool peers_only = !IS_ENABLED(CONFIG_DESKTOP_BLE_NEW_PEER_SCAN_ON_BOOT);
static bool scanning;


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
	EVENT_SUBMIT(event);
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

	k_delayed_work_cancel(&scan_stop_trigger);

	if (count_conn() < CONFIG_BT_MAX_CONN) {
		scan_counter = 0;
		k_delayed_work_submit(&scan_start_trigger, SCAN_TRIG_CHECK_MS);
	}
}

static int configure_address_filters(u8_t *filter_mode)
{
	size_t i;
	int err = 0;

	for (i = 0; i < ARRAY_SIZE(subscribed_peers); i++) {
		if (!bt_addr_le_cmp(&subscribed_peers[i].addr, BT_ADDR_LE_NONE)) {
			break;
		}

		struct bt_conn *conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT,
						&subscribed_peers[i].addr);

		if (conn) {
			bt_conn_unref(conn);
			continue;
		}

		err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_ADDR,
					 &subscribed_peers[i].addr);
		if (err) {
			LOG_ERR("Address filter cannot be added (err %d)", err);
			break;
		}

		char addr_str[BT_ADDR_LE_STR_LEN];

		bt_addr_le_to_str(&subscribed_peers[i].addr, addr_str,
				  sizeof(addr_str));
		LOG_INF("Address filter added %s", log_strdup(addr_str));
		*filter_mode |= BT_SCAN_ADDR_FILTER;
	}

	return err;
}

static int configure_short_name_filters(u8_t *filter_mode)
{
	u8_t peers_mask = 0;
	int err = 0;

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
			break;
		}
		*filter_mode |= BT_SCAN_SHORT_NAME_FILTER;
	}

	if (!err) {
		LOG_INF("Device type filters added");
	}

	return err;
}

static int configure_filters(void)
{
	BUILD_ASSERT_MSG(CONFIG_BT_MAX_PAIRED == CONFIG_BT_MAX_CONN, "");
	BUILD_ASSERT_MSG(CONFIG_BT_MAX_PAIRED <= CONFIG_BT_SCAN_ADDRESS_CNT,
			 "Insufficient number of address filters");
	BUILD_ASSERT_MSG(ARRAY_SIZE(peer_type_short_name) <=
			 CONFIG_BT_SCAN_SHORT_NAME_CNT,
			 "Insufficient number of short name filers");
	BUILD_ASSERT_MSG(ARRAY_SIZE(peer_type_short_name) == PEER_TYPE_COUNT,
			 "");
	bt_scan_filter_remove_all();

	u8_t filter_mode = 0;
	int err = configure_address_filters(&filter_mode);

	bool use_name_filters = true;

	if (IS_ENABLED(CONFIG_DESKTOP_BLE_NEW_PEER_SCAN_REQUEST) &&
	    peers_only) {
		use_name_filters = false;
	}

	if (!err && use_name_filters &&
	    (count_bond() < CONFIG_BT_MAX_PAIRED)) {
		err = configure_short_name_filters(&filter_mode);
	}

	if (!err) {
		err = bt_scan_filter_enable(filter_mode, false);
	} else {
		LOG_ERR("Filters cannot be turned on (err %d)", err);
	}

	return err;
}

static void scan_start(void)
{
	size_t conn_count = count_conn();
	size_t bond_count = count_bond();
	int err;

	if (IS_ENABLED(CONFIG_DESKTOP_BLE_NEW_PEER_SCAN_REQUEST) &&
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

	scanning = true;
	broadcast_scan_state(scanning);

	k_delayed_work_submit(&scan_stop_trigger, SCAN_DURATION_MS);
	k_delayed_work_cancel(&scan_start_trigger);

	return;

error:
	module_set_state(MODULE_STATE_ERROR);
}

static void scan_start_trigger_fn(struct k_work *w)
{
	BUILD_ASSERT_MSG((SCAN_TRIG_TIMEOUT_MS > SCAN_TRIG_CHECK_MS) &&
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
	BUILD_ASSERT_MSG(SCAN_DURATION_MS > 0, "");

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

	bt_addr_le_to_str(device_info->addr, addr, sizeof(addr));

	LOG_INF("Filters matched. %s %sconnectable",
		log_strdup(addr), connectable ? "" : "non");

	scan_stop();
}

static void scan_connecting_error(struct bt_scan_device_info *device_info)
{
	LOG_WRN("Connecting failed");
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

	if (IS_ENABLED(CONFIG_BT_LL_NRFXLIB)) {
		LOG_INF("Keep LLPM params");
		return false;
	}

	param->interval_min = 6;
	param->interval_max = 6;

	return true;
}

static int store_subscribed_peers(void)
{
	if (IS_ENABLED(CONFIG_SETTINGS)) {
		char key[] = MODULE_NAME "/" SUBSCRIBED_PEERS_STORAGE_NAME;
		int err = settings_save_one(key, subscribed_peers,
					    sizeof(subscribed_peers));

		if (err) {
			LOG_ERR("Problem storing subscribed_peers (err %d)",
				err);
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
	}
}

static int settings_set(const char *key, size_t len_rd,
			settings_read_cb read_cb, void *cb_arg)
{
	if (!strcmp(key, SUBSCRIBED_PEERS_STORAGE_NAME)) {
		ssize_t len = read_cb(cb_arg, &subscribed_peers,
				      sizeof(subscribed_peers));
		if (len != sizeof(subscribed_peers)) {
			LOG_ERR("Can't read subscribed_peers from storage");
			module_set_state(MODULE_STATE_ERROR);
			return len;
		}
	}

	return 0;
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

static int verify_subscribed_peers(void)
{
	/* On commit we should verify data to prevent inconsistency.
	 * Inconsistency could be caused e.g. by reset after secure,
	 * but before storing peer type in ble_scan module.
	 */
	bt_foreach_bond(BT_ID_DEFAULT, verify_bond, NULL);

	return 0;
}

static int settings_init(void)
{
	if (IS_ENABLED(CONFIG_SETTINGS)) {
		static struct settings_handler sh = {
			.name = MODULE_NAME,
			.h_set = settings_set,
			.h_commit = verify_subscribed_peers,
		};

		int err = settings_register(&sh);

		if (err) {
			LOG_ERR("Cannot register settings (err %d)", err);
			return err;
		}
	}

	return 0;
}

BT_SCAN_CB_INIT(scan_cb, scan_filter_match, NULL,
		scan_connecting_error, scan_connecting);

static void scan_init(void)
{
	reset_subscribers();

	int err = settings_init();

	if (err) {
		module_set_state(MODULE_STATE_ERROR);
		return;
	}

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

	bt_scan_cb_register(&scan_cb);

	static struct bt_conn_cb conn_callbacks = {
		.le_param_req = le_param_req,
	};

	bt_conn_cb_register(&conn_callbacks);
	k_delayed_work_init(&scan_start_trigger, scan_start_trigger_fn);
	k_delayed_work_init(&scan_stop_trigger, scan_stop_trigger_fn);
}

static void set_conn_params(struct bt_conn *conn, bool peer_llpm_support)
{
	int err;

	if (peer_llpm_support && IS_ENABLED(CONFIG_BT_LL_NRFXLIB)) {
		struct net_buf *buf;

		hci_vs_cmd_conn_update_t *cmd_conn_update;

		buf = bt_hci_cmd_create(HCI_VS_OPCODE_CMD_CONN_UPDATE,
					sizeof(*cmd_conn_update));
		if (!buf) {
			LOG_ERR("Could not allocate command buffer");
			return;
		}

		u16_t conn_handle;

		err = bt_hci_get_conn_handle(conn, &conn_handle);
		if (err) {
			LOG_ERR("Failed obtaining conn_handle (err %d)", err);
			return;
		}

		cmd_conn_update = net_buf_add(buf, sizeof(*cmd_conn_update));
		cmd_conn_update->connection_handle   = conn_handle;
		cmd_conn_update->conn_interval_us    = 1000;
		cmd_conn_update->conn_latency        = 99;
		cmd_conn_update->supervision_timeout = 400;

		err = bt_hci_cmd_send_sync(HCI_VS_OPCODE_CMD_CONN_UPDATE, buf,
					   NULL);
	} else {
		struct bt_le_conn_param param = {
			.interval_min = 0x0006,
			.interval_max = 0x0006,
			.latency = 99,
			.timeout = 400,
		};

		err = bt_conn_le_param_update(conn, &param);
	}

	if (err) {
		LOG_ERR("Cannot set conn params (err:%d)", err);
	} else {
		LOG_INF("%s conn params set",
			peer_llpm_support ? "LLPM" : "BLE");
	}
}

static bool event_handler(const struct event_header *eh)
{
	if ((IS_ENABLED(CONFIG_DESKTOP_HID_MOUSE) && is_hid_mouse_event(eh)) ||
	    (IS_ENABLED(CONFIG_DESKTOP_HID_KEYBOARD) && is_hid_keyboard_event(eh)) ||
	    (IS_ENABLED(CONFIG_DESKTOP_HID_CONSUMER_CTRL) && is_hid_consumer_ctrl_event(eh))) {
		/* Do not scan when devices are in use. */
		scan_counter = 0;

		if (scanning) {
			scan_stop();
		}

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
			started = true;

			/* Settings need to be loaded before scan start */
			scan_start();
		}

		return false;
	}

	if (is_ble_peer_event(eh)) {
		const struct ble_peer_event *event =
			cast_ble_peer_event(eh);

		switch (event->state) {
		case PEER_STATE_CONNECTED:
		case PEER_STATE_SECURED:
			/* Ignore */
			break;
		case PEER_STATE_CONN_FAILED:
		case PEER_STATE_DISCONNECTED:
			if (discovering_peer_conn == event->id) {
				bt_conn_unref(discovering_peer_conn);
				discovering_peer_conn = NULL;
			}
			scan_stop();
			/* ble_state keeps reference to connection object.
			 * Cannot create new connection now.
			 */
			k_delayed_work_submit(&scan_start_trigger, 0);
			scan_counter = SCAN_TRIG_TIMEOUT_MS;
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
			reset_subscribers();
			store_subscribed_peers();
			if (count_conn() == CONFIG_BT_MAX_CONN) {
				peers_only = false;
				break;
			}
			/* Fall-through */

		case PEER_OPERATION_SCAN_REQUEST:
			peers_only = false;
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
		size_t i;

		for (i = 0; i < ARRAY_SIZE(subscribed_peers); i++) {
			/* Already saved. */
			if (!bt_addr_le_cmp(&subscribed_peers[i].addr,
				  bt_conn_get_dst(discovering_peer_conn))) {
				break;
			}

			/* Save data about new subscriber. */
			if (!bt_addr_le_cmp(&subscribed_peers[i].addr,
					    BT_ADDR_LE_NONE)) {
				bt_addr_le_copy(&subscribed_peers[i].addr,
					bt_conn_get_dst(discovering_peer_conn));
				subscribed_peers[i].peer_type =
					event->peer_type;
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
		k_delayed_work_submit(&scan_start_trigger,
				      SCAN_TRIG_TIMEOUT_MS);
		scan_counter = SCAN_TRIG_TIMEOUT_MS;

		set_conn_params(bt_gatt_dm_conn_get(event->dm),
				event->peer_llpm_support);

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
