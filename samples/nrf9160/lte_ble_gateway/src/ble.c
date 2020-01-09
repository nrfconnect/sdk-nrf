/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>

#include <bluetooth/gatt.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt_dm.h>
#include <bluetooth/scan.h>

#include <dk_buttons_and_leds.h>
#include <sys/byteorder.h>

#include <nrf_cloud.h>
#include "aggregator.h"

/* Thinghy advertisement UUID */
#define BT_UUID_THINGY                                                         \
	BT_UUID_DECLARE_128(0x42, 0x00, 0x74, 0xA9, 0xFF, 0x52, 0x10, 0x9B,    \
			    0x33, 0x49, 0x35, 0x9B, 0x00, 0x01, 0x68, 0xEF)

/* Thingy service UUID */
#define BT_UUID_TMS                                                            \
	BT_UUID_DECLARE_128(0x42, 0x00, 0x74, 0xA9, 0xFF, 0x52, 0x10, 0x9B,    \
			    0x33, 0x49, 0x35, 0x9B, 0x00, 0x04, 0x68, 0xEF)

/* Thingy characteristic UUID */
#define BT_UUID_TOC                                                            \
	BT_UUID_DECLARE_128(0x42, 0x00, 0x74, 0xA9, 0xFF, 0x52, 0x10, 0x9B,    \
			    0x33, 0x49, 0x35, 0x9B, 0x03, 0x04, 0x68, 0xEF)

extern void alarm(void);

static u8_t on_received(struct bt_conn *conn,
			struct bt_gatt_subscribe_params *params,
			const void *data, u16_t length)
{
	if (length > 0) {
		printk("Orientation: %x\n", ((u8_t *)data)[0]);
		struct sensor_data in_data;

		in_data.type = THINGY_ORIENTATION;
		in_data.length = 1;
		in_data.data[0] = ((u8_t *)data)[0];

		if (aggregator_put(in_data) != 0) {
			printk("Was not able to insert Thingy orientation data into aggregator.\n");
		}
		/* If the thingy is upside down, trigger an alarm. */
		if (((u8_t *)data)[0] == 3) {
			alarm();
		}

	} else {
		printk("Orientation notification with 0 length\n");
	}
	return BT_GATT_ITER_CONTINUE;
}

static void discovery_completed(struct bt_gatt_dm *disc, void *ctx)
{
	int err;

	/* Must be statically allocated */
	static struct bt_gatt_subscribe_params param = {
		.notify = on_received,
		.value = BT_GATT_CCC_NOTIFY,
	};

	const struct bt_gatt_attr *chrc;
	const struct bt_gatt_attr *desc;

	chrc = bt_gatt_dm_char_by_uuid(disc, BT_UUID_TOC);
	if (!chrc) {
		printk("Missing Thingy orientation characteristic\n");
		goto release;
	}

	desc = bt_gatt_dm_desc_by_uuid(disc, chrc, BT_UUID_TOC);
	if (!desc) {
		printk("Missing Thingy orientation char value descriptor\n");
		goto release;
	}

	param.value_handle = desc->handle,

	desc = bt_gatt_dm_desc_by_uuid(disc, chrc, BT_UUID_GATT_CCC);
	if (!desc) {
		printk("Missing Thingy orientation char CCC descriptor\n");
		goto release;
	}

	param.ccc_handle = desc->handle;

	err = bt_gatt_subscribe(bt_gatt_dm_conn_get(disc), &param);
	if (err) {
		printk("Subscribe failed (err %d)\n", err);
	}

release:
	err = bt_gatt_dm_data_release(disc);
	if (err) {
		printk("Could not release discovery data, err: %d\n", err);
	}
}

static void discovery_service_not_found(struct bt_conn *conn, void *ctx)
{
	printk("Thingy orientation service not found!\n");
}

static void discovery_error_found(struct bt_conn *conn, int err, void *ctx)
{
	printk("The discovery procedure failed, err %d\n", err);
}

static struct bt_gatt_dm_cb discovery_cb = {
	.completed = discovery_completed,
	.service_not_found = discovery_service_not_found,
	.error_found = discovery_error_found,
};

static void connected(struct bt_conn *conn, u8_t conn_err)
{
	int err;
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		printk("Failed to connect to %s (%u)\n", addr, conn_err);
		return;
	}

	printk("Connected: %s\n", addr);

	err = bt_gatt_dm_start(conn, BT_UUID_TMS, &discovery_cb, NULL);
	if (err) {
		printk("Could not start service discovery, err %d\n", err);
	}
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
};

void scan_filter_match(struct bt_scan_device_info *device_info,
		       struct bt_scan_filter_match *filter_match,
		       bool connectable)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(device_info->addr, addr, sizeof(addr));

	printk("Device found: %s\n", addr);
}

void scan_connecting_error(struct bt_scan_device_info *device_info)
{
	printk("Connection to peer failed!\n");
}

BT_SCAN_CB_INIT(scan_cb, scan_filter_match, NULL, scan_connecting_error, NULL);

static void scan_start(void)
{
	int err;

	struct bt_le_scan_param scan_param = {
		.type = BT_HCI_LE_SCAN_ACTIVE,
		.filter_dup = BT_HCI_LE_SCAN_FILTER_DUP_ENABLE,
		.interval = 0x0010,
		.window = 0x0010,
	};

	struct bt_scan_init_param scan_init = {
		.connect_if_match = 1,
		.scan_param = &scan_param,
		.conn_param = BT_LE_CONN_PARAM_DEFAULT,
	};

	bt_scan_init(&scan_init);
	bt_scan_cb_register(&scan_cb);

	err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, BT_UUID_THINGY);
	if (err) {
		printk("Scanning filters cannot be set\n");
		return;
	}

	err = bt_scan_filter_enable(BT_SCAN_UUID_FILTER, false);
	if (err) {
		printk("Filters cannot be turned on\n");
	}

	err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
	if (err) {
		printk("Scanning failed to start, err %d\n", err);
	}

	printk("Scanning...\n");
}

static void ble_ready(int err)
{
	printk("Bluetooth ready\n");

	bt_conn_cb_register(&conn_callbacks);
	scan_start();
}

void ble_init(void)
{
	int err;

	printk("Initializing Bluetooth..\n");
	err = bt_enable(ble_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}
}
