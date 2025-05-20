/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/init.h>

/* .. include_startingpoint_mesh_smp_dfu_rst_1 */
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <bluetooth/mesh/vnd/le_pair_resp.h>

static void passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Please enter passkey (%d) on %s\n", passkey, addr);
}

static void cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing cancelled: %s\n", addr);
}

static void pairing_confirm(struct bt_conn *conn)
{
	int err;

	/* Automatically confirm pairing request from the device side. */
	err = bt_conn_auth_pairing_confirm(conn);
	if (err) {
		printf("Can't confirm pairing (err: %d)\n", err);
		return;
	}

	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing confirmed: %s\n", addr);
}

static struct bt_conn_auth_cb auth_cb = {
	/* Enable passkey_display callback to enable the display capability. */
	.passkey_display = passkey_display,
	/* These 2 callbacks are required for passkey_display callback. */
	.cancel = cancel,
	.pairing_confirm = pairing_confirm,
};

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing completed: %s, bonded: %d\n", addr, bonded);

	/* Invalidate the previously used passkey. */
	bt_mesh_le_pair_resp_passkey_invalidate();
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing failed: %s, reason %d\n", addr, reason);

	/* Invalidate the previously used passkey. */
	bt_mesh_le_pair_resp_passkey_invalidate();
}

static struct bt_conn_auth_info_cb conn_auth_info_callbacks = {
	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed
};

int smp_bt_auth_init(void)
{
	int err;

	err = bt_conn_auth_cb_register(&auth_cb);
	if (err) {
		printk("Can't register authentication callbacks (err: %d)\n", err);
		return err;
	}

	err = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
	if (err) {
		printk("Can't register authentication information callbacks (err: %d)\n", err);
		return err;
	}

	return 0;
}
