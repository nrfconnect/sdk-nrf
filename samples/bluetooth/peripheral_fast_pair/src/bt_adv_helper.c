/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/random/rand32.h>
#include <zephyr/sys/__assert.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <bluetooth/services/fast_pair.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(fp_sample, LOG_LEVEL_INF);

#include "bt_tx_power_adv.h"
#include "bt_adv_helper.h"

#define DEVICE_NAME             CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN         (sizeof(DEVICE_NAME) - 1)

#define TX_POWER_ADV_DATA_POS	(ARRAY_SIZE(ad) - 2)
#define FAST_PAIR_ADV_DATA_POS	(ARRAY_SIZE(ad) - 1)

#define SEC_PER_MIN		60U

/* According to Fast Pair specification RPA rotation must be synchronized with generating new salt
 * for Acount Key Filter advertising data. The RPA rotation must occur at least every 15 minutes
 * while the device is actively advertising in Fast Pair not discoverable mode. The value of this
 * timeout must be lower than CONFIG_BT_RPA_TIMEOUT (3600 seconds) to ensure that RPA rotation will
 * always trigger update of Account Key Filter advertising data.
 */
#define RPA_TIMEOUT_NON_DISCOVERABLE	(13 * SEC_PER_MIN)
#define RPA_TIMEOUT_OFFSET_MAX		(2 * SEC_PER_MIN)
#define RPA_TIMEOUT_FAST_PAIR_MAX	(15 * SEC_PER_MIN)

/* The Bluetooth Core Specification v5.2 (Vol. 4, Part E, 7.8.45) allows for time range between
 * 1 second and 3600 seconds. In case of Fast Pair we should avoid RPA address rotation when device
 * is Fast Pair discoverable. If not discoverable, the RPA address rotation should be done together
 * with Fast Pair payload update (responsibility of sample/application).
 */
BUILD_ASSERT(CONFIG_BT_RPA_TIMEOUT == 3600);

/* Make sure that RPA rotation will be done together with Fast Pair payload update. */
BUILD_ASSERT(RPA_TIMEOUT_NON_DISCOVERABLE < CONFIG_BT_RPA_TIMEOUT);

static struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
		      BT_UUID_16_ENCODE(BT_UUID_HIDS_VAL),
		      BT_UUID_16_ENCODE(BT_UUID_BAS_VAL)),
	/* Empty placeholder for TX power advertising data. */
	{
	},
	/* Empty placeholder for Fast Pair advertising data. */
	{
	},
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
	BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE,
		      (CONFIG_BT_DEVICE_APPEARANCE & BIT_MASK(__CHAR_BIT__)),
		      (CONFIG_BT_DEVICE_APPEARANCE >> __CHAR_BIT__)),
};

static enum bt_fast_pair_adv_mode adv_helper_fp_adv_mode;

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

static int bt_adv_helper_fast_pair_prepare(struct bt_data *adv_data,
					   enum bt_fast_pair_adv_mode fp_adv_mode)
{
	/* Make sure that Fast Pair data was freed and set to NULL to prevent memory leaks. */
	if (adv_data->data) {
		k_free((void *)adv_data->data);
		adv_data->data = NULL;
	}

	/* Fast Pair pairing mode must be manually set by the sample. */
	bt_fast_pair_set_pairing_mode(fp_adv_mode == BT_FAST_PAIR_ADV_MODE_DISCOVERABLE);

	size_t buf_size = bt_fast_pair_adv_data_size(fp_adv_mode);
	uint8_t *buf = k_malloc(buf_size);

	if (!buf) {
		return -ENOMEM;
	}

	int err = bt_fast_pair_adv_data_fill(adv_data, buf, buf_size, fp_adv_mode);

	if (err) {
		k_free(buf);
	}

	return err;
}

static int bt_adv_helper_tx_power_prepare(struct bt_data *adv_data)
{
	/* Make sure that TX power data was freed and set to NULL to prevent memory leaks. */
	if (adv_data->data) {
		k_free((void *)adv_data->data);
		adv_data->data = NULL;
	}

	size_t buf_size = bt_tx_power_adv_data_size();
	uint8_t *buf = k_malloc(buf_size);

	if (!buf) {
		return -ENOMEM;
	}

	int err = bt_tx_power_adv_data_fill(adv_data, buf, buf_size);

	if (err) {
		k_free(buf);
	}

	return err;
}

static int adv_start_internal(enum bt_fast_pair_adv_mode fp_adv_mode)
{
	struct bt_le_oob oob;
	int err = bt_le_adv_stop();

	if (err) {
		LOG_ERR("Cannot stop advertising (err: %d)", err);
		return err;
	}

	err = bt_adv_helper_fast_pair_prepare(&ad[FAST_PAIR_ADV_DATA_POS], fp_adv_mode);
	if (err) {
		LOG_ERR("Cannot prepare Fast Pair advertising data (err: %d)", err);
		return err;
	}

	err = bt_adv_helper_tx_power_prepare(&ad[TX_POWER_ADV_DATA_POS]);
	if (err) {
		LOG_ERR("Cannot prepare TX power advertising data (err: %d)", err);
		return err;
	}

	/* Generate new Resolvable Private Address (RPA). */
	err = bt_le_oob_get_local(BT_ID_DEFAULT, &oob);
	if (err) {
		LOG_ERR("Cannot trigger RPA rotation (err: %d)", err);
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

	err = bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));

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

int bt_adv_helper_adv_start(enum bt_fast_pair_adv_mode fp_adv_mode)
{
	int ret = k_work_cancel_delayable(&rpa_rotate);

	__ASSERT_NO_MSG(ret == 0);
	ARG_UNUSED(ret);

	adv_helper_fp_adv_mode = fp_adv_mode;

	return adv_start_internal(fp_adv_mode);
}

int bt_adv_helper_adv_stop(void)
{
	int ret = k_work_cancel_delayable(&rpa_rotate);

	__ASSERT_NO_MSG(ret == 0);
	ARG_UNUSED(ret);

	return bt_le_adv_stop();
}
