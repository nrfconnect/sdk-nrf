/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <stdbool.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include "advertising.h"

static struct bt_le_ext_adv *adv_set;
static const struct advertising_cb *adv_cb;
static bool initialized;

static const struct bt_data connectable_ad_data[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static const struct bt_data non_connectable_ad_data[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
	BT_DATA_BYTES(BT_DATA_URI, /* The URI of the https://www.nordicsemi.com website */
		      0x17,	   /* UTF-8 code point for "https:" */
		      '/', '/', 'w', 'w', 'w', '.', 'n', 'o', 'r', 'd', 'i', 'c', 's', 'e', 'm',
		      'i', '.', 'c', 'o', 'm'),
};

static const struct bt_data non_connectable_sd_data[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static const struct bt_le_adv_param *connectable_ad_params =
	BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONN, CONFIG_BT_POWER_PROFILING_CONNECTABLE_ADV_INTERVAL_MIN,
			CONFIG_BT_POWER_PROFILING_CONNECTABLE_ADV_INTERVAL_MAX, NULL);

static const struct bt_le_adv_param *non_connectable_ad_params = BT_LE_ADV_PARAM(
	BT_LE_ADV_OPT_NONE, CONFIG_BT_POWER_PROFILING_NON_CONNECTABLE_ADV_INTERVAL_MIN,
	CONFIG_BT_POWER_PROFILING_NON_CONNECTABLE_ADV_INTERVAL_MAX, NULL);

static void adv_sent(struct bt_le_ext_adv *adv, struct bt_le_ext_adv_sent_info *info)
{
	ARG_UNUSED(adv);
	ARG_UNUSED(info);

	if (adv_cb && adv_cb->terminated) {
		adv_cb->terminated();
	}
}

static const struct bt_le_ext_adv_cb adv_callbacks = {
	.sent = adv_sent,
};

static int start_connectable(uint16_t timeout)
{
	int err;
	struct bt_le_ext_adv_start_param ext_start_param = {
		.timeout = timeout,
		.num_events = 0,
	};

	(void)bt_le_ext_adv_stop(adv_set);
	err = bt_le_ext_adv_update_param(adv_set, connectable_ad_params);
	if (err) {
		return err;
	}

	err = bt_le_ext_adv_set_data(adv_set, connectable_ad_data, ARRAY_SIZE(connectable_ad_data),
				     NULL, 0);
	if (err) {
		return err;
	}

	return bt_le_ext_adv_start(adv_set, &ext_start_param);
}

static int start_non_connectable(uint16_t timeout)
{
	int err;
	struct bt_le_ext_adv_start_param ext_start_param = {
		.timeout = timeout,
		.num_events = 0,
	};

	(void)bt_le_ext_adv_stop(adv_set);
	err = bt_le_ext_adv_update_param(adv_set, non_connectable_ad_params);
	if (err) {
		return err;
	}

	err = bt_le_ext_adv_set_data(adv_set, non_connectable_ad_data,
				     ARRAY_SIZE(non_connectable_ad_data), non_connectable_sd_data,
				     ARRAY_SIZE(non_connectable_sd_data));
	if (err) {
		return err;
	}

	return bt_le_ext_adv_start(adv_set, &ext_start_param);
}

int advertising_init(const struct advertising_cb *cb)
{
	int err;

	if (!cb || !cb->terminated) {
		return -EINVAL;
	}

	if (initialized) {
		return -EALREADY;
	}

	adv_cb = cb;

	err = bt_le_ext_adv_create(connectable_ad_params, &adv_callbacks, &adv_set);
	if (err) {
		return err;
	}

	initialized = true;

	return 0;
}

int advertising_start_connectable(void)
{
	if (!initialized) {
		return -EINVAL;
	}

	return start_connectable(CONFIG_BT_POWER_PROFILING_CONNECTABLE_ADV_DURATION);
}

int advertising_start_connectable_nfc(void)
{
	if (!initialized) {
		return -EINVAL;
	}

	return start_connectable(CONFIG_BT_POWER_PROFILING_NFC_ADV_DURATION);
}

int advertising_start_non_connectable(void)
{
	if (!initialized) {
		return -EINVAL;
	}

	return start_non_connectable(CONFIG_BT_POWER_PROFILING_NON_CONNECTABLE_ADV_DURATION);
}

int advertising_stop(void)
{
	if (!initialized || !adv_set) {
		return 0;
	}

	return bt_le_ext_adv_stop(adv_set);
}
