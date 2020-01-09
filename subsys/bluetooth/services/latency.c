/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <string.h>
#include <sys/printk.h>
#include <zephyr/types.h>
#include <logging/log.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/hci.h>
#include <bluetooth/uuid.h>
#include <bluetooth/services/latency.h>

LOG_MODULE_REGISTER(bt_gatt_latency, CONFIG_BT_GATT_LATENCY_LOG_LEVEL);

enum {
	LATENCY_INITIALIZED
};

static const struct bt_gatt_latency_cb *callbacks;

static ssize_t received_latency_request(struct bt_conn *conn,
					const struct bt_gatt_attr *attr,
					const void *buf, u16_t len,
					u16_t offset, u8_t flags)
{
	LOG_DBG("Received Latency request, data %p length %u", buf, len);

	if (callbacks && callbacks->latency_request) {
		callbacks->latency_request(buf, len);
	}

	return len;
}

BT_GATT_SERVICE_DEFINE(latency_svc,
BT_GATT_PRIMARY_SERVICE(BT_UUID_LATENCY),
	BT_GATT_CHARACTERISTIC(BT_UUID_LATENCY_CHAR,
		BT_GATT_CHRC_WRITE,
		BT_GATT_PERM_WRITE,
		NULL, received_latency_request, NULL),
);

int bt_gatt_latency_init(struct bt_gatt_latency *latency,
			 const struct bt_gatt_latency_cb *cb)
{
	if (!latency) {
		return -EINVAL;
	}

	if (atomic_test_and_set_bit(&latency->state, LATENCY_INITIALIZED)) {
		return -EALREADY;
	}

	callbacks = cb;
	return 0;
}
