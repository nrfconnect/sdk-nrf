/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include <bluetooth/services/nus.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bt_nus, CONFIG_BT_NUS_LOG_LEVEL);

static struct bt_nus_cb nus_cb;

static void nus_ccc_cfg_changed(const struct bt_gatt_attr *attr,
				  uint16_t value)
{
	if (nus_cb.send_enabled) {
		LOG_DBG("Notification has been turned %s",
			value == BT_GATT_CCC_NOTIFY ? "on" : "off");
		nus_cb.send_enabled(value == BT_GATT_CCC_NOTIFY ?
			BT_NUS_SEND_STATUS_ENABLED : BT_NUS_SEND_STATUS_DISABLED);
	}
}

static ssize_t on_receive(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr,
			  const void *buf,
			  uint16_t len,
			  uint16_t offset,
			  uint8_t flags)
{
	LOG_DBG("Received data, handle %d, conn %p",
		attr->handle, (void *)conn);

	if (nus_cb.received) {
		nus_cb.received(conn, buf, len);
}
	return len;
}

static void on_sent(struct bt_conn *conn, void *user_data)
{
	ARG_UNUSED(user_data);

	LOG_DBG("Data send, conn %p", (void *)conn);

	if (nus_cb.sent) {
		nus_cb.sent(conn);
	}
}

/* UART Service Declaration */
BT_GATT_SERVICE_DEFINE(nus_svc,
BT_GATT_PRIMARY_SERVICE(BT_UUID_NUS_SERVICE),
	BT_GATT_CHARACTERISTIC(BT_UUID_NUS_TX,
			       BT_GATT_CHRC_NOTIFY,
#ifdef CONFIG_BT_NUS_AUTHEN
			       BT_GATT_PERM_READ_AUTHEN,
#else
			       BT_GATT_PERM_READ,
#endif /* CONFIG_BT_NUS_AUTHEN */
			       NULL, NULL, NULL),
	BT_GATT_CCC(nus_ccc_cfg_changed,
#ifdef CONFIG_BT_NUS_AUTHEN
			       BT_GATT_PERM_READ_AUTHEN | BT_GATT_PERM_WRITE_AUTHEN),
#else
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
#endif /* CONFIG_BT_NUS_AUTHEN */
	BT_GATT_CHARACTERISTIC(BT_UUID_NUS_RX,
			       BT_GATT_CHRC_WRITE |
			       BT_GATT_CHRC_WRITE_WITHOUT_RESP,
#ifdef CONFIG_BT_NUS_AUTHEN
			       BT_GATT_PERM_READ_AUTHEN | BT_GATT_PERM_WRITE_AUTHEN,
#else
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
#endif /* CONFIG_BT_NUS_AUTHEN */
			       NULL, on_receive, NULL),
);

int bt_nus_init(struct bt_nus_cb *callbacks)
{
	if (callbacks) {
		nus_cb.received = callbacks->received;
		nus_cb.sent = callbacks->sent;
		nus_cb.send_enabled = callbacks->send_enabled;
	}

	return 0;
}

int bt_nus_send(struct bt_conn *conn, const uint8_t *data, uint16_t len)
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
