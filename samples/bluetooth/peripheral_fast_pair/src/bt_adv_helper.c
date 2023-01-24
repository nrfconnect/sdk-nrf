/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/random/rand32.h>
#include <zephyr/sys/__assert.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <bluetooth/adv_prov.h>
#include <bluetooth/adv_prov/fast_pair.h>
#include <bluetooth/services/fast_pair.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(fp_sample, LOG_LEVEL_INF);

#include "bt_adv_helper.h"

#define SEC_PER_MIN		60U

/* According to Fast Pair specification RPA rotation must be synchronized with generating new salt
 * for Acount Key Filter advertising data. The RPA rotation must occur at least every 15 minutes
 * while the device is actively advertising in Fast Pair not discoverable mode. The value of this
 * timeout must be lower than CONFIG_BT_RPA_TIMEOUT to ensure that RPA rotation will always trigger
 * update of Account Key Filter advertising data.
 */
#define RPA_TIMEOUT_NON_DISCOVERABLE	(13 * SEC_PER_MIN)
#define RPA_TIMEOUT_OFFSET_MAX		(2 * SEC_PER_MIN)
#define RPA_TIMEOUT_FAST_PAIR_MAX	(15 * SEC_PER_MIN)

BUILD_ASSERT(RPA_TIMEOUT_FAST_PAIR_MAX < CONFIG_BT_RPA_TIMEOUT);

static enum bt_fast_pair_adv_mode adv_helper_fp_adv_mode;
static bool adv_helper_new_adv_session;

static void rpa_rotate_fn(struct k_work *w);
static K_WORK_DELAYABLE_DEFINE(rpa_rotate, rpa_rotate_fn);


static void connected(struct bt_conn *conn, uint8_t err)
{
	if (!err) {
		int ret = k_work_cancel_delayable(&rpa_rotate);

		__ASSERT_NO_MSG(ret == 0);
		ARG_UNUSED(ret);
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected        = connected,
};

static void bond_cnt_cb(const struct bt_bond_info *info, void *user_data)
{
	size_t *cnt = user_data;

	(*cnt)++;
}

static size_t bond_cnt(void)
{
	size_t cnt = 0;

	bt_foreach_bond(BT_ID_DEFAULT, bond_cnt_cb, &cnt);

	return cnt;
}

static bool can_pair(void)
{
	return (bond_cnt() < CONFIG_BT_MAX_PAIRED);
}

static bool pairing_mode(enum bt_fast_pair_adv_mode fp_adv_mode)
{
	return ((fp_adv_mode == BT_FAST_PAIR_ADV_MODE_DISCOVERABLE) && can_pair());
}

static void configure_fp_adv_prov(enum bt_fast_pair_adv_mode fp_adv_mode)
{
	bt_le_adv_prov_fast_pair_enable(can_pair());

	switch (fp_adv_mode) {
	case BT_FAST_PAIR_ADV_MODE_DISCOVERABLE:
		break;

	case BT_FAST_PAIR_ADV_MODE_NOT_DISCOVERABLE_SHOW_UI_IND:
		bt_le_adv_prov_fast_pair_show_ui_pairing(true);
		break;

	case BT_FAST_PAIR_ADV_MODE_NOT_DISCOVERABLE_HIDE_UI_IND:
		bt_le_adv_prov_fast_pair_enable(true);
		bt_le_adv_prov_fast_pair_show_ui_pairing(false);
		break;

	default:
		/* Should not happen. */
		__ASSERT_NO_MSG(false);
		break;
	};
}

static int adv_start_internal(enum bt_fast_pair_adv_mode fp_adv_mode)
{
	struct bt_le_oob oob;
	int err = bt_le_adv_stop();

	if (err) {
		LOG_ERR("Cannot stop advertising (err: %d)", err);
		return err;
	}

	/* Generate new Resolvable Private Address (RPA). */
	err = bt_le_oob_get_local(BT_ID_DEFAULT, &oob);
	if (err) {
		LOG_ERR("Cannot trigger RPA rotation (err: %d)", err);
		return err;
	}

	configure_fp_adv_prov(fp_adv_mode);

	size_t ad_len = bt_le_adv_prov_get_ad_prov_cnt();
	size_t sd_len = bt_le_adv_prov_get_sd_prov_cnt();
	struct bt_data ad[ad_len];
	struct bt_data sd[sd_len];

	struct bt_le_adv_prov_adv_state state;
	struct bt_le_adv_prov_feedback fb;

	state.pairing_mode = pairing_mode(fp_adv_mode);
	state.in_grace_period = false;
	state.rpa_rotated = true;
	state.new_adv_session = adv_helper_new_adv_session;

	adv_helper_new_adv_session = false;

	err = bt_le_adv_prov_get_ad(ad, &ad_len, &state, &fb);
	if (err) {
		LOG_ERR("Cannot get advertising data (err: %d)", err);
		return err;
	}

	err = bt_le_adv_prov_get_sd(sd, &sd_len, &state, &fb);
	if (err) {
		LOG_ERR("Cannot get scan response data (err: %d)", err);
		return err;
	}

	/* According to Fast Pair specification, the advertising interval should be no longer than
	 * 100 ms when discoverable and no longer than 250 ms when not discoverable.
	 */
	static const struct bt_le_adv_param adv_param = {
		.id = BT_ID_DEFAULT,
		.options = (BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_ONE_TIME),
		.interval_min = BT_GAP_ADV_FAST_INT_MIN_1,
		.interval_max = BT_GAP_ADV_FAST_INT_MAX_1,
		.peer = NULL,
	};

	err = bt_le_adv_start(&adv_param, ad, ad_len, sd, sd_len);

	if ((!err) && (fp_adv_mode != BT_FAST_PAIR_ADV_MODE_DISCOVERABLE)) {
		unsigned int rpa_timeout_ms = RPA_TIMEOUT_NON_DISCOVERABLE * MSEC_PER_SEC;
		int8_t rand;

		err = sys_csrand_get(&rand, sizeof(rand));
		if (!err) {
			rpa_timeout_ms += ((int)(RPA_TIMEOUT_OFFSET_MAX * MSEC_PER_SEC)) *
					  rand / INT8_MAX;
		} else {
			LOG_WRN("Cannot get random RPA timeout (err: %d). Used fixed value", err);
		}

		__ASSERT_NO_MSG(rpa_timeout_ms <= RPA_TIMEOUT_FAST_PAIR_MAX * MSEC_PER_SEC);
		int ret = k_work_schedule(&rpa_rotate, K_MSEC(rpa_timeout_ms));

		__ASSERT_NO_MSG(ret == 1);
		ARG_UNUSED(ret);
	}

	return err;

}

static void rpa_rotate_fn(struct k_work *w)
{
	(void)adv_start_internal(adv_helper_fp_adv_mode);
}

int bt_adv_helper_adv_start(enum bt_fast_pair_adv_mode fp_adv_mode, bool new_adv_session)
{
	int ret = k_work_cancel_delayable(&rpa_rotate);

	__ASSERT_NO_MSG(ret == 0);
	ARG_UNUSED(ret);

	adv_helper_fp_adv_mode = fp_adv_mode;
	adv_helper_new_adv_session = new_adv_session;

	LOG_INF("Looking for a new peer: %s", pairing_mode(adv_helper_fp_adv_mode) ? "yes" : "no");

	return adv_start_internal(fp_adv_mode);
}

int bt_adv_helper_adv_stop(void)
{
	int ret = k_work_cancel_delayable(&rpa_rotate);

	__ASSERT_NO_MSG(ret == 0);
	ARG_UNUSED(ret);

	return bt_le_adv_stop();
}
