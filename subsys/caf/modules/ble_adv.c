/*
 * Copyright (c) 2018-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <zephyr/types.h>
#include <zephyr/random/rand32.h>
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
	STATE_ACTIVE,
	STATE_DELAYED_ACTIVE,
	STATE_GRACE_PERIOD,
	STATE_ERROR,
};

static enum state state;
static size_t grace_period_s;
static bool direct_adv;
static bool fast_adv;
static bool rpa_rotated;

static size_t req_grace_period_s;
static bool req_wakeup;
static bool req_fast_adv = true;
static bool req_new_adv_session = true;

static struct k_work adv_delayed_start;
static struct k_work_delayable fast_adv_end;
static struct k_work_delayable grace_period_end;
static struct k_work_delayable rpa_rotate;
static uint8_t cur_identity = BT_ID_DEFAULT; /* We expect zero */

enum peer_rpa {
	PEER_RPA_ERASED,
	PEER_RPA_YES,
	PEER_RPA_NO,
};

static enum peer_rpa peer_is_rpa[CONFIG_BT_ID_MAX];


static void update_state(enum state new_state);

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

static void broadcast_adv_state(bool new_active)
{
	static bool active;

	if (active == new_active) {
		return;
	}

	active = new_active;

	struct ble_peer_search_event *event = new_ble_peer_search_event();
	event->active = active;
	APP_EVENT_SUBMIT(event);

	LOG_INF("Advertising %s", (active)?("started"):("stopped"));
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

static struct bt_conn *conn_get(void)
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

static void bond_addr_get(uint8_t local_id, bt_addr_le_t *addr)
{
	bt_addr_le_copy(addr, BT_ADDR_LE_ANY);
	bt_foreach_bond(local_id, bond_addr_get_cb, addr);
}

static void bond_cnt_cb(const struct bt_bond_info *info, void *user_data)
{
	size_t *cnt = user_data;

	(*cnt)++;
}

static size_t bond_cnt(uint8_t local_id)
{
	size_t cnt = 0;

	bt_foreach_bond(local_id, bond_cnt_cb, &cnt);

	return cnt;
}

static bool can_direct_adv(uint8_t local_id)
{
	if (!IS_ENABLED(CONFIG_CAF_BLE_ADV_DIRECT_ADV)) {
		return false;
	}

	return ((bond_cnt(local_id) == 1) && (peer_is_rpa[local_id] != PEER_RPA_YES));
}

static void reschedule_rpa_rotation(void)
{
#if CONFIG_CAF_BLE_ADV_ROTATE_RPA
	/* Due to the Bluetooth API limitations, it is necessary to manully
	 * rotate the RPA before the actual timeout in the Bluetooth stack.
	 * As a result, the module timeout value is smaller than the Bluetooth
	 * stack configuration.
	 */
	BUILD_ASSERT(CONFIG_CAF_BLE_ADV_ROTATE_RPA_TIMEOUT < CONFIG_BT_RPA_TIMEOUT);
#endif
	if (!IS_ENABLED(CONFIG_CAF_BLE_ADV_ROTATE_RPA)) {
		return;
	}

	unsigned int rpa_timeout_ms = CONFIG_CAF_BLE_ADV_ROTATE_RPA_TIMEOUT * MSEC_PER_SEC;
	uint8_t rand;
	int err = sys_csrand_get(&rand, sizeof(rand));

	if (!err) {
		rpa_timeout_ms -=
			(CONFIG_CAF_BLE_ADV_ROTATE_RPA_TIMEOUT_RAND * MSEC_PER_SEC) *
			rand / UINT8_MAX;
	} else {
		LOG_WRN("Cannot get random RPA timeout (err: %d). Used fixed value", err);
	}

	(void)k_work_reschedule(&rpa_rotate, K_MSEC(rpa_timeout_ms));
}

static void force_rpa_rotation(uint8_t local_id)
{
	if (!IS_ENABLED(CONFIG_CAF_BLE_ADV_ROTATE_RPA)) {
		return;
	}

	struct bt_le_oob oob;
	int err = bt_le_oob_get_local(local_id, &oob);

	if (err) {
		LOG_ERR("Cannot trigger RPA rotation on ID: %" PRIu8 " (err: %d)", local_id, err);
	} else {
		LOG_INF("RPA rotated on ID: %" PRIu8, local_id);
		rpa_rotated = true;
	}
}

static int ble_adv_start_directed(void)
{
	bt_addr_le_t addr;

	bond_addr_get(cur_identity, &addr);
	__ASSERT_NO_MSG(bt_addr_le_cmp(&addr, BT_ADDR_LE_ANY));

	struct bt_le_adv_param adv_param;

	__ASSERT_NO_MSG(can_direct_adv(cur_identity));

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

	req_grace_period_s = 0;

	return 0;
}

static int update_undirected_advertising(struct bt_le_adv_param *adv_param)
{
	size_t ad_len = bt_le_adv_prov_get_ad_prov_cnt();
	size_t sd_len = bt_le_adv_prov_get_sd_prov_cnt();
	struct bt_data ad[ad_len];
	struct bt_data sd[sd_len];

	struct bt_le_adv_prov_adv_state adv_state;
	struct bt_le_adv_prov_feedback fb;

	if (req_new_adv_session) {
		force_rpa_rotation(cur_identity);
	}

	adv_state.pairing_mode = (bond_cnt(cur_identity) == 0);
	adv_state.in_grace_period = (state == STATE_GRACE_PERIOD);
	adv_state.new_adv_session = req_new_adv_session;
	adv_state.rpa_rotated = rpa_rotated;
	req_grace_period_s = 0;

	int err = bt_le_adv_prov_get_ad(ad, &ad_len, &adv_state, &fb);

	if (err) {
		LOG_ERR("Cannot get advertising data (err: %d)", err);
		return err;
	}

	req_grace_period_s = MAX(req_grace_period_s, fb.grace_period_s);

	err = bt_le_adv_prov_get_sd(sd, &sd_len, &adv_state, &fb);
	if (err) {
		LOG_ERR("Cannot get scan response data (err: %d)", err);
		return err;
	}

	req_grace_period_s = MAX(req_grace_period_s, fb.grace_period_s);

	if (req_grace_period_s > 0) {
		__ASSERT_NO_MSG(IS_ENABLED(CONFIG_CAF_BLE_ADV_GRACE_PERIOD) ||
				!IS_ENABLED(CONFIG_CAF_BLE_ADV_PM_EVENTS));
	}

	if (rpa_rotated) {
		rpa_rotated = false;
		reschedule_rpa_rotation();
	}

	if (req_new_adv_session) {
		req_new_adv_session = false;
	}

	if (adv_param) {
		return bt_le_adv_start(adv_param, ad, ad_len, sd, sd_len);
	} else {
		__ASSERT_NO_MSG(!adv_state.new_adv_session);
		__ASSERT_NO_MSG(!adv_state.rpa_rotated);

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

static int ble_adv_start_undirected(void)
{
	struct bt_le_adv_param adv_param = {
		.options = BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_ONE_TIME,
	};

	__ASSERT_NO_MSG((state == STATE_ACTIVE) || (state == STATE_GRACE_PERIOD));

	LOG_INF("Use %s advertising", (fast_adv) ? ("fast") : ("slow"));
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

	return update_undirected_advertising(&adv_param);
}

static void ble_adv_start(void)
{
	__ASSERT_NO_MSG((state == STATE_ACTIVE) || (state == STATE_GRACE_PERIOD));

	int err = bt_le_adv_stop();

	if (err) {
		LOG_ERR("Cannot stop advertising (err %d)", err);
		goto finish;
	}

	if (conn_get()) {
		LOG_INF("Already connected, do not advertise");
		return;
	}

	if (direct_adv) {
		err = ble_adv_start_directed();
	} else {
		err = ble_adv_start_undirected();
	}

	if (err) {
		LOG_ERR("Advertising failed to start (err %d)", err);
		goto finish;
	}

finish:
	if (err) {
		update_state(STATE_ERROR);
	} else {
		broadcast_adv_state(true);
	}
}

static void ble_adv_start_schedule(void)
{
	__ASSERT_NO_MSG((state == STATE_ACTIVE) || (state == STATE_GRACE_PERIOD) ||
			(state == STATE_DELAYED_ACTIVE));

	(void)k_work_submit(&adv_delayed_start);
}

static void ble_adv_data_update(void)
{
	if ((state != STATE_ACTIVE) && (state != STATE_GRACE_PERIOD)) {
		return;
	}

	if (direct_adv) {
		return;
	}

	int err = update_undirected_advertising(NULL);

	if (err == -EAGAIN) {
		LOG_INF("No active advertising");
	} else if (err) {
		update_state(STATE_ERROR);
	}
}

static void ble_adv_stop(void)
{
	__ASSERT_NO_MSG((state == STATE_OFF) || (state == STATE_IDLE) || (state == STATE_ERROR));

	int err = bt_le_adv_stop();

	if (err) {
		LOG_ERR("Cannot stop advertising (err %d)", err);
		update_state(STATE_ERROR);
	} else {
		broadcast_adv_state(false);
		req_grace_period_s = 0;
	}
}

static bool is_module_state_disabled(enum state state)
{
	return ((state == STATE_DISABLED) || (state == STATE_DISABLED_OFF));
}

static bool is_module_state_off(enum state state)
{
	return ((state == STATE_DISABLED_OFF) || (state == STATE_OFF));
}

static void broadcast_module_state(enum state prev_state, enum state new_state)
{
	if (new_state == STATE_ERROR) {
		module_set_state(MODULE_STATE_ERROR);
		return;
	}

	bool submit_ready = false;
	bool submit_off = false;

	__ASSERT_NO_MSG(is_module_state_disabled(prev_state) ||
			!is_module_state_disabled(new_state));

	if (is_module_state_disabled(prev_state)) {
		if (!is_module_state_disabled(new_state)) {
			submit_ready = true;

			if (is_module_state_off(new_state)) {
				submit_off = true;
			}
		}
	} else {
		if (is_module_state_off(prev_state) && !is_module_state_off(new_state)) {
			submit_ready = true;
		}

		if (!is_module_state_off(prev_state) && is_module_state_off(new_state)) {
			submit_off = true;
		}
	}

	if (submit_ready) {
		module_set_state(MODULE_STATE_READY);
	}

	if (submit_off) {
		module_set_state(MODULE_STATE_OFF);
	}

	if (req_wakeup) {
		if (IS_ENABLED(CONFIG_CAF_BLE_ADV_PM_EVENTS)) {
			APP_EVENT_SUBMIT(new_wake_up_event());
		}
		req_wakeup = false;
	}
}

static const char *state2str(enum state s)
{
	switch (s) {
	case STATE_DISABLED:
		return "DISABLED";

	case STATE_DISABLED_OFF:
		return "DISABLED_OFF";

	case STATE_OFF:
		return "OFF";

	case STATE_IDLE:
		return "IDLE";

	case STATE_ACTIVE:
		return "ACTIVE";

	case STATE_DELAYED_ACTIVE:
		return "DELAYED_ACTIVE";

	case STATE_GRACE_PERIOD:
		return "GRACE_PERIOD";

	case STATE_ERROR:
		return "ERROR";

	default:
		/* Should not happen. */
		__ASSERT_NO_MSG(false);
		return NULL;
	}
}

static void update_state_internal(enum state new_state)
{
	if (new_state == STATE_ACTIVE) {
		fast_adv = (req_fast_adv && IS_ENABLED(CONFIG_CAF_BLE_ADV_FAST_ADV));
		direct_adv = can_direct_adv(cur_identity);
	} else {
		fast_adv = false;
		direct_adv = false;
	}

	grace_period_s = req_grace_period_s;

	state = new_state;

	LOG_DBG("State: %s fast_adv: %d direct_adv: %d grace_period: %zu[s]",
		state2str(state), fast_adv, direct_adv, grace_period_s);
}

static void update_rpa_rotate_work(void)
{
	if (!IS_ENABLED(CONFIG_CAF_BLE_ADV_ROTATE_RPA)) {
		return;
	}

	if (((state != STATE_ACTIVE) && (state != STATE_GRACE_PERIOD)) || direct_adv) {
		(void)k_work_cancel_delayable(&rpa_rotate);
	}
}

static void update_grace_period_work(void)
{
	if (!IS_ENABLED(CONFIG_CAF_BLE_ADV_GRACE_PERIOD)) {
		return;
	}

	if (state == STATE_GRACE_PERIOD) {
		__ASSERT_NO_MSG(grace_period_s > 0);
		(void)k_work_reschedule(&grace_period_end, K_SECONDS(grace_period_s));
	} else {
		(void)k_work_cancel_delayable(&grace_period_end);
	}
}

static void update_fast_adv_work(void)
{
	if ((state == STATE_ACTIVE) && fast_adv && !direct_adv) {
		(void)k_work_reschedule(&fast_adv_end,
					K_SECONDS(CONFIG_CAF_BLE_ADV_FAST_ADV_TIMEOUT));
	} else {
		(void)k_work_cancel_delayable(&fast_adv_end);
	}
}

static void notify_ble_stack(void)
{
	(void)k_work_cancel(&adv_delayed_start);

	switch (state) {
	case STATE_DISABLED:
	case STATE_DISABLED_OFF:
		break;

	case STATE_OFF:
	case STATE_IDLE:
	case STATE_ERROR:
		ble_adv_stop();
		break;

	case STATE_DELAYED_ACTIVE:
		ble_adv_start_schedule();
		break;

	case STATE_ACTIVE:
	case STATE_GRACE_PERIOD:
		ble_adv_start();
		break;

	default:
		/* Should not happen. */
		__ASSERT_NO_MSG(false);
	}
}

static void update_state(enum state new_state)
{
	if (state == STATE_ERROR) {
		return;
	}

	enum state prev_state = state;

	update_state_internal(new_state);
	broadcast_module_state(prev_state, state);

	notify_ble_stack();
	update_rpa_rotate_work();
	update_grace_period_work();
	update_fast_adv_work();
}

static void adv_delayed_start_fn(struct k_work *work)
{
	ARG_UNUSED(work);

	if (state == STATE_DELAYED_ACTIVE) {
		update_state(STATE_ACTIVE);
	} else {
		ble_adv_start();
	}
}

static void grace_period_end_fn(struct k_work *work)
{
	ARG_UNUSED(work);

	update_state(STATE_OFF);
}

static void fast_adv_end_fn(struct k_work *work)
{
	ARG_UNUSED(work);

	__ASSERT_NO_MSG(req_fast_adv && fast_adv);
	__ASSERT_NO_MSG(state == STATE_ACTIVE);

	req_fast_adv = false;
	update_state(STATE_ACTIVE);

	__ASSERT_NO_MSG(!fast_adv);
}

static void rpa_rotate_fn(struct k_work *work)
{
	ARG_UNUSED(work);

	if (IS_ENABLED(CONFIG_CAF_BLE_ADV_GRACE_PERIOD) && (state == STATE_GRACE_PERIOD)) {
		/* Due to the Bluetooth API limitations, it is necessary to terminate
		 * the Grace Period session prematurely as there is no way to delay
		 * the RPA rotation.
		 */
		grace_period_end_fn(NULL);
		return;
	}

	__ASSERT_NO_MSG(state == STATE_ACTIVE);
	__ASSERT_NO_MSG(!direct_adv);

	int err = bt_le_adv_stop();

	if (!err) {
		__ASSERT_NO_MSG(!req_new_adv_session);

		force_rpa_rotation(cur_identity);

		err = ble_adv_start_undirected();
	}

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

	k_work_init(&adv_delayed_start, adv_delayed_start_fn);

	if (IS_ENABLED(CONFIG_CAF_BLE_ADV_FAST_ADV)) {
		k_work_init_delayable(&fast_adv_end, fast_adv_end_fn);
	}

	if (IS_ENABLED(CONFIG_CAF_BLE_ADV_ROTATE_RPA)) {
		k_work_init_delayable(&rpa_rotate, rpa_rotate_fn);
	}

	if (IS_ENABLED(CONFIG_CAF_BLE_ADV_GRACE_PERIOD)) {
		k_work_init_delayable(&grace_period_end, grace_period_end_fn);
	}
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
		update_state(STATE_ERROR);
	}
}

static void ble_ready(void)
{
	switch (state) {
	case STATE_DISABLED:
		update_state(STATE_ACTIVE);
		break;

	case STATE_DISABLED_OFF:
		update_state(STATE_OFF);
		break;

	default:
		/* Should not happen. */
		__ASSERT_NO_MSG(false);
		break;
	}
}

static void get_req_modules(struct module_flags *mf)
{
	BUILD_ASSERT(!(IS_ENABLED(CONFIG_BT_SETTINGS) && !IS_ENABLED(CONFIG_CAF_SETTINGS_LOADER)),
		     "Bluetooth settings enabled, but settings loader module is disabled");

	module_flags_set_bit(mf, MODULE_IDX(ble_state));

	if (IS_ENABLED(CONFIG_CAF_SETTINGS_LOADER)) {
		module_flags_set_bit(mf, MODULE_IDX(settings_loader));
	}

	if (IS_ENABLED(CONFIG_CAF_BLE_ADV_BLE_BOND_SUPPORTED)) {
		module_flags_set_bit(mf, MODULE_IDX(ble_bond));
	}
}

static bool handle_module_state_event(const struct module_state_event *event)
{
	static struct module_flags req_modules_bm;
	static bool initialized;

	if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
		__ASSERT_NO_MSG(!initialized);

		get_req_modules(&req_modules_bm);
		__ASSERT_NO_MSG(!module_flags_check_zero(&req_modules_bm));

		init();
		initialized = true;

		return false;
	}

	if (module_flags_check_zero(&req_modules_bm)) {
		/* Already initialized. */
		return false;
	}

	if (event->state == MODULE_STATE_READY) {
		__ASSERT_NO_MSG(initialized);

		module_flags_clear_bit(&req_modules_bm, module_idx_get(event->module_id));

		if (module_flags_check_zero(&req_modules_bm)) {
			ble_ready();
		}
	}

	return false;
}

static bool handle_ble_peer_event(const struct ble_peer_event *event)
{
	switch (event->state) {
	case PEER_STATE_CONNECTED:
		if (IS_ENABLED(CONFIG_CAF_BLE_ADV_GRACE_PERIOD) && (state == STATE_GRACE_PERIOD)) {
			req_wakeup = true;
		}

		update_state(STATE_IDLE);
		break;

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
		if (state != STATE_OFF) {
			req_new_adv_session = true;
			req_fast_adv = true;
			update_state(STATE_DELAYED_ACTIVE);
		}
		break;

	case PEER_STATE_CONN_FAILED:
		if ((state == STATE_ACTIVE) || (state == STATE_GRACE_PERIOD) ||
		    (state == STATE_DELAYED_ACTIVE)) {
			/* During direct advertising, connection failed is reported on advertising
			 * timeout. Switch to slow direct advertising in that case.
			 */
			if (direct_adv) {
				fast_adv_end_fn(NULL);
			}

			ble_adv_start_schedule();
		}
		break;

	default:
		/* Ignore */
		break;
	}

	return false;
}

static bool handle_ble_adv_data_update_event(const struct ble_adv_data_update_event *event)
{
	ARG_UNUSED(event);
	ble_adv_data_update();

	return false;
}

static bool handle_ble_peer_operation_event(const struct ble_peer_operation_event *event)
{
	switch (event->op)  {
	case PEER_OPERATION_SELECTED:
	case PEER_OPERATION_ERASE_ADV:
	case PEER_OPERATION_ERASE_ADV_CANCEL:
		cur_identity = event->bt_stack_id;
		__ASSERT_NO_MSG(cur_identity < CONFIG_BT_ID_MAX);

		if ((state == STATE_OFF) || (state == STATE_DISABLED) ||
		    (state == STATE_DISABLED_OFF) || (state == STATE_ERROR)) {
			break;
		}

		if (state == STATE_GRACE_PERIOD) {
			req_wakeup = true;
		}
		req_fast_adv = true;
		req_new_adv_session = true;

		/* Advertising must be stopped while getting the connection object. */
		int err = bt_le_adv_stop();

		if (err) {
			LOG_ERR("Cannot stop advertising (err %d)", err);
			update_state(STATE_ERROR);
			break;
		}

		struct bt_conn *conn = conn_get();

		if (conn) {
			/* State update will be done after complete disconnection. */
			disconnect_peer(conn);
		} else {
			update_state(STATE_ACTIVE);
		}

		if (event->op == PEER_OPERATION_ERASE_ADV) {
			update_peer_is_rpa(PEER_RPA_ERASED);
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
	switch (state) {
	case STATE_ACTIVE:
		if (IS_ENABLED(CONFIG_CAF_BLE_ADV_GRACE_PERIOD) && (req_grace_period_s > 0)) {
			update_state(STATE_GRACE_PERIOD);
			break;
		}
		/* Fall-through. */

	case STATE_DELAYED_ACTIVE:
	case STATE_IDLE:
		update_state(STATE_OFF);
		break;

	case STATE_DISABLED:
		update_state(STATE_DISABLED_OFF);
		break;

	case STATE_OFF:
	case STATE_GRACE_PERIOD:
	case STATE_DISABLED_OFF:
	case STATE_ERROR:
		/* No action */
		break;

	default:
		/* Should never happen */
		__ASSERT_NO_MSG(false);
		break;
	}

	return !is_module_state_off(state);
}

static bool handle_wake_up_event(const struct wake_up_event *event)
{
	switch (state) {
	case STATE_OFF:
	case STATE_GRACE_PERIOD:
		req_new_adv_session = true;
		req_fast_adv = true;
		update_state(STATE_ACTIVE);
		break;

	case STATE_DISABLED_OFF:
		update_state(STATE_DISABLED);
		break;

	case STATE_IDLE:
	case STATE_ACTIVE:
	case STATE_DELAYED_ACTIVE:
	case STATE_DISABLED:
	case STATE_ERROR:
		/* No action */
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
