/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/random/random.h>

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_vs.h>

#include <bluetooth/adv_prov.h>
#include <bluetooth/adv_prov/fast_pair.h>
#include <bluetooth/services/fast_pair/fast_pair.h>
#include <bluetooth/services/fast_pair/fmdn.h>

#include "app_fp_adv.h"
#include "app_ui.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(fp_fmdn, LOG_LEVEL_INF);

/* Fast Pair advertising interval 100ms. */
#define FP_ADV_INTERVAL (0x00A0)

/* Rotation period configuration for Fast Pair advertitising. */
#define FP_RPA_SEC_PER_MIN		(60U)
#define FP_RPA_TIMEOUT			(13 * FP_RPA_SEC_PER_MIN)
#define FP_RPA_TIMEOUT_OFFSET_MAX	(2 * FP_RPA_SEC_PER_MIN)

#define BITS_PER_VAR(_var)	(__CHAR_BIT__ * sizeof(_var))

static bool is_initialized;
static bool is_enabled;

static bool fmdn_provisioned;

static struct bt_conn *fp_conn;
static struct bt_le_ext_adv *fp_adv_set;
static bool fp_adv_rpa_rotation_suspended;
static enum app_fp_adv_mode fp_adv_mode = APP_FP_ADV_MODE_OFF;
static uint32_t fp_adv_request_bm;
static const char * const fp_adv_mode_description[] = {
	[APP_FP_ADV_MODE_OFF] = "disabled",
	[APP_FP_ADV_MODE_DISCOVERABLE] = "discoverable",
	[APP_FP_ADV_MODE_NOT_DISCOVERABLE] = "non-discoverable",
};

/* According to Fast Pair specification, the advertising interval should be no longer than
 * 100 ms when discoverable and no longer than 250 ms when not discoverable.
 */
static struct bt_le_adv_param fp_adv_param = {
	.id = BT_ID_DEFAULT,
	.options = (BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_ONE_TIME),
	.interval_min = FP_ADV_INTERVAL,
	.interval_max = FP_ADV_INTERVAL,
};

static void fp_adv_restart_work_handle(struct k_work *w);

static K_WORK_DEFINE(fp_adv_restart_work, fp_adv_restart_work_handle);

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
		LOG_WRN("Fast Pair: cannot get random RPA timeout (err: %d). Used fixed value",
			err);
	}

	return rpa_timeout;
}

static void bond_cnt_cb(const struct bt_bond_info *info, void *user_data)
{
	size_t *cnt = user_data;

	(*cnt)++;
}

static size_t bond_cnt(void)
{
	size_t cnt = 0;

	bt_foreach_bond(fp_adv_param.id, bond_cnt_cb, &cnt);

	return cnt;
}

static bool can_pair(void)
{
	return (bond_cnt() < CONFIG_BT_MAX_PAIRED);
}

static bool fp_adv_is_pairing_mode(enum app_fp_adv_mode fp_adv_mode)
{
	return ((fp_adv_mode == APP_FP_ADV_MODE_DISCOVERABLE) && can_pair());
}

static void fp_adv_prov_configure(enum app_fp_adv_mode fp_adv_mode)
{
	bt_le_adv_prov_fast_pair_enable(can_pair());

	switch (fp_adv_mode) {
	case APP_FP_ADV_MODE_DISCOVERABLE:
		break;

	case APP_FP_ADV_MODE_NOT_DISCOVERABLE:
		bt_le_adv_prov_fast_pair_enable(true);
		break;

	default:
		/* Should not happen. */
		__ASSERT_NO_MSG(false);
		break;
	};
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

	/* Set advertising mode of Fast Pair advertising data provider. */
	fp_adv_prov_configure(fp_adv_mode);

	err = bt_hci_get_adv_handle(fp_adv_set, &adv_handle);
	if (err) {
		LOG_ERR("Fast Pair: cannot get advertising handle (err: %d)", err);
		return err;
	}

	state.pairing_mode = fp_adv_is_pairing_mode(fp_adv_mode);
	state.rpa_rotated = rpa_rotated;
	state.new_adv_session = new_session;
	state.adv_handle = adv_handle;

	err = bt_le_adv_prov_get_ad(ad, &ad_len, &state, &fb);
	if (err) {
		LOG_ERR("Fast Pair: cannot get advertising data (err: %d)", err);
		return err;
	}

	err = bt_le_adv_prov_get_sd(sd, &sd_len, &state, &fb);
	if (err) {
		LOG_ERR("Fast Pair: cannot get scan response data (err: %d)", err);
		return err;
	}

	err = bt_le_ext_adv_set_data(fp_adv_set, ad, ad_len, sd, sd_len);
	if (err) {
		LOG_ERR("Fast Pair: bt_le_ext_adv_set_data returned error: %d", err);
		return err;
	}

	return 0;
}

static int bt_stack_advertising_update(void)
{
	int err;
	struct bt_le_ext_adv_start_param ext_adv_start_param = {0};

	err = bt_le_ext_adv_stop(fp_adv_set);
	if (err) {
		LOG_ERR("Fast Pair: cannot stop advertising (err: %d)", err);
		return err;
	}

	if (fp_adv_mode == APP_FP_ADV_MODE_OFF) {
		return 0;
	}

	err = fp_adv_payload_set(false, true);
	if (err) {
		LOG_ERR("Fast Pair: cannot set advertising payload (err: %d)", err);
		return err;
	}

	err = bt_le_ext_adv_start(fp_adv_set, &ext_adv_start_param);
	if (err) {
		LOG_ERR("Fast Pair: bt_le_ext_adv_start returned error: %d", err);
		return err;
	}

	return 0;
}

static void fp_advertising_update(void)
{
	int err;

	if (!fp_adv_set) {
		/* Advertising set is not prepared yet. */
		return;
	}

	if (fp_conn) {
		/* Only one Fast Pair connection is supported. */
		return;
	}

	err = bt_stack_advertising_update();
	if (!err) {
		app_ui_state_change_indicate(APP_UI_STATE_FP_ADV,
					     (fp_adv_mode != APP_FP_ADV_MODE_OFF));

		LOG_INF("Fast Pair: advertising in the %s mode",
			fp_adv_mode_description[fp_adv_mode]);
	} else {
		app_ui_state_change_indicate(APP_UI_STATE_FP_ADV, false);

		LOG_ERR("Fast Pair: advertising failed to start (err %d)", err);
	}
}

static void fp_adv_connected(struct bt_le_ext_adv *adv, struct bt_le_ext_adv_connected_info *info)
{
	__ASSERT_NO_MSG(!fp_conn);

	app_ui_state_change_indicate(APP_UI_STATE_FP_ADV, false);

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

	__ASSERT_NO_MSG(is_enabled);

	LOG_INF("Fast Pair: RPA expired");

	if (!uptime) {
		uptime = k_uptime_get();
	} else {
		LOG_INF("Fast Pair: the last timeout has occurred %" PRId64 " [s] ago",
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
			LOG_ERR("Fast Pair: bt_le_set_rpa_timeout failed: %d for %d [s]",
				err, next_rpa_timeout);
		} else {
			LOG_INF("Fast Pair: setting RPA timeout to %d [s]",
				next_rpa_timeout);
		}
	}

	if (fp_adv_mode == APP_FP_ADV_MODE_DISCOVERABLE) {
		LOG_INF("Fast Pair: RPA rotation blocked for the discoverable advertising");

		expire_rpa = false;
	}

	if (fp_adv_rpa_rotation_suspended) {
		LOG_INF("Fast Pair: RPA rotation is in the suspended mode");

		expire_rpa = false;
	}

	if (fp_adv_mode != APP_FP_ADV_MODE_OFF) {
		/* Update the advertising payload. */
		err = fp_adv_payload_set(expire_rpa, false);
		if (err) {
			LOG_ERR("Fast Pair: cannot set advertising payload (err: %d)", err);
		}
	}

	return expire_rpa;
}

static int fp_adv_set_setup(void)
{
	int err;
	static const struct bt_le_ext_adv_cb fp_adv_set_cb = {
		.connected = fp_adv_connected,
		.rpa_expired = fp_adv_rpa_expired,
	};

	__ASSERT(!fp_adv_set, "Fast Pair: invalid state of the advertising set");

	/* Create the Fast Pair advertising set. */
	err = bt_le_ext_adv_create(&fp_adv_param, &fp_adv_set_cb, &fp_adv_set);
	if (err) {
		LOG_ERR("Fast Pair: bt_le_ext_adv_create returned error: %d", err);
		return err;
	}

	LOG_INF("Fast Pair: prepared the advertising set");

	return 0;
}

static int fp_adv_set_teardown(void)
{
	int err;

	__ASSERT(fp_adv_set, "Fast Pair: invalid state of the advertising set");

	err = bt_le_ext_adv_delete(fp_adv_set);
	if (err) {
		LOG_ERR("Fast Pair: bt_le_ext_adv_delete returned error: %d", err);
		return err;
	}

	fp_adv_set = NULL;

	return 0;
}

static void fp_adv_restart_work_handle(struct k_work *w)
{
	fp_advertising_update();
}

static void fp_adv_conn_clear(void)
{
	int err;

	if (fp_conn) {
		err = bt_conn_disconnect(fp_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		if (err) {
			LOG_ERR("Fast Pair: bt_conn_disconnect returned error: %d", err);
		}
	}
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		LOG_WRN("Connection failed (err %" PRIu8 ")", err);

		(void) k_work_submit(&fp_adv_restart_work);
	} else {
		LOG_INF("Connected");
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Disconnected (reason %" PRIu8 ")", reason);

	if (fp_conn == conn) {
		fp_conn = NULL;
		(void) k_work_submit(&fp_adv_restart_work);
	}
}

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		LOG_INF("Security changed: %s level %u", addr, level);
	} else {
		LOG_WRN("Security failed: %s level %u err %d %s", addr, level, err,
			bt_security_err_to_str(err));
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected        = connected,
	.disconnected     = disconnected,
	.security_changed = security_changed,
};

static void fp_adv_provisioning_state_changed(bool provisioned)
{
	struct bt_le_oob oob;
	int err;

	fmdn_provisioned = provisioned;

	if (!provisioned) {
		/* Force the RPA rotation to synchronize the Fast Pair advertising
		 * payload with its RPA address using rpa_expired callback.
		 */
		err = bt_le_oob_get_local(fp_adv_param.id, &oob);
		if (err) {
			LOG_ERR("Fast Pair: bt_le_oob_get_local failed: %d", err);
		}
	}
}

static struct bt_fast_pair_fmdn_info_cb fmdn_info_cb = {
	.provisioning_state_changed = fp_adv_provisioning_state_changed,
};

enum app_fp_adv_mode app_fp_adv_mode_get(void)
{
	return fp_adv_mode;
}

static uint8_t fp_adv_trigger_idx_get(struct app_fp_adv_trigger *trigger)
{
	__ASSERT_NO_MSG(trigger);

	STRUCT_SECTION_START_EXTERN(app_fp_adv_trigger);
	STRUCT_SECTION_END_EXTERN(app_fp_adv_trigger);

	__ASSERT_NO_MSG((trigger >= STRUCT_SECTION_START(app_fp_adv_trigger)) &&
			(trigger < STRUCT_SECTION_END(app_fp_adv_trigger)));
	ARG_UNUSED(STRUCT_SECTION_END(app_fp_adv_trigger));

	return (trigger - STRUCT_SECTION_START(app_fp_adv_trigger));
}

static void fp_adv_mode_set(enum app_fp_adv_mode adv_mode)
{
	fp_adv_mode = adv_mode;

	if (!fp_conn) {
		/* Support only one connection for the Fast Pair advertising set. */
		fp_advertising_update();
	} else {
		LOG_INF("Fast Pair: automatically switched to %s advertising mode",
			fp_adv_mode_description[fp_adv_mode]);
		LOG_INF("Fast Pair: advertising inactive due to an active connection");
	}
}

static void fp_adv_mode_update(void)
{
	bool has_account_key = bt_fast_pair_has_account_key();
	bool is_any_request_enabled = (fp_adv_request_bm != 0);
	enum app_fp_adv_mode requested_mode;

	if (is_any_request_enabled) {
		requested_mode = has_account_key ?
				 APP_FP_ADV_MODE_NOT_DISCOVERABLE :
				 APP_FP_ADV_MODE_DISCOVERABLE;
	} else {
		requested_mode = APP_FP_ADV_MODE_OFF;
	}

	/* Switch the advertising mode only on change. */
	if (requested_mode != fp_adv_mode) {
		LOG_INF("Fast Pair: advertising: %sabling", is_any_request_enabled ? "en" : "dis");
		fp_adv_mode_set(requested_mode);
	}
}

void app_fp_adv_request(struct app_fp_adv_trigger *trigger, bool enable)
{
	uint8_t idx = fp_adv_trigger_idx_get(trigger);
	bool trigger_enabled = fp_adv_request_bm & BIT(idx);

	if (trigger_enabled == enable) {
		return;
	}

	WRITE_BIT(fp_adv_request_bm, idx, enable);

	LOG_INF("Fast Pair: advertising request from trigger \"%s\": %sable",
		trigger->id, enable ? "en" : "dis");

	if (is_enabled) {
		fp_adv_mode_update();
	}
}

void app_fp_adv_payload_refresh(void)
{
	if (fp_adv_mode != APP_FP_ADV_MODE_OFF) {
		fp_adv_mode_set(fp_adv_mode);
	}
}

void app_fp_adv_rpa_rotation_suspend(bool suspended)
{
	fp_adv_rpa_rotation_suspended = suspended;
}

int app_fp_adv_id_set(uint8_t id)
{
	if (app_fp_adv_is_ready()) {
		/* It is not possible to switch the Bluetooth identity
		 * if Fast Pair advertising module is operational.
		 */
		return -EACCES;
	}

	fp_adv_param.id = id;

	return 0;
}

uint8_t app_fp_adv_id_get(void)
{
	return fp_adv_param.id;
}

int app_fp_adv_init(void)
{
	int err;
	int trigger_cnt;

	err = bt_fast_pair_fmdn_info_cb_register(&fmdn_info_cb);
	if (err) {
		LOG_ERR("Fast Pair: bt_fast_pair_fmdn_info_cb_register returned error: %d", err);
		return err;
	}

	STRUCT_SECTION_COUNT(app_fp_adv_trigger, &trigger_cnt);
	__ASSERT_NO_MSG(trigger_cnt <= BITS_PER_VAR(fp_adv_request_bm));

	is_initialized = true;

	return 0;
}

int app_fp_adv_enable(void)
{
	int err;

	__ASSERT_NO_MSG(bt_fast_pair_is_ready());
	__ASSERT_NO_MSG(is_initialized);

	if (is_enabled) {
		LOG_ERR("Fast Pair: fp_adv module already enabled");
		return 0;
	}

	err = fp_adv_set_setup();
	if (err) {
		LOG_ERR("Fast Pair: fp_adv_set_setup failed (err %d)", err);
		return err;
	}

	fp_adv_mode_update();

	is_enabled = true;

	LOG_INF("Fast Pair: fp_adv module enabled");

	return 0;
}

int app_fp_adv_disable(void)
{
	int err;

	__ASSERT_NO_MSG(is_initialized);

	if (!is_enabled) {
		LOG_ERR("Fast Pair: fp_adv module already disabled");
		return 0;
	}

	is_enabled = false;

	/* Suspend the requested advertising until the fp_adv module reinitializes. */
	fp_adv_mode_set(APP_FP_ADV_MODE_OFF);

	fp_adv_conn_clear();

	err = fp_adv_set_teardown();
	if (err) {
		LOG_ERR("Fast Pair: fp_adv_set_teardown failed (err %d)", err);
		return err;
	}

	LOG_INF("Fast Pair: fp_adv module disabled");

	return 0;
}

bool app_fp_adv_is_ready(void)
{
	return is_enabled;
}
