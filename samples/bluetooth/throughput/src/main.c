/** @file
 *  @brief Throughput service sample
 */

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

#include <bluetooth/common/scan.h>

#define DEVICE_NAME	CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)
#define INTERVAL_MIN	0x140	/* 320 units, 400 ms */
#define INTERVAL_MAX	0x140	/* 320 units, 400 ms */

static u16_t char_handle;
static volatile bool test_ready;
static struct bt_conn *default_conn;
static struct bt_uuid *uuid16 = BT_UUID_THROUGHPUT_CHAR;
static struct bt_uuid *uuid128 = BT_UUID_THROUGHPUT;
static struct bt_gatt_read_params read_param;
static struct bt_gatt_exchange_params exchange_params;
static struct bt_gatt_discover_params discover_params;
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

static struct bt_scan_cb scan_cb = {
	.filter_match = scan_filter_match,
	.filter_no_match = scan_filter_no_match,
	.connecting_error = scan_connecting_error,
};

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

static u8_t discover_func(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			  struct bt_gatt_discover_params *params)
{
	int err;

	if (!attr) {
		printk("Discover complete\n");
		memset(params, 0, sizeof(*params));
		return BT_GATT_ITER_STOP;
	}

	if (!bt_uuid_cmp(discover_params.uuid, uuid128)) {
		/* service found, discover characteristic */
		discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
		discover_params.uuid = &BT_UUID_16(uuid16)->uuid;
		discover_params.start_handle = attr->handle + 1;

		err = bt_gatt_discover(conn, &discover_params);
		if (err) {
			printk("Discover failed (err %d)\n", err);
		}
	} else if (!bt_uuid_cmp(discover_params.uuid, uuid16)) {
		/* characteristic found */
		char_handle = attr->handle + 1;
		exchange_params.func = exchange_func;

		err = bt_gatt_exchange_mtu(conn, &exchange_params);
		if (err) {
			printk("MTU exchange failed (err %d)\n", err);
		} else {
			printk("MTU exchange pending\n");
		}
	}

	return BT_GATT_ITER_STOP;
}

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

	discover_params.type = BT_GATT_DISCOVER_PRIMARY;
	discover_params.uuid = &BT_UUID_128(uuid128)->uuid;
	discover_params.func = discover_func;
	discover_params.start_handle = 0x0001;
	discover_params.end_handle = 0xffff;

	if (info.role == BT_CONN_ROLE_MASTER) {
		err = bt_gatt_discover(default_conn, &discover_params);
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

	struct bt_scan_init scan_init = {
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

static void bt_ready(int err)
{
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	scan_init();
	throughput_init();
	advertise_and_scan();
}

static u8_t read_fn(struct bt_conn *conn, u8_t err,
		    struct bt_gatt_read_params *params, const void *data,
		    u16_t len)
{
	struct metrics met = {0};

	if (err) {
		printk("GATT read callback failed (err: %d)\n", err);
		return 0;
	}

	if (data) {
		len = min(len, sizeof(met));
		memcpy(&met, data, len);

		printk("[peer] received %u bytes (%u KB)"
		       " in %u GATT writes at %u bps\n",
		       met.write_len, met.write_len / 1024, met.write_count,
		       met.write_rate);

		test_ready = true;
	}

	return BT_GATT_ITER_STOP;
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
	err = bt_gatt_write_without_response(
		default_conn, char_handle, dummy, 1, false);

	/* get cycle stamp */
	stamp = k_uptime_get_32();

	while (prog < IMG_SIZE) {
		err = bt_gatt_write_without_response(
			default_conn, char_handle, dummy, 244, false);
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
	read_param.single.handle = char_handle;

	err = bt_gatt_read(default_conn, &read_param);
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

	read_param.func = read_fn;
	read_param.handle_count = 1;
	read_param.single.offset = 0;

	for (;;) {
		if (test_ready) {
			test_run();
		}
	}
}
