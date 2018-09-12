/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

/** @file
 *  @brief Nordic UART Bridge Service (NUS) sample
 */

#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include <bluetooth/services/nus.h>

static struct bt_gatt_ccc_cfg nuslc_ccc_cfg[BT_GATT_CCC_MAX];
static bool                   notify_enabled;

static struct bt_nus_cb nus_cb;

static void nuslc_ccc_cfg_changed(const struct bt_gatt_attr *attr,
				  u16_t value)
{
	notify_enabled = (value & BT_GATT_CCC_NOTIFY) ? true : false;
}

static ssize_t on_receive(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr,
			  const void *buf,
			  u16_t len,
			  u16_t offset,
			  u8_t flags)
{
	if (nus_cb.received_cb) {
		nus_cb.received_cb(buf, len);
	}

	return len;
}

static ssize_t on_sent(struct bt_conn *conn,
		       const struct bt_gatt_attr *attr,
		       void *buf,
		       u16_t len,
		       u16_t offset)
{
	if (nus_cb.sent_cb) {
		nus_cb.sent_cb(buf, len);
	}

	return len;
}

/* UART Service Declaration */
static struct bt_gatt_attr attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_NUS_SERVICE),
	BT_GATT_CHARACTERISTIC(BT_UUID_NUS_TX,
			       BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ,
			       on_sent, NULL, NULL),
	BT_GATT_CCC(nuslc_ccc_cfg, nuslc_ccc_cfg_changed),
	BT_GATT_CHARACTERISTIC(BT_UUID_NUS_RX,
			       BT_GATT_CHRC_WRITE |
			       BT_GATT_CHRC_WRITE_WITHOUT_RESP,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			       NULL, on_receive, NULL),
};

static struct bt_gatt_service nus_svc = BT_GATT_SERVICE(attrs);

int nus_init(struct bt_nus_cb *callbacks)
{
	if (callbacks) {
		nus_cb.received_cb = callbacks->received_cb;
		nus_cb.sent_cb     = callbacks->sent_cb;
	}

	return bt_gatt_service_register(&nus_svc);
}

int nus_send(const u8_t *data, uint16_t len)
{
	if (!notify_enabled) {
		return -EFAULT;
	}

	return bt_gatt_notify(NULL, &attrs[2], data, len);
}

