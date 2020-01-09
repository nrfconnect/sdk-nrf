/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <sys/printk.h>
#include <sys/byteorder.h>
#include <zephyr.h>
#include <gpio.h>
#include <soc.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include <bluetooth/gatt_dm.h>

#define DEVICE_NAME             CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN         (sizeof(DEVICE_NAME) - 1)

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static void discover_all_completed(struct bt_gatt_dm *dm, void *ctx)
{
	char uuid_str[37];

	const struct bt_gatt_attr *gatt_service_attr =
			bt_gatt_dm_service_get(dm);
	const struct bt_gatt_service_val *gatt_service =
			bt_gatt_dm_attr_service_val(gatt_service_attr);

	size_t attr_count = bt_gatt_dm_attr_cnt(dm);

	bt_uuid_to_str(gatt_service->uuid, uuid_str, sizeof(uuid_str));
	printk("Found service %s\n", uuid_str);
	printk("Attribute count: %d\n", attr_count);

	bt_gatt_dm_data_print(dm);
	bt_gatt_dm_data_release(dm);

	bt_gatt_dm_continue(dm, NULL);
}

static void discover_all_service_not_found(struct bt_conn *conn, void *ctx)
{
	printk("No more services\n");
}

static void discover_all_error_found(struct bt_conn *conn, int err, void *ctx)
{
	printk("The discovery procedure failed, err %d\n", err);
}

static struct bt_gatt_dm_cb discover_all_cb = {
	.completed = discover_all_completed,
	.service_not_found = discover_all_service_not_found,
	.error_found = discover_all_error_found,
};

static void connected(struct bt_conn *conn, u8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (err) {
		printk("Connection failed (err 0x%02x)\n", err);
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	printk("Connected %s\n", addr);

	err = bt_gatt_dm_start(conn, NULL, &discover_all_cb, NULL);
	if (err) {
		printk("Failed to start discovery (err %d)\n", err);
	}
}

static void disconnected(struct bt_conn *conn, u8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	printk("Disconnected from %s (reason 0x%02x)\n", addr, reason);
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		printk("Security changed: %s level %u\n", addr, level);
	} else {
		printk("Security failed: %s level %u err %d\n", addr, level,
			err);
	}
}

static struct bt_conn_cb conn_callbacks = {
	.connected        = connected,
	.disconnected     = disconnected,
	.security_changed = security_changed,
};

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing cancelled: %s\n", addr);
}

static void pairing_confirm(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	bt_conn_auth_pairing_confirm(conn);

	printk("Pairing confirmed: %s\n", addr);
}

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing completed: %s, bonded: %d\n", addr, bonded);
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing failed conn: %s, reason %d\n", addr, reason);
}

static struct bt_conn_auth_cb conn_auth_callbacks = {
	.cancel = auth_cancel,
	.pairing_confirm = pairing_confirm,
	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed
};

void main(void)
{
	int err;

	printk("Starting GATT Discovery Manager example\n");

	err = bt_enable(NULL);
	if (err) {
		printk("BLE init failed with error code %d\n", err);
		return;
	}

	bt_conn_cb_register(&conn_callbacks);
	if (err) {
		printk("Failed to register connection callbacks.\n");
		return;
	}

	err = bt_conn_auth_cb_register(&conn_auth_callbacks);
	if (err) {
		printk("Failed to register authorization callbacks.\n");
		return;
	}

	err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad),
			      NULL, 0);
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");
}
