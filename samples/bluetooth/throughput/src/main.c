/** @file
 *  @brief Throughput service sample
 */

/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
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

static void exchange_func(struct bt_conn *conn, u8_t rc,
			  struct bt_gatt_exchange_params *params)
{
	struct bt_conn_info info = {0};

	printk("MTU exchange %s\n", rc == 0 ? "successful" : "failed");

	rc = bt_conn_get_info(conn, &info);
	if (info.role == BT_CONN_ROLE_MASTER) {
		test_ready = true;
	}
}

static u8_t discover_func(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			  struct bt_gatt_discover_params *params)
{
	int rc;

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

		rc = bt_gatt_discover(conn, &discover_params);
		if (rc) {
			printk("Discover failed (err %d)\n", rc);
		}
	} else if (!bt_uuid_cmp(discover_params.uuid, uuid16)) {
		/* characteristic found */
		char_handle = attr->handle + 1;
		exchange_params.func = exchange_func;

		rc = bt_gatt_exchange_mtu(conn, &exchange_params);
		if (rc) {
			printk("MTU exchange failed (err %d)\n", rc);
		} else {
			printk("MTU exchange pending\n");
		}
	}

	return BT_GATT_ITER_STOP;
}

static bool eir_found(u8_t type, const u8_t *data, u8_t data_len,
		      void *user_data)
{
	int rc;
	char dev[BT_ADDR_LE_STR_LEN];
	struct bt_uuid_128 uuid = {0};

	bt_addr_le_t *addr = user_data;

	if ((type != BT_DATA_UUID128_ALL && type != BT_DATA_UUID128_SOME) ||
	    (data_len % 16)) {
		/* only process well formed 128 bit UUIDs */
		return true;
	}

	for (size_t i = 0; i < data_len; i += 16) {

		/* copy the UUID to a local buffer */
		memset(&uuid, 0x00, sizeof(uuid));
		uuid.uuid.type = BT_UUID_TYPE_128;
		memcpy(uuid.val, &data[i], sizeof(uuid.val));

		/* format a human readable address to display */
		bt_addr_le_to_str(addr, dev, sizeof(dev));

		if (!bt_uuid_cmp((struct bt_uuid *)&uuid, uuid128)) {

			/* stop scanning before creating a BLE connection */
			printk("Found a peer device %s\n", dev);

			rc = bt_le_scan_stop();
			if (rc) {
				printk("Stop LE scan failed (err %d)\n", rc);
				return false;
			}

			default_conn = bt_conn_create_le(addr, conn_param);
			if (!default_conn) {
				printk("Error while creating connection\n");
			}

			return false;
		}

		printk("Skipping device %s\n", dev);
	}

	return true;
}

static void ad_parse(struct net_buf_simple *ad,
		     bool (*func)(u8_t type, const u8_t *data, u8_t data_len,
				  void *user_data),
		     void *le_addr)
{
	while (ad->len > 1) {
		u8_t type;
		u8_t len = net_buf_simple_pull_u8(ad);

		/* check for early termination */
		if (len == 0) {
			return;
		}

		if (len > ad->len) {
			printk("Malformed advertising data\n");
			return;
		}

		type = net_buf_simple_pull_u8(ad);

		if (!func(type, ad->data, len - 1, le_addr)) {
			return;
		}

		net_buf_simple_pull(ad, len - 1);
	}
}

static void device_found(const bt_addr_le_t *addr, s8_t rssi, u8_t type,
			 struct net_buf_simple *ad)
{
	/* we're only interested in connectable events */
	if (type == BT_LE_ADV_IND || type == BT_LE_ADV_DIRECT_IND) {
		ad_parse(ad, eir_found, (void *)addr);
	}
}

static void connected(struct bt_conn *conn, u8_t rc)
{
	struct bt_conn_info info = {0};

	if (rc) {
		printk("Connection failed (err %u)\n", rc);
		return;
	}

	default_conn = bt_conn_ref(conn);
	rc = bt_conn_get_info(default_conn, &info);
	if (rc) {
		printk("Error %u while getting bt conn info\n", rc);
	}

	printk("Connected as %s\n",
	       info.role == BT_CONN_ROLE_MASTER ? "master" : "slave");
	printk("Conn. interval is %u units\n", info.le.interval);

	/* make sure we're not scanning or advertising */
	bt_le_adv_stop();
	bt_le_scan_stop();

	discover_params.type = BT_GATT_DISCOVER_PRIMARY;
	discover_params.uuid = &BT_UUID_128(uuid128)->uuid;
	discover_params.func = discover_func;
	discover_params.start_handle = 0x0001;
	discover_params.end_handle = 0xffff;

	if (info.role == BT_CONN_ROLE_MASTER) {
		rc = bt_gatt_discover(default_conn, &discover_params);
		if (rc) {
			printk("Discover failed (err %d)\n", rc);
		}
	}
}

static void advertise_and_scan(void)
{
	int rc;
	struct bt_le_scan_param scan_param = {
	    .type = BT_HCI_LE_SCAN_PASSIVE,
	    .filter_dup = BT_HCI_LE_SCAN_FILTER_DUP_ENABLE,
	    .interval = 0x0010,
	    .window = 0x0010,
	};

	rc = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), sd,
			     ARRAY_SIZE(sd));
	if (rc) {
		printk("Advertising failed to start (err %d)\n", rc);
		return;
	}

	printk("Advertising successfully started\n");

	rc = bt_le_scan_start(&scan_param, device_found);
	if (rc) {
		printk("Starting scanning failed (err %d)\n", rc);
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

static void bt_ready(int rc)
{
	if (rc) {
		printk("Bluetooth init failed (err %d)\n", rc);
		return;
	}

	printk("Bluetooth initialized\n");

	throughput_init();
	advertise_and_scan();
}

static u8_t read_fn(struct bt_conn *conn, u8_t rc,
		    struct bt_gatt_read_params *params, const void *data,
		    u16_t len)
{
	struct metrics met = {0};

	if (rc) {
		printk("GATT read callback failed (err: %d)\n", rc);
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
	int rc;
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
	rc = bt_gatt_write_without_response(
		default_conn, char_handle, dummy, 1, false);

	/* get cycle stamp */
	stamp = k_uptime_get_32();

	while (prog < IMG_SIZE) {
		rc = bt_gatt_write_without_response(
			default_conn, char_handle, dummy, 244, false);
		if (rc) {
			printk("GATT write failed (err %d)\n", rc);
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

	rc = bt_gatt_read(default_conn, &read_param);
	if (rc) {
		printk("GATT read failed (err %d)\n", rc);
	}
}

static bool le_param_req(struct bt_conn *conn, struct bt_le_conn_param *param)
{
	/* reject peer conn param request */
	return false;
}

void main(void)
{
	int rc;

	static struct bt_conn_cb conn_callbacks = {
	    .connected = connected,
	    .disconnected = disconnected,
	    .le_param_req = le_param_req,
	};

	console_init();

	rc = bt_enable(bt_ready);
	if (rc) {
		printk("Bluetooth init failed (err %d)\n", rc);
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
