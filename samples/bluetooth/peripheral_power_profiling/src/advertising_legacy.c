/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <stdbool.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include "advertising.h"

static const struct advertising_cb *adv_cb;
static bool initialized;

static const struct bt_data connectable_ad_data[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static const struct bt_data non_connectable_ad_data[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
	BT_DATA_BYTES(BT_DATA_URI, /* The URI of the https://www.nordicsemi.com website */
		      0x17, /* UTF-8 code point for "https:" */
		      '/', '/', 'w', 'w', 'w', '.',
		      'n', 'o', 'r', 'd', 'i', 'c', 's', 'e', 'm', 'i', '.',
		      'c', 'o', 'm'),
};

static const struct bt_data non_connectable_sd_data[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static const struct bt_le_adv_param *connectable_ad_params =
	BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONN,
			CONFIG_BT_POWER_PROFILING_CONNECTABLE_ADV_INTERVAL_MIN,
			CONFIG_BT_POWER_PROFILING_CONNECTABLE_ADV_INTERVAL_MAX,
			NULL);

static const struct bt_le_adv_param *non_connectable_ad_params =
	BT_LE_ADV_PARAM(BT_LE_ADV_OPT_NONE,
			CONFIG_BT_POWER_PROFILING_NON_CONNECTABLE_ADV_INTERVAL_MIN,
			CONFIG_BT_POWER_PROFILING_NON_CONNECTABLE_ADV_INTERVAL_MAX,
			NULL);

static void timeout_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	(void)bt_le_adv_stop();

	if (adv_cb && adv_cb->terminated) {
		adv_cb->terminated();
	}
}

static K_WORK_DELAYABLE_DEFINE(adv_timeout_work, timeout_work_handler);

static void legacy_connected(struct bt_conn *conn, uint8_t err)
{
	ARG_UNUSED(conn);

	if (err) {
		return;
	}

	(void)k_work_cancel_delayable(&adv_timeout_work);
}

BT_CONN_CB_DEFINE(legacy_adv_conn_cb) = {
	.connected = legacy_connected,
};

static void schedule_timeout(uint16_t duration_10ms)
{
	(void)k_work_cancel_delayable(&adv_timeout_work);
	(void)k_work_schedule(&adv_timeout_work, K_MSEC((uint32_t)duration_10ms * 10U));
}

int advertising_init(const struct advertising_cb *cb)
{
	if (!cb || !cb->terminated) {
		return -EINVAL;
	}

	if (initialized) {
		return -EALREADY;
	}

	adv_cb = cb;
	initialized = true;

	return 0;
}

int advertising_start_connectable(void)
{
	int err;

	if (!initialized) {
		return -EINVAL;
	}

	(void)k_work_cancel_delayable(&adv_timeout_work);
	(void)bt_le_adv_stop();

	err = bt_le_adv_start(connectable_ad_params, connectable_ad_data,
			      ARRAY_SIZE(connectable_ad_data), NULL, 0);
	if (err) {
		return err;
	}

	schedule_timeout(CONFIG_BT_POWER_PROFILING_CONNECTABLE_ADV_DURATION);

	return 0;
}

int advertising_start_connectable_nfc(void)
{
	int err;

	if (!initialized) {
		return -EINVAL;
	}

	(void)k_work_cancel_delayable(&adv_timeout_work);
	(void)bt_le_adv_stop();

	err = bt_le_adv_start(connectable_ad_params, connectable_ad_data,
			      ARRAY_SIZE(connectable_ad_data), NULL, 0);
	if (err) {
		return err;
	}

	schedule_timeout(CONFIG_BT_POWER_PROFILING_NFC_ADV_DURATION);

	return 0;
}

int advertising_start_non_connectable(void)
{
	int err;

	if (!initialized) {
		return -EINVAL;
	}

	(void)k_work_cancel_delayable(&adv_timeout_work);
	(void)bt_le_adv_stop();

	err = bt_le_adv_start(non_connectable_ad_params, non_connectable_ad_data,
			      ARRAY_SIZE(non_connectable_ad_data), non_connectable_sd_data,
			      ARRAY_SIZE(non_connectable_sd_data));
	if (err) {
		return err;
	}

	schedule_timeout(CONFIG_BT_POWER_PROFILING_NON_CONNECTABLE_ADV_DURATION);

	return 0;
}

int advertising_stop(void)
{
	(void)k_work_cancel_delayable(&adv_timeout_work);

	if (!initialized) {
		return 0;
	}

	return bt_le_adv_stop();
}
