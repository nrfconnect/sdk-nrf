/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <bluetooth/gatt_dm.h>
#include <bluetooth/scan.h>

#include <dk_buttons_and_leds.h>
#include <zephyr/sys/byteorder.h>

#include <net/nrf_cloud.h>
#include <zephyr/logging/log.h>
#include "aggregator.h"

LOG_MODULE_DECLARE(lte_ble_gw);

/* Thinghy advertisement UUID */
#define BT_UUID_THINGY_VAL \
	BT_UUID_128_ENCODE(0xef680100, 0x9b35, 0x4933, 0x9b10, 0x52ffa9740042)

#define BT_UUID_THINGY \
	BT_UUID_DECLARE_128(BT_UUID_THINGY_VAL)
/* Thingy service UUID */
#define BT_UUID_TMS_VAL \
	BT_UUID_128_ENCODE(0xef680400, 0x9b35, 0x4933, 0x9b10, 0x52ffa9740042)

#define BT_UUID_TMS \
	BT_UUID_DECLARE_128(BT_UUID_TMS_VAL)
		/* Thingy characteristic UUID */
#define BT_UUID_TOC_VAL \
	BT_UUID_128_ENCODE(0xef680403, 0x9b35, 0x4933, 0x9b10, 0x52ffa9740042)

#define BT_UUID_TOC \
	BT_UUID_DECLARE_128(BT_UUID_TOC_VAL)
extern void alarm(void);

static uint8_t on_received(struct bt_conn *conn,
			struct bt_gatt_subscribe_params *params,
			const void *data, uint16_t length)
{
	if (length > 0) {
		LOG_INF("Orientation: %x", ((uint8_t *)data)[0]);
		struct sensor_data in_data;

		in_data.type = THINGY_ORIENTATION;
		in_data.length = 1;
		in_data.data[0] = ((uint8_t *)data)[0];

		if (aggregator_put(in_data) != 0) {
			LOG_ERR("Was not able to insert Thingy orientation data into aggregator");
		}
		/* If the thingy is upside down, trigger an alarm. */
		if (((uint8_t *)data)[0] == 3) {
			alarm();
		}

	} else {
		LOG_DBG("Orientation notification with 0 length");
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

	const struct bt_gatt_dm_attr *chrc;
	const struct bt_gatt_dm_attr *desc;

	chrc = bt_gatt_dm_char_by_uuid(disc, BT_UUID_TOC);
	if (!chrc) {
		LOG_ERR("Missing Thingy orientation characteristic");
		goto release;
	}

	desc = bt_gatt_dm_desc_by_uuid(disc, chrc, BT_UUID_TOC);
	if (!desc) {
		LOG_ERR("Missing Thingy orientation char value descriptor");
		goto release;
	}

	param.value_handle = desc->handle,

	desc = bt_gatt_dm_desc_by_uuid(disc, chrc, BT_UUID_GATT_CCC);
	if (!desc) {
		LOG_ERR("Missing Thingy orientation char CCC descriptor");
		goto release;
	}

	param.ccc_handle = desc->handle;

	err = bt_gatt_subscribe(bt_gatt_dm_conn_get(disc), &param);
	if (err) {
		LOG_ERR("Subscribe failed (err %d)", err);
	}

release:
	err = bt_gatt_dm_data_release(disc);
	if (err) {
		LOG_ERR("Could not release discovery data, err: %d", err);
	}
}

static void discovery_service_not_found(struct bt_conn *conn, void *ctx)
{
	LOG_ERR("Thingy orientation service not found!");
}

static void discovery_error_found(struct bt_conn *conn, int err, void *ctx)
{
	LOG_ERR("The discovery procedure failed, err %d", err);
}

static struct bt_gatt_dm_cb discovery_cb = {
	.completed = discovery_completed,
	.service_not_found = discovery_service_not_found,
	.error_found = discovery_error_found,
};

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	int err;
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		LOG_ERR("Failed to connect to %s (%u)", addr, conn_err);
		return;
	}

	LOG_INF("Connected: %s", addr);

	err = bt_gatt_dm_start(conn, BT_UUID_TMS, &discovery_cb, NULL);
	if (err) {
		LOG_ERR("Could not start service discovery, err %d", err);
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

	bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));

	LOG_INF("Device found: %s", addr);
}

void scan_connecting_error(struct bt_scan_device_info *device_info)
{
	LOG_ERR("Connection to peer failed!");
}

BT_SCAN_CB_INIT(scan_cb, scan_filter_match, NULL, scan_connecting_error, NULL);

static void scan_start(void)
{
	int err;

	struct bt_le_scan_param scan_param = {
		.type = BT_LE_SCAN_TYPE_ACTIVE,
		.options = BT_LE_SCAN_OPT_FILTER_DUPLICATE,
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
		LOG_ERR("Scanning filters cannot be set");
		return;
	}

	err = bt_scan_filter_enable(BT_SCAN_UUID_FILTER, false);
	if (err) {
		LOG_ERR("Filters cannot be turned on");
	}

	err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
	if (err) {
		LOG_ERR("Scanning failed to start, err %d", err);
	}

	LOG_INF("Scanning...");
}

static void ble_ready(int err)
{
	LOG_INF("Bluetooth ready");

	bt_conn_cb_register(&conn_callbacks);
	scan_start();
}

void ble_init(void)
{
	int err;

	LOG_INF("Initializing Bluetooth..");
	err = bt_enable(ble_ready);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return;
	}
}
