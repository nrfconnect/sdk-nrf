/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <console.h>
#include <string.h>
#include <misc/printk.h>
#include <zephyr/types.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/hci.h>
#include <bluetooth/uuid.h>
#include <bluetooth/services/latency.h>
#include <bluetooth/services/latency_c.h>
#include <bluetooth/scan.h>
#include <bluetooth/gatt_dm.h>
#include <ble_controller_hci_vs.h>

#define DEVICE_NAME	CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)
#define INTERVAL_MIN    0x50     /* 80 units,  100 ms */
#define INTERVAL_MAX    0x50     /* 80 units,  100 ms */
#define INTERVAL_LLPM   0x0D01   /* Proprietary  1 ms */

static volatile bool test_ready;
static struct bt_conn *default_conn;
static struct bt_gatt_latency gatt_latency;
static struct bt_gatt_latency_c gatt_latency_client;
static struct bt_le_conn_param *conn_param =
	BT_LE_CONN_PARAM(INTERVAL_MIN, INTERVAL_MAX, 0, 400);
static struct bt_conn_info conn_info = {0};

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, LATENCY_UUID),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
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

	printk("Filter does not match. Address: %s connectable: %d\n",
	       addr, connectable);
}

void scan_connecting_error(struct bt_scan_device_info *device_info)
{
	printk("Connecting failed\n");
}

BT_SCAN_CB_INIT(scan_cb, scan_filter_match, scan_filter_no_match,
		scan_connecting_error, NULL);

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
		.connect_if_match = true,
		.scan_param = &scan_param,
		.conn_param = conn_param
	};

	bt_scan_init(&scan_init);
	bt_scan_cb_register(&scan_cb);

	err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, BT_UUID_LATENCY);
	if (err) {
		printk("Scanning filters cannot be set\n");
		return;
	}

	err = bt_scan_filter_enable(BT_SCAN_UUID_FILTER, false);
	if (err) {
		printk("Filters cannot be turned on\n");
	}
}

static void discovery_complete(struct bt_gatt_dm *dm, void *context)
{
	struct bt_gatt_latency_c *latency = context;

	printk("Service discovery completed\n");

	bt_gatt_dm_data_print(dm);
	bt_gatt_latency_c_handles_assign(dm, latency);
	bt_gatt_dm_data_release(dm);

	/* Start testing when the GATT service is discovered */
	test_ready = true;
}

static void discovery_service_not_found(struct bt_conn *conn, void *context)
{
	printk("Service not found\n");
}

static void discovery_error(struct bt_conn *conn, int err, void *context)
{
	printk("Error while discovering GATT database: (%d)\n", err);
}

struct bt_gatt_dm_cb discovery_cb = {
	.completed         = discovery_complete,
	.service_not_found = discovery_service_not_found,
	.error_found       = discovery_error,
};

static void advertise_and_scan(void)
{
	int err;

	err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad),
			      sd, ARRAY_SIZE(sd));
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

static void connected(struct bt_conn *conn, u8_t err)
{
	if (err) {
		printk("Connection failed (err %u)\n", err);
		return;
	}

	default_conn = bt_conn_ref(conn);
	err = bt_conn_get_info(default_conn, &conn_info);
	if (err) {
		printk("Error %d while getting bt conn info\n", err);
	}

	printk("Connected as %s\n",
	       conn_info.role == BT_CONN_ROLE_MASTER ? "master" : "slave");
	printk("Conn. interval is %u units (1.25 ms/unit)\n",
	       conn_info.le.interval);

	/* make sure we're not scanning or advertising */
	bt_le_adv_stop();
	bt_scan_stop();

	err = bt_gatt_dm_start(default_conn, BT_UUID_LATENCY, &discovery_cb,
			       &gatt_latency_client);
	if (err) {
		printk("Discover failed (err %d)\n", err);
	}
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

static void le_param_updated(struct bt_conn *conn, u16_t interval,
			     u16_t latency, u16_t timeout)
{
	if (interval == INTERVAL_LLPM) {
		printk("Connection interval updated: LLPM (1 ms)\n");
	}
}

static int enable_llpm_mode(void)
{
	int err;
	struct net_buf *buf;
	hci_vs_cmd_llpm_mode_set_t *cmd_enable;

	buf = bt_hci_cmd_create(HCI_VS_OPCODE_CMD_LLPM_MODE_SET,
				sizeof(*cmd_enable));
	if (!buf) {
		printk("Could not allocate LLPM command buffer\n");
	}

	cmd_enable = net_buf_add(buf, sizeof(*cmd_enable));
	cmd_enable->enable = true;

	err = bt_hci_cmd_send(HCI_VS_OPCODE_CMD_LLPM_MODE_SET, buf);
	if (err) {
		printk("Error enabling LLPM %d\n", err);
	} else {
		printk("LLPM mode enabled\n");
	}

	return err;
}

static int enable_llpm_short_connection_interval(void)
{
	int err;
	struct bt_le_conn_param conn_param = {
		.interval_min = INTERVAL_LLPM,
		.interval_max = INTERVAL_LLPM,
		.latency = 0,
		.timeout = 300
	};

	err = bt_conn_le_param_update(default_conn, &conn_param);
	if (err) {
		printk("Update connection parameters failed (err %d)\n", err);
	}

	return err;
}

static bool on_vs_evt(struct net_buf_simple *buf)
{
	u8_t code;
	hci_vs_evt_qos_conn_event_report_t *evt;

	code = net_buf_simple_pull_u8(buf);
	if (code != HCI_VS_SUBEVENT_CODE_QOS_CONN_EVENT_REPORT) {
		return false;
	}

	evt = (void *)buf->data;
	if ((evt->event_counter & 0x3FF) == 0) {
		printk("QoS conn event reports: "
		       "channel index 0x%02x, CRC errors 0x%02x\n",
		       evt->channel_index, evt->crc_error_count);
	}

	return true;
}

static int enable_qos_conn_evt_report(void)
{
	int err;
	struct net_buf *buf;

	err = bt_hci_register_vnd_evt_cb(on_vs_evt);
	if (err) {
		printk("Failed registering vendor specific callback\n");
	}

	hci_vs_cmd_qos_conn_event_report_enable_t *cmd_enable;

	buf = bt_hci_cmd_create(HCI_VS_OPCODE_CMD_QOS_CONN_EVENT_REPORT_ENABLE,
				sizeof(*cmd_enable));
	if (!buf) {
		printk("Could not allocate command buffer\n");
	}

	cmd_enable = net_buf_add(buf, sizeof(*cmd_enable));
	cmd_enable->enable = true;

	err = bt_hci_cmd_send(HCI_VS_OPCODE_CMD_QOS_CONN_EVENT_REPORT_ENABLE,
			      buf);
	if (err) {
		printk("Could not send command buffer\n");
	} else {
		printk("Connection event reports enabled\n");
	}

	return err;
}

static void latency_response_handler(const void *buf, u16_t len)
{
	u32_t latency_time;

	if (len == sizeof(latency_time)) {
		/* compute how long the time spent */
		latency_time = *((u32_t *)buf);
		u32_t cycles_spent = k_cycle_get_32() - latency_time;
		u32_t us_spent = SYS_CLOCK_HW_CYCLES_TO_NS(cycles_spent) / 2000;

		printk("Transmission Latency: %u (us)\n", us_spent);
	}
}

static const struct bt_gatt_latency_c_cb latency_client_cb = {
	.latency_response = latency_response_handler
};

static void bt_ready(int err)
{
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	scan_init();

	err = bt_gatt_latency_init(&gatt_latency, NULL);
	if (err) {
		printk("Latency service initialization failed (err %d)\n", err);
		return;
	}

	err = bt_gatt_latency_c_init(&gatt_latency_client, &latency_client_cb);
	if (err) {
		printk("Latency client initialization failed (err %d)\n", err);
		return;
	}

	if (enable_llpm_mode()) {
		printk("Enable LLPM mode failed.\n");
		return;
	}

	advertise_and_scan();
}

static void test_run(void)
{
	int err;

	if (!test_ready) {
		/* disconnected while blocking inside _getchar() */
		return;
	}

	test_ready = false;

	/* Switch to LLPM short connection interval */
	if (conn_info.role == BT_CONN_ROLE_MASTER) {
		printk("Press any key to set LLPM short connection interval (1 ms)\n");
		console_getchar();

		if (enable_llpm_short_connection_interval()) {
			printk("Enable LLPM short connection interval failed\n");
			return;
		}
	}

	printk("Press any key to start measuring transmission latency\n");
	console_getchar();

	/* Start sending the timestamp to its peer */
	while (default_conn) {
		u32_t time = k_cycle_get_32();

		err = bt_gatt_latency_c_request(&gatt_latency_client, &time,
						sizeof(time));
		if (err && err != -EALREADY) {
			printk("Latency failed (err %d)\n", err);
		}

		k_sleep(200); /* sleep 200 ms*/
	}
}

void main(void)
{
	int err;
	static struct bt_conn_cb conn_callbacks = {
		.connected = connected,
		.disconnected = disconnected,
		.le_param_updated = le_param_updated,
	};

	console_init();

	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	if (enable_qos_conn_evt_report()) {
		printk("Enable LLPM QoS failed.\n");
		return;
	}

	bt_conn_cb_register(&conn_callbacks);

	for (;;) {
		if (test_ready) {
			test_run();
		}
	}
}
