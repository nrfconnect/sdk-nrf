/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>

#include <bluetooth/gatt.h>
#include <bluetooth/uuid.h>
#include <bluetooth/common/gatt_dm.h>
#include <bluetooth/common/scan.h>

#include <dk_buttons_and_leds.h>
#include <misc/byteorder.h>

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

static void discovery_completed(struct bt_conn *conn,
				   const struct bt_gatt_attr *attrs,
				   size_t attrs_len)
{
	int err;
	static struct bt_gatt_subscribe_params param = {
		.notify = on_received,
		.value = BT_GATT_CCC_NOTIFY,
	};

	printk("GATT discovery completed\n");

	for (size_t i = 0; i < attrs_len - 1; i++) {
		if (!bt_uuid_cmp(attrs[i].uuid, BT_UUID_TOC)) {
			param.value_handle = attrs[i].handle;
			if (!bt_uuid_cmp(attrs[i + 1].uuid, BT_UUID_GATT_CCC)) {
				param.ccc_handle = attrs[i + 1].handle;
				break;
			}
		}
	}

	if (param.value_handle == 0 && param.ccc_handle == 0) {
		printk("Could not find char handles.\n");
		goto release;
	}

	err = bt_gatt_subscribe(conn, &param);
	if (err) {
		printk("Subscribe failed (err %d)", err);
	}

release:
	err = bt_gatt_dm_data_release();
	if (err) {
		printk("Could not release discovery data, err: %d\n", err);
	}
}

static void discovery_service_not_found(struct bt_conn *conn)
{
	printk("Thing orientation service not found!\n");
}

static void discovery_error_found(struct bt_conn *conn, int err)
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

	err = bt_gatt_dm_start(conn, BT_UUID_TMS, &discovery_cb);
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

static struct bt_scan_cb scan_cb = {
	.filter_match = scan_filter_match,
	.connecting_error = scan_connecting_error,
};

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

void ble_init(void)
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	bt_conn_cb_register(&conn_callbacks);
	scan_start();
}
