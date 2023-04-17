/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <soc.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include <bluetooth/gatt_dm.h>

#include <dk_buttons_and_leds.h>

#define DEVICE_NAME             CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN         (sizeof(DEVICE_NAME) - 1)

/* Key used to accept or reject passkey value */
#define KEY_PAIRING_ACCEPT DK_BTN1_MSK
#define KEY_PAIRING_REJECT DK_BTN2_MSK

static struct bt_conn *pairing_confirmation_conn;
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static void discover_all_completed(struct bt_gatt_dm *dm, void *ctx)
{
	char uuid_str[37];

	const struct bt_gatt_dm_attr *gatt_service_attr =
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

static void connected(struct bt_conn *conn, uint8_t err)
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

static void disconnected(struct bt_conn *conn, uint8_t reason)
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

BT_CONN_CB_DEFINE(conn_callbacks) = {
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

	__ASSERT_NO_MSG(!pairing_confirmation_conn);
	pairing_confirmation_conn = bt_conn_ref(conn);

	printk("Pairing confirmation required for %s\n", addr);
	printk("Press Button 1 to confirm, Button 2 to reject.\n");
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

	if (pairing_confirmation_conn) {
		bt_conn_unref(pairing_confirmation_conn);
		pairing_confirmation_conn = NULL;
	}
}

static struct bt_conn_auth_cb conn_auth_callbacks = {
	.cancel = auth_cancel,
	.pairing_confirm = pairing_confirm,
};

static struct bt_conn_auth_info_cb conn_auth_info_callbacks = {
	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed
};

static void button_changed(uint32_t button_state, uint32_t has_changed)
{
	int err;
	uint32_t buttons = button_state & has_changed;

	if (buttons & KEY_PAIRING_ACCEPT) {
		struct bt_conn *conn = pairing_confirmation_conn;
		pairing_confirmation_conn = NULL;

		if (conn) {
			err = bt_conn_auth_pairing_confirm(conn);
			if (err) {
				printk("Failed to confirm the pairing: %d\n", err);
			} else {
				printk("Pairing confirmed\n");
			}

			bt_conn_unref(conn);
			conn = NULL;
		}
	}

	if (buttons & KEY_PAIRING_REJECT) {
		struct bt_conn *conn = pairing_confirmation_conn;
		pairing_confirmation_conn = NULL;

		if (conn) {
			err = bt_conn_auth_cancel(conn);
			if (err) {
				printk("Failed to reject the pairing: %d\n", err);
			} else {
				printk("Pairing rejected\n");
			}

			bt_conn_unref(conn);
			conn = NULL;
		}
	}
}

int main(void)
{
	int err;

	printk("Starting GATT Discovery Manager example\n");

	err = dk_buttons_init(button_changed);
	if (err) {
		printk("Cannot init buttons (err: %d)\n", err);
	}

	err = bt_enable(NULL);
	if (err) {
		printk("BLE init failed with error code %d\n", err);
		return 0;
	}

	err = bt_conn_auth_cb_register(&conn_auth_callbacks);
	if (err) {
		printk("Failed to register authorization callbacks.\n");
		return 0;
	}

	err = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
	if (err) {
		printk("Failed to register authorization info callbacks.\n");
		return 0;
	}

	err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad),
			      NULL, 0);
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return 0;
	}

	printk("Advertising successfully started\n");

	return 0;
}
