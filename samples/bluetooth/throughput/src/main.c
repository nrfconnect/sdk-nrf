/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <kernel.h>
#include <console.h>
#include <misc/printk.h>
#include <string.h>
#include <zephyr/types.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/hci.h>
#include <bluetooth/uuid.h>
#include <bluetooth/services/throughput.h>
#include <bluetooth/scan.h>
#include <bluetooth/gatt_dm.h>

#define DEVICE_NAME	CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)
#define INTERVAL_MIN	0x140	/* 320 units, 400 ms */
#define INTERVAL_MAX	0x140	/* 320 units, 400 ms */

static volatile bool test_ready;
static struct bt_conn *default_conn;
static struct bt_gatt_throughput gatt_throughput;
static struct bt_uuid *uuid128 = BT_UUID_THROUGHPUT;
static struct bt_gatt_exchange_params exchange_params;
static struct bt_le_conn_param *conn_param =
	BT_LE_CONN_PARAM(INTERVAL_MIN, INTERVAL_MAX, 0, 400);

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL,
		0xBB, 0x4A, 0xFF, 0x4F, 0xAD, 0x03, 0x41, 0x5D,
		0xA9, 0x6C, 0x9D, 0x6C, 0xDD, 0xDA, 0x83, 0x04),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static const char img[][81] = {
#include "img.file"
};

void scan_filter_match(struct bt_scan_device_info *device_info,
		       struct bt_scan_filter_match *filter_match,
		       bool connectable)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(device_info->addr, addr, sizeof(addr));

	printk("Filters matched. Address: %s connectable: %d\n",
		addr, connectable);
}

void scan_filter_no_match(struct bt_scan_device_info *device_info,
			  bool connectable)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(device_info->addr, addr, sizeof(addr));

	printk("Filter not match. Address: %s connectable: %d\n",
				addr, connectable);
}

void scan_connecting_error(struct bt_scan_device_info *device_info)
{
	printk("Connecting failed\n");
}

BT_SCAN_CB_INIT(scan_cb, scan_filter_match, scan_filter_no_match,
		scan_connecting_error, NULL);

static void exchange_func(struct bt_conn *conn, u8_t err,
			  struct bt_gatt_exchange_params *params)
{
	struct bt_conn_info info = {0};

	printk("MTU exchange %s\n", err == 0 ? "successful" : "failed");

	err = bt_conn_get_info(conn, &info);
	if (info.role == BT_CONN_ROLE_MASTER) {
		test_ready = true;
	}
}

static void discovery_complete(struct bt_gatt_dm *dm,
			       void *context)
{
	int err;
	struct bt_gatt_throughput *throughput = context;

	printk("Service discovery completed\n");

	bt_gatt_dm_data_print(dm);
	bt_gatt_throughput_handles_assign(dm, throughput);
	bt_gatt_dm_data_release(dm);

	exchange_params.func = exchange_func;

	err = bt_gatt_exchange_mtu(default_conn, &exchange_params);
	if (err) {
		printk("MTU exchange failed (err %d)\n", err);
	} else {
		printk("MTU exchange pending\n");
	}
}

static void discovery_service_not_found(struct bt_conn *conn,
					void *context)
{
	printk("Service not found\n");
}

static void discovery_error(struct bt_conn *conn,
			    int err,
			    void *context)
{
	printk("Error while discovering GATT database: (%d)\n", err);
}

struct bt_gatt_dm_cb discovery_cb = {
	.completed         = discovery_complete,
	.service_not_found = discovery_service_not_found,
	.error_found       = discovery_error,
};

static void connected(struct bt_conn *conn, u8_t err)
{
	struct bt_conn_info info = {0};

	if (err) {
		printk("Connection failed (err %u)\n", err);
		return;
	}

	default_conn = bt_conn_ref(conn);
	err = bt_conn_get_info(default_conn, &info);
	if (err) {
		printk("Error %u while getting bt conn info\n", err);
	}

	printk("Connected as %s\n",
	       info.role == BT_CONN_ROLE_MASTER ? "master" : "slave");
	printk("Conn. interval is %u units\n", info.le.interval);

	/* make sure we're not scanning or advertising */
	bt_le_adv_stop();
	bt_scan_stop();

	if (info.role == BT_CONN_ROLE_MASTER) {
		err = bt_gatt_dm_start(default_conn,
				       BT_UUID_THROUGHPUT,
				       &discovery_cb,
				       &gatt_throughput);

		if (err) {
			printk("Discover failed (err %d)\n", err);
		}
	}
}

static void scan_init(void)
{
	int err;
	struct bt_le_scan_param scan_param = {
	    .type = BT_HCI_LE_SCAN_PASSIVE,
	    .filter_dup = BT_HCI_LE_SCAN_FILTER_DUP_ENABLE,
	    .interval = 0x0010,
	    .window = 0x0010,
	};

	struct bt_scan_init_param scan_init = {
		.connect_if_match = 1,
		.scan_param = &scan_param,
		.conn_param = conn_param
	};

	bt_scan_init(&scan_init);
	bt_scan_cb_register(&scan_cb);

	err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, uuid128);
	if (err) {
		printk("Scanning filters cannot be set\n");

		return;
	}

	err = bt_scan_filter_enable(BT_SCAN_UUID_FILTER, false);
	if (err) {
		printk("Filters cannot be turned on\n");
	}
}

static void advertise_and_scan(void)
{
	int err;

	err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), sd,
			     ARRAY_SIZE(sd));
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");

	err = bt_scan_start(BT_SCAN_TYPE_SCAN_PASSIVE);
	if (err) {
		printk("Starting scanning failed (err %d)\n", err);
	}

	printk("Scanning successfully started\n");
}

static void disconnected(struct bt_conn *conn, u8_t reason)
{
	printk("Disconnected (reason %u)\n", reason);

	test_ready = false;

	if (default_conn) {
		bt_conn_unref(default_conn);
		default_conn = NULL;
	}

	advertise_and_scan();
}

static u8_t throughput_read(const struct bt_gatt_throughput_metrics *met)
{
	printk("[peer] received %u bytes (%u KB)"
	       " in %u GATT writes at %u bps\n",
	       met->write_len, met->write_len / 1024, met->write_count,
	       met->write_rate);

	test_ready = true;

	return BT_GATT_ITER_STOP;
}

static void throughput_received(const struct bt_gatt_throughput_metrics *met)
{
	static u32_t kb;

	if (met->write_len == 0) {
		kb = 0;
		printk("\n");

		return;
	}

	if ((met->write_len / 1024) != kb) {
		kb = (met->write_len / 1024);
		printk("=");
	}
}

static void throughput_send(const struct bt_gatt_throughput_metrics *met)
{
	printk("\n[local] received %u bytes (%u KB)"
		" in %u GATT writes at %u bps\n",
		met->write_len, met->write_len / 1024,
		met->write_count, met->write_rate);
}

static const struct bt_gatt_throughput_cb throughput_cb = {
	.data_read = throughput_read,
	.data_received = throughput_received,
	.data_send = throughput_send
};

static void bt_ready(int err)
{
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	scan_init();
	bt_gatt_throughput_init(&gatt_throughput, &throughput_cb);
	if (err) {
		printk("Throughput service initialization failed.\n");
		return;
	}

	advertise_and_scan();
}

static void test_run(void)
{
	int err;
	u64_t stamp;
	u32_t delta;
	u32_t data = 0;
	u32_t prog = 0;

	/* a dummy data buffer */
	static char dummy[256];


	/* wait for user input to continue */
	printk("Ready, press any key to start\n");
	console_getchar();

	if (!test_ready) {
		/* disconnected while blocking inside _getchar() */
		return;
	}

	test_ready = false;

	/* reset peer metrics */
	err = bt_gatt_throughput_write(&gatt_throughput, dummy, 1);
	if (err) {
		printk("Reset peer metrics failed.\n");
		return;
	}

	/* get cycle stamp */
	stamp = k_uptime_get_32();

	while (prog < IMG_SIZE) {
		err = bt_gatt_throughput_write(&gatt_throughput, dummy, 244);
		if (err) {
			printk("GATT write failed (err %d)\n", err);
			break;
		}

		/* print graphics */
		printk("%c", img[prog / IMG_X][prog % IMG_X]);
		data += 244;
		prog++;
	}

	delta = k_uptime_delta_32(&stamp);

	printk("\nDone\n");
	printk("[local] sent %u bytes (%u KB) in %u ms at %llu kbps\n",
	       data, data / 1024, delta, ((u64_t)data * 8 / delta));

	/* read back char from peer */
	err = bt_gatt_throughput_read(&gatt_throughput);
	if (err) {
		printk("GATT read failed (err %d)\n", err);
	}
}

static bool le_param_req(struct bt_conn *conn, struct bt_le_conn_param *param)
{
	/* reject peer conn param request */
	return false;
}

void main(void)
{
	int err;

	static struct bt_conn_cb conn_callbacks = {
	    .connected = connected,
	    .disconnected = disconnected,
	    .le_param_req = le_param_req,
	};

	console_init();

	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	bt_conn_cb_register(&conn_callbacks);

	for (;;) {
		if (test_ready) {
			test_run();
		}
	}
}
