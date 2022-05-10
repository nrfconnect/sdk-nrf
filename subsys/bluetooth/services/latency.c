/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <zephyr/sys/printk.h>
#include <zephyr/types.h>
#include <zephyr/logging/log.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <bluetooth/services/latency.h>

LOG_MODULE_REGISTER(bt_latency, CONFIG_BT_LATENCY_LOG_LEVEL);

enum {
	LATENCY_INITIALIZED
};

static const struct bt_latency_cb *callbacks;

static ssize_t received_latency_request(struct bt_conn *conn,
					const struct bt_gatt_attr *attr,
					const void *buf, uint16_t len,
					uint16_t offset, uint8_t flags)
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

int bt_latency_init(struct bt_latency *latency, const struct bt_latency_cb *cb)
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
