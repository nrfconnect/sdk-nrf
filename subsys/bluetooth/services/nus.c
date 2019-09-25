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

static bool is_notification_enabled(struct bt_conn *conn,
				    const struct bt_gatt_ccc_cfg *ccd)
{
	const bt_addr_le_t *conn_addr = bt_conn_get_dst(conn);

	for (size_t i = 0; i < BT_GATT_CCC_MAX; i++) {

		const bt_addr_le_t *ccd_addr = &ccd[i].peer;

		if ((!memcmp(conn_addr, ccd_addr, sizeof(bt_addr_le_t))) &&
		    (ccd[i].value == BT_GATT_CCC_NOTIFY)) {
			return true;
		}
	}

	LOG_DBG("Notification disabled for conn: %p", conn);

	return false;
}

static void nuslc_ccc_cfg_changed(const struct bt_gatt_attr *attr,
				  u16_t value)
{
	char addr[BT_ADDR_LE_STR_LEN] = {0};
	struct bt_gatt_ccc_cfg *cfg =
		(struct bt_gatt_ccc_cfg *)attr->user_data;

	bt_addr_le_to_str(&cfg->peer, addr, ARRAY_SIZE(addr));

	LOG_DBG("%s CCCD has changed, state: %s", log_strdup(addr),
		value ? "notification enabled" : "notification disabled");
}

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
	BT_GATT_CCC(nuslc_ccc_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
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
	const struct bt_gatt_attr *ccc_attr = &nus_svc.attrs[2];
	const struct bt_gatt_ccc_cfg *ccc_cfg = ccc_attr->user_data;

	params.attr = ccc_attr;
	params.data = data;
	params.len = len;
	params.func = on_sent;

	if (!conn) {
		LOG_DBG("Notification send to all connected peers");
		return  bt_gatt_notify_cb(NULL, &params);
	} else if (is_notification_enabled(conn, ccc_cfg)) {
		return bt_gatt_notify_cb(conn, &params);
	} else {
		return -EINVAL;
	}
}
