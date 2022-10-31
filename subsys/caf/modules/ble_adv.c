/*
 * Copyright (c) 2018-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <zephyr/types.h>
#include <zephyr/settings/settings.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>

#include <bluetooth/adv_prov.h>

#define MODULE ble_adv
#include <caf/events/module_state_event.h>
#include <caf/events/ble_common_event.h>
#include <caf/events/power_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_CAF_BLE_ADV_LOG_LEVEL);

#define MAX_KEY_LEN 30
#define PEER_IS_RPA_STORAGE_NAME "peer_is_rpa_"

enum state {
	STATE_DISABLED,
	STATE_DISABLED_OFF,
	STATE_OFF,
	STATE_IDLE,
	STATE_ACTIVE_FAST,
	STATE_ACTIVE_SLOW,
	STATE_ACTIVE_FAST_DIRECT,
	STATE_ACTIVE_SLOW_DIRECT,
	STATE_DELAYED_ACTIVE_FAST,
	STATE_DELAYED_ACTIVE_SLOW,
	STATE_GRACE_PERIOD
};

static enum state state;
static size_t req_grace_period_s;

static struct k_work_delayable adv_update;
static struct k_work_delayable grace_period_end;
static uint8_t cur_identity = BT_ID_DEFAULT; /* We expect zero */

enum peer_rpa {
	PEER_RPA_ERASED,
	PEER_RPA_YES,
	PEER_RPA_NO,
};

static enum peer_rpa peer_is_rpa[CONFIG_BT_ID_MAX];


static int settings_set(const char *key, size_t len_rd, settings_read_cb read_cb, void *cb_arg)
{
	/* Assuming ID is written as one digit */
	if (!strncmp(key, PEER_IS_RPA_STORAGE_NAME,
	     sizeof(PEER_IS_RPA_STORAGE_NAME) - 1)) {
		char *end;
		long int read_id = strtol(key + strlen(key) - 1, &end, 10);

		if ((*end != '\0') || (read_id < 0) || (read_id >= CONFIG_BT_ID_MAX)) {
			LOG_ERR("Identity is not a valid number");
			return -ENOTSUP;
		}

		ssize_t len = read_cb(cb_arg, &peer_is_rpa[read_id],
				  sizeof(peer_is_rpa[read_id]));

		if ((len != sizeof(peer_is_rpa[read_id])) || (len != len_rd)) {
			LOG_ERR("Can't read peer_is_rpa%ld from storage",
				read_id);
			return len;
		}
	}

	return 0;
}

#if CONFIG_CAF_BLE_ADV_DIRECT_ADV
SETTINGS_STATIC_HANDLER_DEFINE(ble_adv, MODULE_NAME, NULL, settings_set, NULL, NULL);
#endif

static void broadcast_adv_state(bool active)
{
	struct ble_peer_search_event *event = new_ble_peer_search_event();
	event->active = active;
	APP_EVENT_SUBMIT(event);

	LOG_INF("Advertising %s", (active)?("started"):("stopped"));
}

static int ble_adv_stop(void)
{
	int err = bt_le_adv_stop();
	if (err) {
		LOG_ERR("Cannot stop advertising (err %d)", err);
	} else {
		k_work_cancel_delayable(&adv_update);

		if (IS_ENABLED(CONFIG_CAF_BLE_ADV_GRACE_PERIOD)) {
			k_work_cancel_delayable(&grace_period_end);
		}

		state = STATE_IDLE;

		broadcast_adv_state(false);
	}

	return err;
}

static void conn_find_cb(struct bt_conn *conn, void *data)
{
	struct bt_conn **temp_conn = data;

	/* Peripheral can have only one Bluetooth connection per
	 * Bluetooth local identity.
	 */
	__ASSERT_NO_MSG((*temp_conn) == NULL);
	(*temp_conn) = conn;
}

static inline struct bt_conn *conn_get(void)
{
	struct bt_conn *conn = NULL;

	bt_conn_foreach(BT_CONN_TYPE_LE, conn_find_cb, &conn);

	return conn;
}

static void bond_addr_get_cb(const struct bt_bond_info *info, void *user_data)
{
	bt_addr_le_t *addr = user_data;

	/* Return address of the first found bond. */
	if (!bt_addr_le_cmp(addr, BT_ADDR_LE_ANY)) {
		bt_addr_le_copy(addr, &info->addr);
	}
}

static inline void bond_addr_get(uint8_t local_id, bt_addr_le_t *addr)
{
	bt_addr_le_copy(addr, BT_ADDR_LE_ANY);
	bt_foreach_bond(local_id, bond_addr_get_cb, addr);
}

static void bond_cnt_cb(const struct bt_bond_info *info, void *user_data)
{
	size_t *cnt = user_data;

	(*cnt)++;
}

static inline size_t bond_cnt(uint8_t local_id)
{
	size_t cnt = 0;

	bt_foreach_bond(local_id, bond_cnt_cb, &cnt);

	return cnt;
}

static int ble_adv_start_directed(bool fast_adv)
{
	bt_addr_le_t addr;

	bond_addr_get(cur_identity, &addr);
	__ASSERT_NO_MSG(bt_addr_le_cmp(&addr, BT_ADDR_LE_ANY));

	struct bt_le_adv_param adv_param;

	if (fast_adv) {
		LOG_INF("Use fast advertising");
		adv_param = *BT_LE_ADV_CONN_DIR(&addr);
	} else {
		adv_param = *BT_LE_ADV_CONN_DIR_LOW_DUTY(&addr);
	}

	adv_param.id = cur_identity;

	int err = bt_le_adv_start(&adv_param, NULL, 0, NULL, 0);

	if (err) {
		return err;
	}

	char addr_str[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(&addr, addr_str, sizeof(addr_str));

	LOG_INF("Direct advertising to %s", addr_str);

	return 0;
}

static int update_undirected_advertising(struct bt_le_adv_param *adv_param, bool in_grace_period)
{
	size_t ad_len = bt_le_adv_prov_get_ad_prov_cnt();
	size_t sd_len = bt_le_adv_prov_get_sd_prov_cnt();
	struct bt_data ad[ad_len];
	struct bt_data sd[sd_len];

	struct bt_le_adv_prov_adv_state state;
	struct bt_le_adv_prov_feedback fb;

	state.pairing_mode = (bond_cnt(cur_identity) == 0);
	state.in_grace_period = in_grace_period;
	req_grace_period_s = 0;

	int err = bt_le_adv_prov_get_ad(ad, &ad_len, &state, &fb);

	if (err) {
		LOG_ERR("Cannot get advertising data (err: %d)", err);
		return err;
	}

	req_grace_period_s = MAX(req_grace_period_s, fb.grace_period_s);

	err = bt_le_adv_prov_get_sd(sd, &sd_len, &state, &fb);
	if (err) {
		LOG_ERR("Cannot get scan response data (err: %d)", err);
		return err;
	}

	req_grace_period_s = MAX(req_grace_period_s, fb.grace_period_s);

	if (req_grace_period_s > 0) {
		__ASSERT_NO_MSG(IS_ENABLED(CONFIG_CAF_BLE_ADV_GRACE_PERIOD) ||
				!IS_ENABLED(CONFIG_CAF_BLE_ADV_PM_EVENTS));
	}

	if (adv_param) {
		return bt_le_adv_start(adv_param, ad, ad_len, sd, sd_len);
	} else {
		return bt_le_adv_update_data(ad, ad_len, sd, sd_len);
	}
}

static void setup_accept_list_cb(const struct bt_bond_info *info, void *user_data)
{
	int *bond_cnt = user_data;

	if ((*bond_cnt) < 0) {
		return;
	}

	int err = bt_le_filter_accept_list_add(&info->addr);

	if (err) {
		LOG_ERR("Cannot add peer to filter accept list (err: %d)", err);
		(*bond_cnt) = -EIO;
	} else {
		(*bond_cnt)++;
	}
}

static int setup_accept_list(uint8_t local_id)
{
	if (!IS_ENABLED(CONFIG_CAF_BLE_ADV_FILTER_ACCEPT_LIST)) {
		return 0;
	}

	int err = bt_le_filter_accept_list_clear();

	if (err) {
		LOG_ERR("Cannot clear filter accept list (err: %d)", err);
		return err;
	}

	int bond_cnt = 0;

	bt_foreach_bond(local_id, setup_accept_list_cb, &bond_cnt);

	return bond_cnt;
}

static int ble_adv_start_undirected(bool fast_adv)
{
	struct bt_le_adv_param adv_param = {
		.options = BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_ONE_TIME,
	};

	LOG_INF("Use %s advertising", (fast_adv)?("fast"):("slow"));
	if (fast_adv) {
		BUILD_ASSERT(CONFIG_CAF_BLE_ADV_FAST_INT_MIN <= CONFIG_CAF_BLE_ADV_FAST_INT_MAX);
		adv_param.interval_min = CONFIG_CAF_BLE_ADV_FAST_INT_MIN;
		adv_param.interval_max = CONFIG_CAF_BLE_ADV_FAST_INT_MAX;
	} else {
		BUILD_ASSERT(CONFIG_CAF_BLE_ADV_SLOW_INT_MIN <= CONFIG_CAF_BLE_ADV_SLOW_INT_MAX);
		adv_param.interval_min = CONFIG_CAF_BLE_ADV_SLOW_INT_MIN;
		adv_param.interval_max = CONFIG_CAF_BLE_ADV_SLOW_INT_MAX;
	}

	adv_param.id = cur_identity;

	int allowed_cnt = setup_accept_list(cur_identity);

	if (allowed_cnt < 0) {
		return allowed_cnt;
	} else if (allowed_cnt > 0) {
		adv_param.options |= BT_LE_ADV_OPT_FILTER_SCAN_REQ;
		adv_param.options |= BT_LE_ADV_OPT_FILTER_CONN;
	}

	return update_undirected_advertising(&adv_param, false);
}

static int ble_adv_start(bool can_fast_adv)
{
	bool fast_adv = IS_ENABLED(CONFIG_CAF_BLE_ADV_FAST_ADV) && can_fast_adv;

	int err = ble_adv_stop();
	if (err) {
		LOG_ERR("Cannot stop advertising (err %d)", err);
		goto error;
	}

	if (conn_get()) {
		LOG_INF("Already connected, do not advertise");
		return 0;
	}

	bool direct = false;

	/* Use direct advertising only if the local identity in use has exactly one bond. */
	if (IS_ENABLED(CONFIG_CAF_BLE_ADV_DIRECT_ADV) &&
	    bond_cnt(cur_identity) == 1) {
		/* Direct advertising only to peer without RPA. */
		direct = (peer_is_rpa[cur_identity] != PEER_RPA_YES);
	}

	if (direct) {
		err = ble_adv_start_directed(fast_adv);
	} else {
		err = ble_adv_start_undirected(fast_adv);
	}

	if (err) {
		LOG_ERR("Advertising failed to start (err %d)", err);
		goto error;
	}

	if (direct) {
		if (fast_adv) {
			state = STATE_ACTIVE_FAST_DIRECT;
		} else {
			state = STATE_ACTIVE_SLOW_DIRECT;
		}
	} else {
		if (fast_adv) {
			k_work_reschedule(&adv_update,
					  K_SECONDS(CONFIG_CAF_BLE_ADV_FAST_ADV_TIMEOUT));
			state = STATE_ACTIVE_FAST;
		} else {
			state = STATE_ACTIVE_SLOW;
		}
	}

	broadcast_adv_state(true);
error:
	return err;
}

static void grace_period_fn(struct k_work *work)
{
	int err = ble_adv_stop();

	if (err) {
		module_set_state(MODULE_STATE_ERROR);
	} else {
		state = STATE_OFF;
		module_set_state(MODULE_STATE_OFF);
	}
}

static int enter_grace_period(void)
{
	size_t used_grace_period = req_grace_period_s;
	int err = update_undirected_advertising(NULL, true);

	if (!err) {
		LOG_INF("Entered grace period");

		k_work_cancel_delayable(&adv_update);
		k_work_reschedule(&grace_period_end, K_SECONDS(used_grace_period));

		state = STATE_GRACE_PERIOD;
	} else if (err == -EAGAIN) {
		LOG_INF("No active advertising");
		err = ble_adv_stop();
		if (!err) {
			state = STATE_OFF;
			module_set_state(MODULE_STATE_OFF);
		}
	} else {
		LOG_ERR("Cannot modify advertising data (err %d)", err);
	}

	return err;
}

static void ble_adv_update_fn(struct k_work *work)
{
	bool can_fast_adv = false;

	switch (state) {
	case STATE_DELAYED_ACTIVE_FAST:
		can_fast_adv = true;
		break;

	case STATE_ACTIVE_FAST:
	case STATE_DELAYED_ACTIVE_SLOW:
		break;

	default:
		/* Should not happen. */
		__ASSERT_NO_MSG(false);
	}

	int err = ble_adv_start(can_fast_adv);

	if (err) {
		module_set_state(MODULE_STATE_ERROR);
	}
}

static void init(void)
{
	/* These things will be opt-out by the compiler. */
	if (!IS_ENABLED(CONFIG_CAF_BLE_ADV_DIRECT_ADV)) {
		ARG_UNUSED(settings_set);
	}

	k_work_init_delayable(&adv_update, ble_adv_update_fn);

	if (IS_ENABLED(CONFIG_CAF_BLE_ADV_GRACE_PERIOD)) {
		k_work_init_delayable(&grace_period_end, grace_period_fn);
	}

	/* We should not start advertising before ble_bond is ready.
	 * Stay in disabled state. */

	module_set_state(MODULE_STATE_READY);
}

static void start(void)
{
	int err = ble_adv_start(true);

	if (err) {
		module_set_state(MODULE_STATE_ERROR);
	}

	return;
}

static void update_peer_is_rpa(enum peer_rpa new_peer_rpa)
{
	if (IS_ENABLED(CONFIG_SETTINGS) &&
	    IS_ENABLED(CONFIG_CAF_BLE_ADV_DIRECT_ADV)) {
		peer_is_rpa[cur_identity] = new_peer_rpa;
		/* Assuming ID is written using only one digit. */
		__ASSERT_NO_MSG(cur_identity < 10);
		char key[MAX_KEY_LEN];
		int err = snprintk(key, sizeof(key), MODULE_NAME "/%s%d",
				   PEER_IS_RPA_STORAGE_NAME, cur_identity);

		if ((err > 0) && (err < MAX_KEY_LEN)) {
			err = settings_save_one(key, &peer_is_rpa[cur_identity],
					sizeof(peer_is_rpa[cur_identity]));
		}

		if (err) {
			LOG_ERR("Problem storing peer_is_rpa: (err = %d)", err);
		}
	}
}

static void disconnect_peer(struct bt_conn *conn)
{
	int err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);

	if (!err) {
		/* Submit event to let other application modules prepare for
		 * the disconnection.
		 */
		struct ble_peer_event *event = new_ble_peer_event();

		event->id = conn;
		event->state = PEER_STATE_DISCONNECTING;
		APP_EVENT_SUBMIT(event);

		LOG_INF("Peer disconnecting");
	} else if (err == -ENOTCONN) {
		LOG_INF("Peer already disconnected");
	} else {
		LOG_ERR("Failed to disconnect peer (err=%d)", err);
		module_set_state(MODULE_STATE_ERROR);
	}
}

static void ble_initialized(void)
{
	static bool started;

	if (started) {
		return;
	}

	switch (state) {
	case STATE_DISABLED:
		state = STATE_OFF;
		start();
		break;
	case STATE_DISABLED_OFF:
		state = STATE_OFF;
		break;
	default:
		/* Should not happen. */
		__ASSERT_NO_MSG(false);
		break;
	}

	started = true;
}

static bool handle_module_state_event(const struct module_state_event *event)
{
	if (check_state(event, MODULE_ID(ble_state), MODULE_STATE_READY)) {
		static bool initialized;

		__ASSERT_NO_MSG(!initialized);

		init();
		initialized = true;

		if (!IS_ENABLED(CONFIG_CAF_BLE_ADV_BLE_BOND_SUPPORTED)) {
			ble_initialized();
		}
	} else if (IS_ENABLED(CONFIG_CAF_BLE_ADV_BLE_BOND_SUPPORTED) &&
		   check_state(event, MODULE_ID(ble_bond), MODULE_STATE_READY)) {
		/* Settings need to be loaded before advertising start */
		ble_initialized();
	}

	return false;
}

static bool handle_ble_peer_event(const struct ble_peer_event *event)
{
	int err = 0;
	bool can_fast_adv = false;

	switch (event->state) {
	case PEER_STATE_CONNECTED:
	{
		bool wu = false;

		if (IS_ENABLED(CONFIG_CAF_BLE_ADV_GRACE_PERIOD) &&
		    (state == STATE_GRACE_PERIOD)) {
			wu = true;
		}
		err = ble_adv_stop();
		if (wu) {
			APP_EVENT_SUBMIT(new_wake_up_event());
		}
		break;
	}
	case PEER_STATE_SECURED:
		if (peer_is_rpa[cur_identity] == PEER_RPA_ERASED) {
			if (bt_addr_le_is_rpa(bt_conn_get_dst(event->id))) {
				update_peer_is_rpa(PEER_RPA_YES);
			} else {
				update_peer_is_rpa(PEER_RPA_NO);
			}
		}
		break;

	case PEER_STATE_DISCONNECTED:
		can_fast_adv = true;
		/* Fall-through */

	case PEER_STATE_CONN_FAILED:
		if (state != STATE_OFF) {
			state = can_fast_adv ?
				STATE_DELAYED_ACTIVE_FAST :
				STATE_DELAYED_ACTIVE_SLOW;

			k_work_reschedule(&adv_update, K_NO_WAIT);
		}
		break;

	default:
		/* Ignore */
		break;
	}

	if (err) {
		module_set_state(MODULE_STATE_ERROR);
	}

	return false;
}

static bool handle_ble_adv_data_update_event(const struct ble_adv_data_update_event *event)
{
	ARG_UNUSED(event);

	int err = 0;
	bool in_grace_period = false;

	switch (state) {
	case STATE_GRACE_PERIOD:
		in_grace_period = true;
		/* Fall-through */

	case STATE_ACTIVE_FAST:
	case STATE_ACTIVE_SLOW:
		err = update_undirected_advertising(NULL, in_grace_period);
		if (err) {
			LOG_ERR("Cannot modify advertising data (err %d)", err);
		}
		break;

	default:
		/* Advertising data will be updated on undirected advertising start. */
		break;
	}

	return false;
}

static bool handle_ble_peer_operation_event(const struct ble_peer_operation_event *event)
{
	switch (event->op)  {
	case PEER_OPERATION_SELECTED:
	case PEER_OPERATION_ERASE_ADV:
	case PEER_OPERATION_ERASE_ADV_CANCEL:
		if ((state == STATE_OFF) || (state == STATE_GRACE_PERIOD) ||
		    (state == STATE_DISABLED) || (state == STATE_DISABLED_OFF)) {
			cur_identity = event->bt_stack_id;
			__ASSERT_NO_MSG(cur_identity < CONFIG_BT_ID_MAX);
			break;
		}

		int err = ble_adv_stop();

		if (err) {
			module_set_state(MODULE_STATE_ERROR);
		}

		struct bt_conn *conn = conn_get();

		if (conn) {
			disconnect_peer(conn);
		}

		cur_identity = event->bt_stack_id;
		__ASSERT_NO_MSG(cur_identity < CONFIG_BT_ID_MAX);

		if (event->op == PEER_OPERATION_ERASE_ADV) {
			update_peer_is_rpa(PEER_RPA_ERASED);
		}
		if (!conn) {
			err = ble_adv_start(true);
			if (err) {
				module_set_state(MODULE_STATE_ERROR);
			}
		}
		break;

	case PEER_OPERATION_ERASED:
		__ASSERT_NO_MSG(cur_identity == event->bt_stack_id);
		__ASSERT_NO_MSG(cur_identity < CONFIG_BT_ID_MAX);
		break;

	case PEER_OPERATION_SELECT:
	case PEER_OPERATION_ERASE:
	case PEER_OPERATION_CANCEL:
		/* Ignore */
		break;

	case PEER_OPERATION_SCAN_REQUEST:
	default:
		__ASSERT_NO_MSG(false);
		break;
	}

	return false;
}

static bool handle_power_down_event(const struct power_down_event *event)
{
	int err = 0;

	switch (state) {
	case STATE_ACTIVE_FAST:
	case STATE_ACTIVE_SLOW:
		if (IS_ENABLED(CONFIG_CAF_BLE_ADV_GRACE_PERIOD) &&
		    (req_grace_period_s > 0)) {
			err = enter_grace_period();
		} else {
			err = ble_adv_stop();
			if (!err) {
				state = STATE_OFF;
				module_set_state(MODULE_STATE_OFF);
			}
		}
		break;

	case STATE_DELAYED_ACTIVE_FAST:
	case STATE_DELAYED_ACTIVE_SLOW:
	case STATE_ACTIVE_FAST_DIRECT:
	case STATE_ACTIVE_SLOW_DIRECT:
		err = ble_adv_stop();
		if (!err) {
			state = STATE_OFF;
			module_set_state(MODULE_STATE_OFF);
		}
		break;

	case STATE_IDLE:
		state = STATE_OFF;
		module_set_state(MODULE_STATE_OFF);
		break;

	case STATE_OFF:
	case STATE_GRACE_PERIOD:
	case STATE_DISABLED_OFF:
		/* No action */
		break;

	case STATE_DISABLED:
		state = STATE_DISABLED_OFF;
		break;

	default:
		/* Should never happen */
		__ASSERT_NO_MSG(false);
		break;
	}

	if (err) {
		module_set_state(MODULE_STATE_ERROR);
	}

	return (state != STATE_OFF) &&
	       (state != STATE_DISABLED);
}

static bool handle_wake_up_event(const struct wake_up_event *event)
{
	bool was_off = false;
	int err;

	switch (state) {
	case STATE_OFF:
		was_off = true;
		state = STATE_IDLE;
		/* Fall-through */

	case STATE_GRACE_PERIOD:
		err = ble_adv_start(true);

		if (err) {
			module_set_state(MODULE_STATE_ERROR);
		} else if (was_off) {
			module_set_state(MODULE_STATE_READY);
		}
		break;

	case STATE_IDLE:
	case STATE_ACTIVE_FAST:
	case STATE_ACTIVE_SLOW:
	case STATE_ACTIVE_FAST_DIRECT:
	case STATE_ACTIVE_SLOW_DIRECT:
	case STATE_DELAYED_ACTIVE_FAST:
	case STATE_DELAYED_ACTIVE_SLOW:
	case STATE_DISABLED:
		/* No action */
		break;

	case STATE_DISABLED_OFF:
		state = STATE_DISABLED;
		break;

	default:
		__ASSERT_NO_MSG(false);
		break;
	}

	return false;
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_module_state_event(aeh)) {
		return handle_module_state_event(cast_module_state_event(aeh));
	}

	if (is_ble_peer_event(aeh)) {
		return handle_ble_peer_event(cast_ble_peer_event(aeh));
	}

	if (is_ble_adv_data_update_event(aeh)) {
		return handle_ble_adv_data_update_event(cast_ble_adv_data_update_event(aeh));
	}

	if (IS_ENABLED(CONFIG_CAF_BLE_ADV_BLE_BOND_SUPPORTED) &&
	    is_ble_peer_operation_event(aeh)) {
		return handle_ble_peer_operation_event(cast_ble_peer_operation_event(aeh));
	}

	if (IS_ENABLED(CONFIG_CAF_BLE_ADV_PM_EVENTS) &&
	    is_power_down_event(aeh)) {
		return handle_power_down_event(cast_power_down_event(aeh));
	}

	if (IS_ENABLED(CONFIG_CAF_BLE_ADV_PM_EVENTS) &&
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
APP_EVENT_SUBSCRIBE(MODULE, ble_adv_data_update_event);
#if CONFIG_CAF_BLE_ADV_BLE_BOND_SUPPORTED
APP_EVENT_SUBSCRIBE(MODULE, ble_peer_operation_event);
#endif /* CONFIG_CAF_BLE_ADV_BLE_BOND_SUPPORTED */
#if CONFIG_CAF_BLE_ADV_PM_EVENTS
APP_EVENT_SUBSCRIBE(MODULE, power_down_event);
APP_EVENT_SUBSCRIBE(MODULE, wake_up_event);
#endif /* CONFIG_CAF_BLE_ADV_PM_EVENTS */
