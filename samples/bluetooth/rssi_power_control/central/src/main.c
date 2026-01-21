/*
 * Copyright (c) 2025 Nordic Semiconductor
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bluetooth/scan.h>
#include <sdc_hci_vs.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

static struct bt_conn *default_conn;

/* Set up logging */
LOG_MODULE_REGISTER(central_unit, LOG_LEVEL_INF);

/* Creating a delayed work item */
static struct k_work_delayable tx_rssi_work;

#define POLLING_INTERVAL_MS 500
#define POWER_CONTROL_SENSITIVITY 0
#define ENABLED 1
#define DISABLED 0
#define LOWER_LIMIT_RSSI -60
#define UPPER_LIMIT_RSSI -40
#define LOWER_TARGET_RSSI -55
#define UPPER_TARGET_RSSI -45
#define WAIT_TIME_MS 1000
#define PERIPHERAL_NAME "power_control_peripheral"

static int8_t peripheral_tx_power;

/* After a name match has occurred, this is the functionality that occurs: */
static void scan_filter_match(struct bt_scan_device_info *device_info,
			      struct bt_scan_filter_match *filter_match,
			      bool connectable)
{
	char addr[BT_ADDR_LE_STR_LEN];
	struct bt_conn_le_create_param *conn_params;
	int err;

	bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));
	LOG_INF("Found peripheral: %s", addr);

	bt_scan_stop();

	conn_params = BT_CONN_LE_CREATE_PARAM(
		BT_CONN_LE_OPT_NONE,
		BT_GAP_SCAN_FAST_INTERVAL,
		BT_GAP_SCAN_FAST_INTERVAL
	);

	err = bt_conn_le_create(device_info->recv_info->addr, conn_params,
				BT_LE_CONN_PARAM_DEFAULT, &default_conn);
	if (err) {
		LOG_ERR("Connection failed (err: %d)", err);
		bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
		return;
	}

	LOG_INF("Connection to %s", addr);
}

/* Initialize a scan callback structure */
BT_SCAN_CB_INIT(scan_cb, scan_filter_match, NULL, NULL, NULL);

/* Function to initialize automated power control */
static void init_power_control(void)
{
	sdc_hci_cmd_vs_set_power_control_request_params_t p_params = {
		.beta = POWER_CONTROL_SENSITIVITY,
		.auto_enable = ENABLED,
		.lower_limit = LOWER_LIMIT_RSSI,
		.upper_limit = UPPER_LIMIT_RSSI,
		.lower_target_rssi = LOWER_TARGET_RSSI,
		.upper_target_rssi = UPPER_TARGET_RSSI,
		.wait_period_ms = WAIT_TIME_MS
	};

	int err = sdc_hci_cmd_vs_set_power_control_request_params(&p_params);

	if (err) {
		LOG_ERR("Failed to initialize power control");
		return;
	}
	LOG_INF("Successful initialization of power control");
}

/* Initializes the scanning process */
static void scan_init(void)
{
	int err;

	/* Scan parameters with fast scanning intervals and windows */
	struct bt_le_scan_param scan_param = {
		.type = BT_LE_SCAN_TYPE_ACTIVE,
		.interval = BT_GAP_SCAN_FAST_INTERVAL,
		.window = BT_GAP_SCAN_FAST_WINDOW,
	};

	/* Struct to initialize the scan module */
	struct bt_scan_init_param scan_init = {
		/* Disable automatic connection. Instead handle
		 * connection manually through scan_filter_match()
		 */
		.connect_if_match = DISABLED,
		/* Links to a bt_le_scan_param structure, defined earlier */
		.scan_param = &scan_param,
	};


	bt_scan_init(&scan_init);
	bt_scan_cb_register(&scan_cb);

	err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_NAME, PERIPHERAL_NAME);

	if (err) {
		LOG_ERR("Failed to add peripheral name filter (err %d)", err);
		return;
	}

	err = bt_scan_filter_enable(BT_SCAN_NAME_FILTER, false);
	if (err) {
		LOG_ERR("Failed to enable filter (err: %d)", err);
		return;
	}
}

/* Functionality to read the RSSI value */
static void read_conn_rssi(uint16_t handle, int8_t *rssi)
{
	struct net_buf *buf, *rsp = NULL;
	struct bt_hci_cp_read_rssi *cp;
	struct bt_hci_rp_read_rssi *rp;

	int err;


	buf = bt_hci_cmd_alloc(K_FOREVER);
	if (!buf) {
		LOG_ERR("Unnable to allocate command buffer");
		return;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(handle);

	err = bt_hci_cmd_send_sync(BT_HCI_OP_READ_RSSI, buf, &rsp);
	if (err) {
		LOG_ERR("Read RSSI err: %d", err);
		return;
	}

	rp = (void *)rsp->data;
	*rssi = rp->rssi;

	net_buf_unref(rsp);
}

/* Work handler to poll Tx and RSSI periodically */
static void tx_rssi_work_handler(struct k_work *work)
{
	uint16_t conn_handle;
	int rc;
	int8_t rssi = 0;

	if (!default_conn) {
		LOG_ERR("No connection, do not reschedule");
		return;
	}

	rc = bt_hci_get_conn_handle(default_conn, &conn_handle);
	if (rc) {
		LOG_ERR("Failed to get connection handle");
		return;
	}

	read_conn_rssi(conn_handle, &rssi);
	LOG_INF("Tx: %d dBm, RSSI: %d dBm", peripheral_tx_power, rssi);

	/* Reschedule the work */
	k_work_reschedule(&tx_rssi_work, K_MSEC(POLLING_INTERVAL_MS));
}

/* Function activated when power report callback occurs */
static void tx_power_report_cb(struct bt_conn *conn,
			       const struct bt_conn_le_tx_power_report *report)
{
	peripheral_tx_power = report->tx_power_level;
}

/* This function runs after a successful or failed connection attempt */
static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	int err;

	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		LOG_ERR("Failed to connect to %s (err 0x%02x)", addr, conn_err);
		bt_conn_unref(default_conn);
		default_conn = NULL;
		bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
		return;
	}
	LOG_INF("Connected to %s", addr);

	/* Enable transmit power reporting for both local and remote devices */
	err = bt_conn_le_set_tx_power_report_enable(conn, true, true);

	if (err) {
		LOG_ERR("Failed to enable TX power reporting (err: %d)", err);
		return;
	}
	init_power_control();

	/* Start periodic TX + RSSI polling */
	k_work_init_delayable(&tx_rssi_work, tx_rssi_work_handler);
	k_work_reschedule(&tx_rssi_work, K_MSEC(POLLING_INTERVAL_MS));
}

/* Function when disconnection occurs */
static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_INF("Disconnected from %s (reason: 0x%02x)", addr, reason);

	if (default_conn == conn) {
		bt_conn_unref(default_conn);
		default_conn = NULL;
		bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
	}

	k_work_cancel_delayable(&tx_rssi_work);
}

/* Register the callback functions */
BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

BT_CONN_CB_DEFINE(tx_power_cb) = {
	.tx_power_report = tx_power_report_cb,
};

int main(void)
{
	int err;

	LOG_INF("Starting Power Control sample");

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return 0;
	}

	scan_init();

	err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
	if (err) {
		LOG_ERR("Scanning failed to start (err: %d)", err);
		return 0;
	}

	LOG_INF("Scanning for peripherals...");

	k_sleep(K_FOREVER);
}
