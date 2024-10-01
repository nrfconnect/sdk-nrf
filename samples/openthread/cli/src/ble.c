/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/gatt.h>

#include "ble.h"

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define ADV_INT_MIN 0x01e0 /* 300 ms */
#define ADV_INT_MAX 0x0260 /* 380 ms */

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static struct bt_conn_cb conn_callbacks;

void ble_enable(void)
{
	const struct bt_le_adv_param *adv_param = BT_LE_ADV_PARAM(
		BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_ONE_TIME, ADV_INT_MIN, ADV_INT_MAX, NULL);

	bt_enable(NULL);
	bt_conn_cb_register(&conn_callbacks);
	bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), NULL, 0);
}
