/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include "pwr_service.h"

#define PWR_SERVICE_UUID_VAL BT_UUID_128_ENCODE(0x00001630, 0x1212, 0xEFDE, 0x1523, 0x785FEABCD123)
#define PWR_CHAR_UUID_VAL BT_UUID_128_ENCODE(0x00001630, 0x1212, 0xEFDE, 0x1524, 0x785FEABCD123)

#define PWR_SERVICE_UUID BT_UUID_DECLARE_128(PWR_SERVICE_UUID_VAL)
#define PWR_CHAR_UUID BT_UUID_DECLARE_128(PWR_CHAR_UUID_VAL)

#define PWR_CHAR_ATTR_IDX 2

static const struct pwr_service_cb *pwr_cb;
static void *pwr_cb_user_data;
static uint8_t notify_data[CONFIG_BT_POWER_PROFILING_DATA_LENGTH];

static ssize_t pwr_char_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			     void *buf, uint16_t len, uint16_t offset)
{

	return bt_gatt_attr_read(conn, attr, buf, len, offset, notify_data, sizeof(notify_data));
}

static void cccd_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	bool notify_enabled = (value == BT_GATT_CCC_NOTIFY);

	if (pwr_cb->notify_status_cb) {
		pwr_cb->notify_status_cb(notify_enabled ? PWR_SERVICE_STATUS_NOTIFY_ENABLED :
							  PWR_SERVICE_STATUS_NOTIFY_DISABLED,
					 pwr_cb_user_data);
	}
}

BT_GATT_SERVICE_DEFINE(pwr_svc,
	BT_GATT_PRIMARY_SERVICE(PWR_SERVICE_UUID),
	BT_GATT_CHARACTERISTIC(PWR_CHAR_UUID,
			       (BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY),
			       (BT_GATT_PERM_READ),
			       pwr_char_read, NULL, NULL),
	BT_GATT_CCC(cccd_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)
);

static void pwr_service_notify_complete(struct bt_conn *conn, void *user_data)
{
	if (pwr_cb->sent_cb) {
		pwr_cb->sent_cb(conn, pwr_cb_user_data);
	}
}

int pwr_service_notify(struct bt_conn *conn)
{
	const struct bt_gatt_attr *attr = &pwr_svc.attrs[PWR_CHAR_ATTR_IDX];
	struct bt_gatt_notify_params params;

	memset(&params, 0, sizeof(params));

	params.attr = attr;
	params.data = notify_data;
	params.len = sizeof(notify_data);
	params.func = pwr_service_notify_complete;

	if (!conn) {
		return bt_gatt_notify_cb(NULL, &params);
	} else if (bt_gatt_is_subscribed(conn, attr, BT_GATT_CCC_NOTIFY)) {
		return bt_gatt_notify_cb(conn, &params);
	} else {
		return -EINVAL;
	}
}

int pwr_service_cb_register(const struct pwr_service_cb *cb, void *user_data)
{
	if (pwr_cb) {
		return -EALREADY;
	}

	pwr_cb = cb;
	pwr_cb_user_data = user_data;

	return 0;
}
