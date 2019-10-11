/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include <bluetooth/services/nus.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(bt_gatt_nus, CONFIG_BT_GATT_NUS_LOG_LEVEL);

static struct bt_gatt_nus_cb nus_cb;

static ssize_t on_receive(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr,
			  const void *buf,
			  u16_t len,
			  u16_t offset,
			  u8_t flags)
{
	LOG_DBG("Received data, handle %d, conn %p",
		attr->handle, conn);

	if (nus_cb.received_cb) {
		nus_cb.received_cb(conn, buf, len);
}
	return len;
}

static void on_sent(struct bt_conn *conn, void *user_data)
{
	ARG_UNUSED(user_data);

	LOG_DBG("Data send, conn %p", conn);

	if (nus_cb.sent_cb) {
		nus_cb.sent_cb(conn);
	}
}

/* UART Service Declaration */
BT_GATT_SERVICE_DEFINE(nus_svc,
BT_GATT_PRIMARY_SERVICE(BT_UUID_NUS_SERVICE),
	BT_GATT_CHARACTERISTIC(BT_UUID_NUS_TX,
			       BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ,
			       NULL, NULL, NULL),
	BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_CHARACTERISTIC(BT_UUID_NUS_RX,
			       BT_GATT_CHRC_WRITE |
			       BT_GATT_CHRC_WRITE_WITHOUT_RESP,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			       NULL, on_receive, NULL),
);

int bt_gatt_nus_init(struct bt_gatt_nus_cb *callbacks)
{
	if (callbacks) {
		nus_cb.received_cb = callbacks->received_cb;
		nus_cb.sent_cb     = callbacks->sent_cb;
	}

	return 0;
}

int bt_gatt_nus_send(struct bt_conn *conn, const u8_t *data, uint16_t len)
{
	struct bt_gatt_notify_params params = {0};
	const struct bt_gatt_attr *attr = &nus_svc.attrs[2];

	params.attr = attr;
	params.data = data;
	params.len = len;
	params.func = on_sent;

	if (!conn) {
		LOG_DBG("Notification send to all connected peers");
		return bt_gatt_notify_cb(NULL, &params);
	} else if (bt_gatt_is_subscribed(conn, attr, BT_GATT_CCC_NOTIFY)) {
		return bt_gatt_notify_cb(conn, &params);
	} else {
		return -EINVAL;
	}
}
