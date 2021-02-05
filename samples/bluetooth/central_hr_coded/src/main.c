/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief Central Heart Rate over LE Coded PHY sample
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr.h>
#include <sys/printk.h>
#include <sys/byteorder.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/gatt_dm.h>
#include <bluetooth/scan.h>

static struct bt_conn *default_conn;

static struct bt_gatt_subscribe_params subscribe_params;

static uint8_t notify_func(struct bt_conn *conn,
		struct bt_gatt_subscribe_params *params,
		const void *data, uint16_t length)
{
	if (!data) {
		printk("[UNSUBSCRIBED]\n");
		params->value_handle = 0U;
		return BT_GATT_ITER_STOP;
	}

	if (length == 2) {
		uint8_t hr_bpm = ((uint8_t *)data)[1];

		printk("[NOTIFICATION] Heart Rate %u bpm\n", hr_bpm);
	} else {
		printk("[NOTIFICATION] data %p length %u\n", data, length);
	}

	return BT_GATT_ITER_CONTINUE;
}

static void discover_hrs_completed(struct bt_gatt_dm *dm, void *ctx)
{
	int err;

	const struct bt_gatt_dm_attr *gatt_chrc;
	const struct bt_gatt_dm_attr *gatt_desc;

	struct bt_conn *conn = bt_gatt_dm_conn_get(dm);

	printk("The discovery procedure succeeded\n");

	bt_gatt_dm_data_print(dm);

	/* Heart rate characteristic */
	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_HRS_MEASUREMENT);
	if (!gatt_chrc) {
		printk("No heart rate measurement characteristic found");
		return;
	}

	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc,
			BT_UUID_HRS_MEASUREMENT);
	if (!gatt_desc) {
		printk("No heat rate measurement characteristic value found");
		return;
	}

	subscribe_params.value_handle = gatt_desc->handle;

	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc,
			BT_UUID_GATT_CCC);

	if (!gatt_desc) {
		printk("No heart rate CCC descriptor found. "
		       "Heart rate service does not support notifications.");
		return;
	}

	subscribe_params.notify = notify_func;
	subscribe_params.value = BT_GATT_CCC_NOTIFY;
	subscribe_params.ccc_handle = gatt_desc->handle;

	err = bt_gatt_subscribe(conn, &subscribe_params);
	if (err && err != -EALREADY) {
		printk("Subscribe failed (err %d)\n", err);
	} else {
		printk("[SUBSCRIBED]\n");
	}

	err = bt_gatt_dm_data_release(dm);
	if (err) {
		printk("Could not release the discovery data (err %d)\n", err);
	}
}

static void discover_hrs_service_not_found(struct bt_conn *conn, void *ctx)
{
	printk("No more services\n");
}

static void discover_hrs_error_found(struct bt_conn *conn, int err, void *ctx)
{
	printk("The discovery procedure failed, err %d\n", err);
}


static struct bt_gatt_dm_cb discover_hrs_cb = {
	.completed = discover_hrs_completed,
	.service_not_found = discover_hrs_service_not_found,
	.error_found = discover_hrs_error_found,
};

static void scan_filter_match(struct bt_scan_device_info *device_info,
			      struct bt_scan_filter_match *filter_match,
			      bool connectable)
{
	int err;
	char addr[BT_ADDR_LE_STR_LEN];
	struct bt_conn_le_create_param *conn_params;

	bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));

	printk("Filters matched. Address: %s connectable: %s\n",
		addr, connectable ? "yes" : "no");

	err = bt_scan_stop();
	if (err) {
		printk("Stop LE scan failed (err %d)\n", err);
	}

	conn_params = BT_CONN_LE_CREATE_PARAM(
			BT_CONN_LE_OPT_CODED | BT_CONN_LE_OPT_NO_1M,
			BT_GAP_SCAN_FAST_INTERVAL,
			BT_GAP_SCAN_FAST_INTERVAL);

	err = bt_conn_le_create(device_info->recv_info->addr, conn_params,
				BT_LE_CONN_PARAM_DEFAULT,
				&default_conn);

	if (err) {
		printk("Create conn failed (err %d)\n", err);

		err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
		if (err) {
			printk("Scanning failed to start (err %d)\n", err);
			return;
		}
	}

	printk("Connection pending\n");
}

BT_SCAN_CB_INIT(scan_cb, scan_filter_match, NULL, NULL, NULL);

static void scan_init(void)
{
	int err;

	/* Use active scanning and disable duplicate filtering to handle any
	 * devices that might update their advertising data at runtime. */
	struct bt_le_scan_param scan_param = {
		.type     = BT_LE_SCAN_TYPE_ACTIVE,
		.interval = BT_GAP_SCAN_FAST_INTERVAL,
		.window   = BT_GAP_SCAN_FAST_WINDOW,
		.options  = BT_LE_SCAN_OPT_CODED | BT_LE_SCAN_OPT_NO_1M
	};

	struct bt_scan_init_param scan_init = {
		.connect_if_match = 0,
		.scan_param = &scan_param,
		.conn_param = NULL
	};

	bt_scan_init(&scan_init);
	bt_scan_cb_register(&scan_cb);

	err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, BT_UUID_HRS);
	if (err) {
		printk("Scanning filters cannot be set (err %d)\n", err);

		return;
	}

	err = bt_scan_filter_enable(BT_SCAN_UUID_FILTER, false);
	if (err) {
		printk("Filters cannot be turned on (err %d)\n", err);
	}
}

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	int err;
	struct bt_conn_info info;
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		printk("Failed to connect to %s (%u)\n", addr, conn_err);

		bt_conn_unref(default_conn);
		default_conn = NULL;

		err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
		if (err) {
			printk("Scanning failed to start (err %d)\n", err);
		}

		return;
	}

	err = bt_conn_get_info(conn, &info);

	if (err) {
		printk("Failed to get connection info\n");
	} else {
		const struct bt_conn_le_phy_info *phy_info;

		phy_info = info.le.phy;
		printk("Connected: %s, tx_phy %u, rx_phy %u\n",
				addr, phy_info->tx_phy, phy_info->rx_phy);
	}

	if (conn == default_conn) {

		err = bt_gatt_dm_start(conn, BT_UUID_HRS, &discover_hrs_cb, NULL);
		if (err) {
			printk("Failed to start discovery (err %d)\n", err);
		}
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	int err;
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s (reason 0x%02x)\n", addr, reason);

	if (default_conn != conn) {
		return;
	}

	bt_conn_unref(default_conn);
	default_conn = NULL;

	err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
	if (err) {
		printk("Scanning failed to start (err %d)\n", err);
	}
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

void main(void)
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	bt_conn_cb_register(&conn_callbacks);

	scan_init();

	err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
	if (err) {
		printk("Scanning failed to start (err %d)\n", err);
		return;
	}

	printk("Scanning successfully started\n");
}
