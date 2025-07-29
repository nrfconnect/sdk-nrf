/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/random/random.h>

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_vs.h>

#include <bluetooth/adv_prov.h>

#include <bluetooth/services/fast_pair/adv_manager.h>
#include <bluetooth/services/fast_pair/fast_pair.h>
#include <bluetooth/services/fast_pair/fmdn.h>

#include "fp_adv_manager_use_case.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(fast_pair_adv_manager, CONFIG_BT_FAST_PAIR_LOG_LEVEL);

/* Fast Pair advertising interval 100ms. */
#define FP_ADV_INTERVAL (0x00A0)

/* Rotation period configuration for Fast Pair advertitising. */
#define FP_RPA_SEC_PER_MIN		(60U)
#define FP_RPA_TIMEOUT			(13 * FP_RPA_SEC_PER_MIN)
#define FP_RPA_TIMEOUT_OFFSET_MAX	(2 * FP_RPA_SEC_PER_MIN)

#define BITS_PER_VAR(_var)	(__CHAR_BIT__ * sizeof(_var))

/* Bitmask types for Fast Pair advertising requests. */
enum fp_adv_request_bm {
	FP_ADV_REQUEST_BM_ENABLED,
	FP_ADV_REQUEST_BM_PAIRING_MODE,
	FP_ADV_REQUEST_BM_SUSPEND_RPA,
	FP_ADV_REQUEST_BM_COUNT,
};

static bool is_initialized;
static bool is_enabled;

static bool fmdn_provisioned;

static struct bt_conn *fp_conn;
static struct bt_le_ext_adv *fp_adv_set;
static bool fp_adv_set_active;

/* Array to hold all bitmask variables */
static uint32_t fp_adv_request_bms[FP_ADV_REQUEST_BM_COUNT];

/* According to Fast Pair specification, the advertising interval should be no longer than
 * 100 ms when discoverable and no longer than 250 ms when not discoverable.
 */
static struct bt_le_adv_param fp_adv_param = {
	.id = BT_ID_DEFAULT,
	.options = BT_LE_ADV_OPT_CONN,
	.interval_min = FP_ADV_INTERVAL,
	.interval_max = FP_ADV_INTERVAL,
};

static bool fp_adv_manager_is_rpa_suspended(void);
static void fp_adv_restart_work_handle(struct k_work *w);
static void fp_adv_conn_err_work_handle(struct k_work *w);

static K_WORK_DEFINE(fp_adv_restart_work, fp_adv_restart_work_handle);
static K_WORK_DEFINE(fp_adv_conn_err_work, fp_adv_conn_err_work_handle);

/* Reference to the information callback list. */
static sys_slist_t fast_pair_adv_info_cb_slist =
	SYS_SLIST_STATIC_INIT(&fast_pair_adv_info_cb_slist);

/* Reference to the use case callback structure. */
static const struct fp_adv_manager_use_case_cb *use_case_cb;

static inline uint32_t fp_adv_request_bm_get(enum fp_adv_request_bm bm_type)
{
	__ASSERT_NO_MSG(bm_type < FP_ADV_REQUEST_BM_COUNT);
	return fp_adv_request_bms[bm_type];
}

static inline void fp_adv_request_bm_bit_set(enum fp_adv_request_bm bm_type, uint8_t idx,
					     bool enabled)
{
	__ASSERT_NO_MSG(bm_type < FP_ADV_REQUEST_BM_COUNT);
	WRITE_BIT(fp_adv_request_bms[bm_type], idx, enabled);
}

static inline bool fp_adv_request_bm_bit_get(enum fp_adv_request_bm bm_type, uint8_t idx)
{
	__ASSERT_NO_MSG(bm_type < FP_ADV_REQUEST_BM_COUNT);
	return (fp_adv_request_bms[bm_type] & BIT(idx)) != 0;
}

static void fp_adv_state_changed_notify(bool active)
{
	struct bt_fast_pair_adv_manager_info_cb *listener;

	SYS_SLIST_FOR_EACH_CONTAINER(&fast_pair_adv_info_cb_slist, listener, node) {
		if (listener->adv_state_changed) {
			listener->adv_state_changed(active);
		}
	}
}

int bt_fast_pair_adv_manager_info_cb_register(struct bt_fast_pair_adv_manager_info_cb *cb)
{
	/* It is assumed that this function executes in the cooperative thread context
	 * or in the system initialization context (SYS_INIT macro).
	 */
	__ASSERT_NO_MSG(!k_is_in_isr());

	if (bt_fast_pair_adv_manager_is_ready()) {
		return -EACCES;
	}

	if (!cb) {
		return -EINVAL;
	}

	if (sys_slist_find(&fast_pair_adv_info_cb_slist, &cb->node, NULL)) {
		return 0;
	}

	sys_slist_append(&fast_pair_adv_info_cb_slist, &cb->node);

	return 0;
}

void fp_adv_manager_use_case_cb_register(const struct fp_adv_manager_use_case_cb *cb)
{
	__ASSERT_NO_MSG(!bt_fast_pair_adv_manager_is_ready());
	__ASSERT_NO_MSG(cb);
	__ASSERT_NO_MSG(!use_case_cb);

	use_case_cb = cb;
}

static uint16_t fp_adv_rpa_timeout_calculate(void)
{
	int err;
	int8_t rand;
	uint16_t rpa_timeout = FP_RPA_TIMEOUT;

	err = sys_csrand_get(&rand, sizeof(rand));
	if (!err) {
		int16_t rand_timeout_diff;

		rand_timeout_diff = (((int)(FP_RPA_TIMEOUT_OFFSET_MAX * MSEC_PER_SEC)) / INT8_MAX);
		rand_timeout_diff *= rand;
		rand_timeout_diff /= ((int) MSEC_PER_SEC);

		rpa_timeout += rand_timeout_diff;
	} else {
		LOG_WRN("Fast Pair Adv Manager: cannot get random RPA timeout (err: %d). "
			"Used fixed value", err);
	}

	return rpa_timeout;
}

static int fp_adv_payload_set(bool rpa_rotated, bool new_session)
{
	int err;
	uint8_t adv_handle;
	struct bt_le_adv_prov_adv_state state = {0};
	struct bt_le_adv_prov_feedback fb = {0};
	size_t ad_len = bt_le_adv_prov_get_ad_prov_cnt();
	size_t sd_len = bt_le_adv_prov_get_sd_prov_cnt();
	struct bt_data ad[ad_len];
	struct bt_data sd[sd_len];

	__ASSERT(fp_adv_set, "Fast Pair Adv Manager: invalid state of the advertising set");

	err = bt_hci_get_adv_handle(fp_adv_set, &adv_handle);
	if (err) {
		LOG_ERR("Fast Pair Adv Manager: cannot get advertising handle (err: %d)", err);
		return err;
	}

	state.pairing_mode = bt_fast_pair_adv_manager_is_pairing_mode();
	state.rpa_rotated = rpa_rotated;
	state.new_adv_session = new_session;
	state.adv_handle = adv_handle;

	err = bt_le_adv_prov_get_ad(ad, &ad_len, &state, &fb);
	if (err) {
		LOG_ERR("Fast Pair Adv Manager: cannot get advertising data (err: %d)", err);
		return err;
	}

	err = bt_le_adv_prov_get_sd(sd, &sd_len, &state, &fb);
	if (err) {
		LOG_ERR("Fast Pair Adv Manager: cannot get scan response data (err: %d)", err);
		return err;
	}

	err = bt_le_ext_adv_set_data(fp_adv_set, ad, ad_len, sd, sd_len);
	if (err) {
		LOG_ERR("Fast Pair Adv Manager: bt_le_ext_adv_set_data returned error: %d", err);
		return err;
	}

	return 0;
}

static int bt_stack_advertising_start(bool force_start)
{
	int err;
	struct bt_le_ext_adv_start_param ext_adv_start_param = {0};

	__ASSERT(fp_adv_set, "Fast Pair Adv Manager: invalid state of the advertising set");

	if (fp_adv_set_active && !force_start) {
		return 0;
	}

	err = bt_le_ext_adv_start(fp_adv_set, &ext_adv_start_param);
	if (err) {
		if ((err == -EALREADY) && force_start) {
			LOG_WRN("Fast Pair Adv Manager: advertising already started");
		} else {
			LOG_ERR("Fast Pair Adv Manager: bt_le_ext_adv_start returned error: %d",
				err);
		}

		return err;
	}

	fp_adv_set_active = true;

	return 0;
}

static int bt_stack_advertising_stop(void)
{
	int err;

	__ASSERT(fp_adv_set, "Fast Pair Adv Manager: invalid state of the advertising set");

	if (!fp_adv_set_active) {
		return 0;
	}

	err = bt_le_ext_adv_stop(fp_adv_set);
	if (err) {
		LOG_ERR("Fast Pair Adv Manager: cannot stop advertising (err: %d)", err);
		return err;
	}

	fp_adv_set_active = false;

	return 0;
}

static int bt_stack_advertising_state_update(bool activate)
{
	int err;

	__ASSERT(!fp_conn, "Fast Pair Adv Manager: invalid connection state");

	if (activate) {
		err = fp_adv_payload_set(false, true);
		if (err) {
			LOG_ERR("Fast Pair Adv Manager: cannot set advertising payload (err: %d)",
				err);
			return err;
		}

		err = bt_stack_advertising_start(false);
		if (err) {
			return err;
		}
	} else {
		err = bt_stack_advertising_stop();
		if (err) {
			return err;
		}
	}

	return 0;
}

static void fp_advertising_state_update(bool activate)
{
	int err;
	bool fp_adv_set_was_active = fp_adv_set_active;

	/* Do not perform any advertising operation when the connection number is maxed out. */
	if (fp_conn) {
		return;
	}

	err = bt_stack_advertising_state_update(activate);
	if (!err) {
		if (activate) {
			LOG_INF("Fast Pair Adv Manager: advertising in the %s mode",
				bt_fast_pair_adv_manager_is_pairing_mode() ?
				"discoverable" : "not-discoverable");
		} else {
			LOG_INF("Fast Pair Adv Manager: advertising is disabled");
		}
	} else {
		LOG_ERR("Fast Pair Adv Manager: advertising failed to start (err %d)", err);
	}

	if (fp_adv_set_was_active != fp_adv_set_active) {
		fp_adv_state_changed_notify(fp_adv_set_active);
	}
}

static void fp_adv_connected(struct bt_le_ext_adv *adv, struct bt_le_ext_adv_connected_info *info)
{
	char addr[BT_ADDR_LE_STR_LEN];

	__ASSERT_NO_MSG(!fp_conn);
	__ASSERT_NO_MSG(fp_adv_set);

	bt_addr_le_to_str(bt_conn_get_dst(info->conn), addr, sizeof(addr));
	LOG_INF("Fast Pair Adv Manager: connected to %s", addr);

	fp_adv_set_active = false;
	fp_adv_state_changed_notify(fp_adv_set_active);

	fp_conn = info->conn;
}

static bool fp_adv_rpa_expired(struct bt_le_ext_adv *adv)
{
	int err;
	uint16_t next_rpa_timeout;
	static int64_t uptime;
	bool expire_rpa = true;

	/* It is assumed that the callback executes in the cooperative
	 * thread context as it interacts with the FMDN operations.
	 */
	__ASSERT_NO_MSG(!k_is_preempt_thread());
	__ASSERT_NO_MSG(!k_is_in_isr());

	__ASSERT_NO_MSG(fp_adv_set);

	LOG_INF("Fast Pair Adv Manager: RPA expired");

	if (!fp_adv_set_active) {
		LOG_INF("Fast Pair Adv Manager: RPA rotation in the inactive advertising state");
	}

	if (!uptime) {
		uptime = k_uptime_get();
	} else {
		LOG_INF("Fast Pair Adv Manager: the last timeout has occurred %" PRId64 " [s] ago",
			(k_uptime_delta(&uptime) / MSEC_PER_SEC));
	}

	/* In the FMDN provisioned state, the FMDN advertising set is responsible
	 * for setting the global RPA timeout using the bt_le_set_rpa_timeout API.
	 * In the unprovisioned state, it is the responsibility of the Fast Pair advertising set.
	 */
	if (!fmdn_provisioned) {
		next_rpa_timeout = fp_adv_rpa_timeout_calculate();
		err = bt_le_set_rpa_timeout(next_rpa_timeout);
		if (err && (err != -EALREADY)) {
			LOG_ERR("Fast Pair Adv Manager: "
				"bt_le_set_rpa_timeout failed: %d for %d [s]",
				err, next_rpa_timeout);
		} else {
			LOG_INF("Fast Pair Adv Manager: setting RPA timeout to %d [s]",
				next_rpa_timeout);
		}
	} else {
		if (!fp_adv_set_active) {
			/* Keep the RPA in the valid state to ensure that the RPA expired callback
			 * will be received on a forced RPA rotation during the FMDN unprovisioning
			 * operation. The forced RPA rotation allows the Fast Pair advertising set
			 * to take control from the FMDN advertising set over the RPA timeout. The
			 * RPA expired callback will not be received if any RPA rotation was
			 * previously allowed here in the provisioned state and with inactive Fast
			 * Pair advertising set.
			 */
			expire_rpa = false;
		}
	}

	if (fp_adv_manager_is_rpa_suspended()) {
		LOG_INF("Fast Pair Adv Manager: RPA rotation is in the suspended mode");

		expire_rpa = false;
	}

	if (bt_fast_pair_adv_manager_is_adv_active()) {
		/* Update the advertising payload. */
		err = fp_adv_payload_set(expire_rpa, false);
		if (err) {
			LOG_ERR("Fast Pair Adv Manager: "
				"cannot set advertising payload (err: %d)", err);
		}
	}

	return expire_rpa;
}

static int fp_adv_set_rotate(void)
{
	int err;
	struct bt_le_oob oob;

	__ASSERT(fp_adv_set, "Fast Pair Adv Manager: invalid state of the advertising set");

	/* Force the RPA rotation to synchronize the Fast Pair advertising
	 * payload with its RPA address using rpa_expired callback.
	 */
	err = bt_le_oob_get_local(fp_adv_param.id, &oob);
	if (err) {
		LOG_ERR("Fast Pair Adv Manager: bt_le_oob_get_local failed: %d", err);
		return err;
	}

	return 0;
}

static int fp_adv_set_setup(void)
{
	int err;
	static const struct bt_le_ext_adv_cb fp_adv_set_cb = {
		.connected = fp_adv_connected,
		.rpa_expired = fp_adv_rpa_expired,
	};

	__ASSERT(!fp_adv_set,
		 "Fast Pair Adv Manager: invalid state of the advertising set");

	/* Create the Fast Pair advertising set. */
	err = bt_le_ext_adv_create(&fp_adv_param, &fp_adv_set_cb, &fp_adv_set);
	if (err) {
		LOG_ERR("Fast Pair Adv Manager: bt_le_ext_adv_create returned error: %d", err);
		return err;
	}

	LOG_INF("Fast Pair Adv Manager: prepared the advertising set");

	return 0;
}

static int fp_adv_set_teardown(void)
{
	int err;

	__ASSERT(fp_adv_set, "Fast Pair Adv Manager: invalid state of the advertising set");

	err = bt_le_ext_adv_delete(fp_adv_set);
	if (err) {
		LOG_ERR("Fast Pair Adv Manager: bt_le_ext_adv_delete returned error: %d", err);
		return err;
	}

	fp_adv_set = NULL;

	return 0;
}

static bool fp_adv_is_any_request_enabled(void)
{
	return (fp_adv_request_bm_get(FP_ADV_REQUEST_BM_ENABLED) != 0);
}

static void fp_adv_restart_work_handle(struct k_work *w)
{
	if (bt_fast_pair_adv_manager_is_ready() && fp_adv_is_any_request_enabled()) {
		fp_advertising_state_update(true);
	}
}

static void fp_adv_conn_err_work_handle(struct k_work *w)
{
	if (bt_fast_pair_adv_manager_is_ready() && fp_adv_set_active) {
		bt_stack_advertising_start(true);
	}
}

static void fp_adv_conn_clear(void)
{
	int err;
	char addr[BT_ADDR_LE_STR_LEN];

	if (fp_conn) {
		err = bt_conn_disconnect(fp_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		if (err) {
			LOG_ERR("Fast Pair Adv Manager: bt_conn_disconnect returned error: %d",
				err);
			return;
		}

		bt_addr_le_to_str(bt_conn_get_dst(fp_conn), addr, sizeof(addr));
		LOG_INF("Fast Pair Adv Manager: disconnecting peer: %s", addr);
	}
}

static bool is_fp_adv_conn(struct bt_conn *conn)
{
	return (fp_conn == conn);
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		LOG_WRN("Fast Pair Adv Manager: connection possibly failed (err %" PRIu8 ")", err);

		/* Optimistically try to restart the advertising since we do not know
		 * at this point if the connection is established from the Fast Pair
		 * advertising set. In case of an error, the connected callback for
		 * the Fast Pair advertising set is not called.
		 */
		(void) k_work_submit(&fp_adv_conn_err_work);
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (!is_fp_adv_conn(conn)) {
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_INF("Fast Pair Adv Manager: disconnected from %s (reason %" PRIu8 ")", addr, reason);

	fp_conn = NULL;
	(void) k_work_submit(&fp_adv_restart_work);
}

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (!is_fp_adv_conn(conn)) {
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		LOG_INF("Fast Pair Adv Manager: security changed: %s level %u", addr, level);
	} else {
		LOG_WRN("Fast Pair Adv Manager: security failed: %s level %u err %d %s",
			addr, level, err, bt_security_err_to_str(err));
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected        = connected,
	.disconnected     = disconnected,
	.security_changed = security_changed,
};

static void fp_adv_provisioning_state_changed(bool provisioned)
{
	fmdn_provisioned = provisioned;

	if (!bt_fast_pair_adv_manager_is_ready()) {
		return;
	}

	if (!provisioned) {
		(void) fp_adv_set_rotate();
	}
}

static struct bt_fast_pair_fmdn_info_cb fmdn_info_cb = {
	.provisioning_state_changed = fp_adv_provisioning_state_changed,
};

bool bt_fast_pair_adv_manager_is_pairing_mode(void)
{
	return (fp_adv_request_bm_get(FP_ADV_REQUEST_BM_PAIRING_MODE) != 0);
}

static bool fp_adv_request_bm_pairing_mode_set(uint8_t idx, bool enabled)
{
	bool pairing_mode_to_be_activated;
	bool pairing_mode_active = bt_fast_pair_adv_manager_is_pairing_mode();

	fp_adv_request_bm_bit_set(FP_ADV_REQUEST_BM_PAIRING_MODE, idx, enabled);

	pairing_mode_to_be_activated = bt_fast_pair_adv_manager_is_pairing_mode();
	if (pairing_mode_to_be_activated != pairing_mode_active) {
		LOG_DBG("Fast Pair Adv Manager: set pairing mode to %s",
			pairing_mode_to_be_activated ? "true" : "false");

		return true;
	}

	return false;
}

static bool fp_adv_manager_is_rpa_suspended(void)
{
	return (fp_adv_request_bm_get(FP_ADV_REQUEST_BM_SUSPEND_RPA) != 0);
}

static void fp_adv_request_bm_suspend_rpa_set(uint8_t idx, bool enabled)
{
	bool rpa_to_be_suspended;
	bool rpa_suspended = fp_adv_manager_is_rpa_suspended();

	fp_adv_request_bm_bit_set(FP_ADV_REQUEST_BM_SUSPEND_RPA, idx, enabled);

	rpa_to_be_suspended = fp_adv_manager_is_rpa_suspended();
	if (rpa_to_be_suspended != rpa_suspended) {
		LOG_DBG("Fast Pair Adv Manager: set RPA rotation suspended to %s",
			rpa_to_be_suspended ? "true" : "false");
	}
}

static uint8_t fp_adv_trigger_idx_get(struct bt_fast_pair_adv_manager_trigger *trigger)
{
	__ASSERT_NO_MSG(trigger);

	STRUCT_SECTION_START_EXTERN(bt_fast_pair_adv_manager_trigger);
	STRUCT_SECTION_END_EXTERN(bt_fast_pair_adv_manager_trigger);

	__ASSERT_NO_MSG(trigger >= STRUCT_SECTION_START(bt_fast_pair_adv_manager_trigger));
	__ASSERT_NO_MSG(trigger < STRUCT_SECTION_END(bt_fast_pair_adv_manager_trigger));
	ARG_UNUSED(STRUCT_SECTION_END(bt_fast_pair_adv_manager_trigger));

	return (trigger - STRUCT_SECTION_START(bt_fast_pair_adv_manager_trigger));
}

void bt_fast_pair_adv_manager_request(
	struct bt_fast_pair_adv_manager_trigger *trigger, bool enable)
{
	bool adv_to_be_triggered;
	bool adv_triggered = fp_adv_is_any_request_enabled();
	uint8_t idx = fp_adv_trigger_idx_get(trigger);
	bool trigger_enabled = fp_adv_request_bm_bit_get(FP_ADV_REQUEST_BM_ENABLED, idx);
	bool force_update = false;

	if (trigger_enabled == enable) {
		return;
	}

	fp_adv_request_bm_bit_set(FP_ADV_REQUEST_BM_ENABLED, idx, enable);

	LOG_INF("Fast Pair Adv Manager: advertising request from trigger \"%s\": %sable",
		trigger->id, enable ? "en" : "dis");

	if (trigger->config) {
		if (trigger->config->pairing_mode) {
			force_update = fp_adv_request_bm_pairing_mode_set(idx, enable);
		}

		if (trigger->config->suspend_rpa) {
			fp_adv_request_bm_suspend_rpa_set(idx, enable);
		}
	}

	if (bt_fast_pair_adv_manager_is_ready()) {
		bool update_adv_state = false;

		adv_to_be_triggered = fp_adv_is_any_request_enabled();
		if (adv_triggered != adv_to_be_triggered) {
			LOG_INF("Fast Pair Adv Manager: triggers state change to: %s",
				adv_to_be_triggered ? "active" : "inactive");

			update_adv_state = true;
		}

		if (enable && force_update) {
			update_adv_state = true;
		}

		if (update_adv_state) {
			fp_advertising_state_update(adv_to_be_triggered);
		}
	}
}

void bt_fast_pair_adv_manager_payload_refresh(void)
{
	int err;

	if (!bt_fast_pair_adv_manager_is_adv_active()) {
		return;
	}

	err = fp_adv_payload_set(false, true);
	if (err) {
		LOG_ERR("Fast Pair Adv Manager: cannot refresh the advertising payload (err: %d)",
			err);
		return;
	}
}

int bt_fast_pair_adv_manager_id_set(uint8_t id)
{
	if (bt_fast_pair_adv_manager_is_ready()) {
		/* It is not possible to switch the Bluetooth identity
		 * if Fast Pair Advertising Manager module is operational.
		 */
		return -EACCES;
	}

	fp_adv_param.id = id;

	return 0;
}

uint8_t bt_fast_pair_adv_manager_id_get(void)
{
	return fp_adv_param.id;
}

bool bt_fast_pair_adv_manager_is_adv_active(void)
{
	return fp_adv_set_active;
}

int bt_fast_pair_adv_manager_enable(void)
{
	int err;

	__ASSERT_NO_MSG(bt_fast_pair_is_ready());
	__ASSERT_NO_MSG(is_initialized);

	if (is_enabled) {
		LOG_ERR("Fast Pair Adv Manager: module already enabled");
		return 0;
	}

	if (IS_ENABLED(CONFIG_BT_FAST_PAIR_FMDN)) {
		fmdn_provisioned = bt_fast_pair_fmdn_is_provisioned();
	}

	if (!fp_adv_set) {
		err = fp_adv_set_setup();
		if (err) {
			LOG_ERR("Fast Pair Adv Manager: fp_adv_set_setup failed (err %d)", err);
			return err;
		}
	}

	if (!fmdn_provisioned) {
		err = fp_adv_set_rotate();
		if (err) {
			LOG_ERR("Fast Pair: fp_adv_set_rotate failed: %d", err);
			(void) fp_adv_set_teardown();
			return err;
		}
	}

	if (use_case_cb && use_case_cb->enable) {
		err = use_case_cb->enable();
		if (err) {
			LOG_ERR("Fast Pair Adv Manager: use case enable failed (err %d)", err);
			return err;
		}
	}

	if (fp_adv_is_any_request_enabled()) {
		fp_advertising_state_update(true);
	}

	is_enabled = true;

	LOG_INF("Fast Pair Adv Manager: module enabled");

	return 0;
}

int bt_fast_pair_adv_manager_disable(void)
{
	int err;

	__ASSERT_NO_MSG(is_initialized);

	is_enabled = false;

	if (use_case_cb && use_case_cb->disable) {
		err = use_case_cb->disable();
		if (err) {
			LOG_ERR("Fast Pair Adv Manager: use case disable failed (err %d)", err);
			return err;
		}
	}

	/* Suspend the Bluetooth activities until this module is reenabled. */
	if (bt_fast_pair_adv_manager_is_adv_active()) {
		fp_advertising_state_update(false);
	}
	fp_adv_conn_clear();

	if (fp_adv_set) {
		err = fp_adv_set_teardown();
		if (err) {
			LOG_ERR("Fast Pair Adv Manager: fp_adv_set_teardown failed (err %d)", err);
			return err;
		}
	}

	LOG_INF("Fast Pair Adv Manager: module disabled");

	return 0;
}

bool bt_fast_pair_adv_manager_is_ready(void)
{
	return is_enabled;
}

int fp_adv_manager_init(void)
{
	int trigger_cnt;

	if (IS_ENABLED(CONFIG_BT_FAST_PAIR_FMDN)) {
		int err;

		err = bt_fast_pair_fmdn_info_cb_register(&fmdn_info_cb);
		if (err) {
			LOG_ERR("Fast Pair Adv Manager: "
				"bt_fast_pair_fmdn_info_cb_register returned error: %d", err);
			return err;
		}
	}

	STRUCT_SECTION_COUNT(bt_fast_pair_adv_manager_trigger, &trigger_cnt);
	__ASSERT_NO_MSG(trigger_cnt <= BITS_PER_VAR(fp_adv_request_bms[0]));

	is_initialized = true;

	return 0;
}

SYS_INIT(fp_adv_manager_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
