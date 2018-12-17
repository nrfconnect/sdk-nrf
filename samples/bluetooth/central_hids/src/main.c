/* main.c - Application main entry point */

/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr.h>
#include <misc/printk.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/common/gatt_dm.h>
#include <misc/byteorder.h>
#include <bluetooth/common/scan.h>
#include <bluetooth/services/hids_c.h>

static struct bt_conn *default_conn;
static struct bt_gatt_hids_c hids_c;

static void scan_filter_match(struct bt_scan_device_info *device_info,
			      struct bt_scan_filter_match *filter_match,
			      bool connectable)
{
	int err;
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(device_info->addr, addr, sizeof(addr));

	printk("Filters matched. Address: %s connectable: %s\n",
		addr, connectable ? "yes" : "no");

	err = bt_scan_stop();
	if (err) {
		printk("Stop LE scan failed (err %d)\n", err);
	}
}

static void scan_connecting_error(struct bt_scan_device_info *device_info)
{
	printk("Connecting failed\n");
}

static void scan_connecting(struct bt_scan_device_info *device_info,
			    struct bt_conn *conn)
{
	default_conn = bt_conn_ref(conn);
}

static struct bt_scan_cb scan_cb = {
	.filter_match = scan_filter_match,
	.connecting_error = scan_connecting_error,
	.connecting = scan_connecting
};

static void discovery_completed_cb(struct bt_gatt_dm *dm,
				   void *context)
{
	int err;

	printk("The discovery procedure succeeded\n");

	bt_gatt_dm_data_print(dm);

	err = bt_gatt_hids_c_handles_assign(dm, &hids_c);
	if (err) {
		printk("Could not init HIDS client object, error: %d\n", err);
	}

	err = bt_gatt_dm_data_release(dm);
	if (err) {
		printk("Could not release the discovery data, error "
		       "code: %d\n", err);
	}
}

static void discovery_service_not_found_cb(struct bt_conn *conn,
					   void *context)
{
	printk("The service could not be found during the discovery\n");
}

static void discovery_error_found_cb(struct bt_conn *conn,
				     int err,
				     void *context)
{
	printk("The discovery procedure failed with %d\n", err);
}

static struct bt_gatt_dm_cb discovery_cb = {
	.completed = discovery_completed_cb,
	.service_not_found = discovery_service_not_found_cb,
	.error_found = discovery_error_found_cb,
};

static void connected(struct bt_conn *conn, u8_t conn_err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		printk("Failed to connect to %s (%u)\n", addr, conn_err);
		return;
	}

	printk("Connected: %s\n", addr);

	if (bt_conn_security(conn, BT_SECURITY_MEDIUM)) {
		printk("Failed to set security\n");
	}

	if (conn == default_conn) {
		int err = bt_gatt_dm_start(conn,
					   BT_UUID_HIDS,
					   &discovery_cb,
					   NULL);
		if (err) {
			printk("Could not start the discovery procedure, error "
			       "code: %d\n", err);
		}
	}
}

static void disconnected(struct bt_conn *conn, u8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int err;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s (reason %u)\n", addr, reason);

	if (bt_gatt_hids_c_assign_check(&hids_c)) {
		printk("HIDS client active - releasing");
		bt_gatt_hids_c_release(&hids_c);
	}

	if (default_conn != conn) {
		return;
	}

	bt_conn_unref(default_conn);
	default_conn = NULL;

	/* This demo doesn't require active scan */
	err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
	if (err) {
		printk("Scanning failed to start (err %d)\n", err);
	}
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

static void scan_init(void)
{
	int err;

	struct bt_scan_init_param scan_init = {
		.connect_if_match = 1,
		.scan_param = NULL,
		.conn_param = BT_LE_CONN_PARAM_DEFAULT
	};

	bt_scan_init(&scan_init);
	bt_scan_cb_register(&scan_cb);

	err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, BT_UUID_HIDS);
	if (err) {
		printk("Scanning filters cannot be set (err %d)\n", err);

		return;
	}

	err = bt_scan_filter_enable(BT_SCAN_UUID_FILTER, false);
	if (err) {
		printk("Filters cannot be turned on (err %d)\n", err);
	}
}

static u8_t hids_c_notify_cb(struct bt_gatt_hids_c *hids_c,
			     struct bt_gatt_hids_c_rep_info *rep,
			     u8_t err,
			     const u8_t *data)
{
	u8_t size = rep->size;
	u8_t i;

	if (!data) {
		return BT_GATT_ITER_STOP;
	}
	printk("Notification, id: %u, size: %u, data:",
	       rep->ref.id,
	       size);
	for (i = 0; i < size; ++i) {
		printk(" 0x%x", data[i]);
	}
	printk("\n");
	return BT_GATT_ITER_CONTINUE;
}

static void hids_c_ready_cb(struct bt_gatt_hids_c *hids_c)
{
	u8_t i;

	printk("HIDS is ready to work\n");
	for (i = 0; i < hids_c->rep_cnt; ++i) {
		struct bt_gatt_hids_c_rep_info *rep;

		rep = hids_c->rep_info[i];
		if (rep->ref.type ==
		    BT_GATT_HIDS_C_REPORT_TYPE_INPUT) {
			printk("Subscribe in report id: %u\n",
			       rep->ref.id);
			bt_gatt_hids_c_rep_subscribe(hids_c, rep,
						     hids_c_notify_cb);
		}
	}
}

static void hids_c_prep_fail_cb(struct bt_gatt_hids_c *hids_c, int err)
{
	printk("ERROR: HIDS client preparation failed!\n");
}

static void hids_c_pm_update_cb(struct bt_gatt_hids_c *hids_c)
{
	printk("Protocol mode updated: %s\n",
	      bt_gatt_hids_c_pm_get(hids_c) == BT_GATT_HIDS_C_PM_BOOT ?
	      "BOOT" : "REPORT");
}

/* HIDS client initialization parameters */
static const struct bt_gatt_hids_c_init_params hids_c_init_params = {
	.ready_cb      = hids_c_ready_cb,
	.prep_error_cb = hids_c_prep_fail_cb,
	.pm_update_cb  = hids_c_pm_update_cb
};

void main(void)
{
	int err;

	bt_gatt_hids_c_init(&hids_c, &hids_c_init_params);

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	scan_init();
	bt_conn_cb_register(&conn_callbacks);

	err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
	if (err) {
		printk("Scanning failed to start (err %d)\n", err);
		return;
	}

	printk("Scanning successfully started\n");
}
