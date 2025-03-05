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
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <bluetooth/gatt_dm.h>
#include <bluetooth/scan.h>
#include <bluetooth/services/hrs_client.h>

static struct bt_conn *default_conn;

static struct bt_hrs_client hrs_c;

static const char *phy_to_str(uint8_t phy)
{
	switch (phy) {
	case BT_GAP_LE_PHY_NONE:
		return "No packets";
	case BT_GAP_LE_PHY_1M:
		return "LE 1M";
	case BT_GAP_LE_PHY_2M:
		return "LE 2M";
	case BT_GAP_LE_PHY_CODED:
		return "LE Coded";
	case BT_GAP_LE_PHY_CODED_S8:
		return "S=8 Coded";
	case BT_GAP_LE_PHY_CODED_S2:
		return "S=2 Coded";
	default: return "Unknown";
	}
}

static void notify_func(struct bt_hrs_client *hrs_c,
			const struct bt_hrs_client_measurement *meas,
			int err)
{
	if (err) {
		printk("Error during receiving Heart Rate Measurement notification, err: %d\n",
		       err);
		return;
	}

	printk("Heart Rate Measurement notification received:\n\n");
	printk("\tHeart Rate Measurement Value Format: %s\n",
		meas->flags.value_format ? "16 - bit" : "8 - bit");
	printk("\tSensor Contact detected: %d\n", meas->flags.sensor_contact_detected);
	printk("\tSensor Contact supported: %d\n", meas->flags.sensor_contact_supported);
	printk("\tEnergy Expended present: %d\n", meas->flags.energy_expended_present);
	printk("\tRR-Intervals present: %d\n", meas->flags.rr_intervals_present);

	printk("\n\tHeart Rate Measurement Value: %d bpm\n", meas->hr_value);

	if (meas->flags.energy_expended_present) {
		printk("\n\tEnergy Expended: %d J\n", meas->energy_expended);
	}

	if (meas->flags.rr_intervals_present) {
		printk("\t RR-intervals: ");
		for (size_t i = 0; i < meas->rr_intervals_count; i++) {
			printk("%d ", meas->rr_intervals[i]);
		}
	}

	printk("\n");
}

static void discover_hrs_completed(struct bt_gatt_dm *dm, void *ctx)
{
	int err;

	printk("The discovery procedure succeeded\n");

	bt_gatt_dm_data_print(dm);

	err = bt_hrs_client_handles_assign(dm, &hrs_c);
	if (err) {
		printk("Could not init HRS client object (err %d)\n", err);
		return;
	}

	err = bt_hrs_client_measurement_subscribe(&hrs_c, notify_func);
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

	uint8_t prim_phy = device_info->recv_info->primary_phy;
	uint8_t sec_phy = device_info->recv_info->secondary_phy;

	printk("Filters matched. Address: %s connectable: %s Primary PHY: %s Secondary PHY %s\n",
		addr, connectable ? "yes" : "no", phy_to_str(prim_phy), phy_to_str(sec_phy));

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
		printk("Failed to connect to %s, 0x%02x %s\n", addr, conn_err,
		       bt_hci_err_to_str(conn_err));

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
		printk("Failed to get connection info (err %d)\n", err);
	} else {
		const struct bt_conn_le_phy_info *phy_info;

		phy_info = info.le.phy;
		printk("Connected: %s, tx_phy %s, rx_phy %s\n",
				addr, phy_to_str(phy_info->tx_phy), phy_to_str(phy_info->rx_phy));
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

	printk("Disconnected: %s, reason 0x%02x %s\n", addr, reason, bt_hci_err_to_str(reason));

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

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

int main(void)
{
	int err;

	printk("Starting Bluetooth Central HR coded sample\n");

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	printk("Bluetooth initialized\n");

	err = bt_hrs_client_init(&hrs_c);
	if (err) {
		printk("Heart Rate Service client failed to init (err %d)\n", err);
		return 0;
	}

	scan_init();

	err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
	if (err) {
		printk("Scanning failed to start (err %d)\n", err);
		return 0;
	}

	printk("Scanning successfully started\n");
	return 0;
}
