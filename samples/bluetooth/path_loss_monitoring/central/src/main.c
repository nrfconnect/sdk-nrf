/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bluetooth/scan.h>
#include <dk_buttons_and_leds.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/logging/log.h>

#define ENABLED 1
#define DISABLED 0
#define LOW_PATH_LOSS_LED DK_LED1
#define MEDIUM_PATH_LOSS_LED DK_LED2
#define HIGH_PATH_LOSS_LED DK_LED3
#define HIGH_PATH_LOSS_BORDER 60
#define LOW_PATH_LOSS_BORDER 40
#define NUM_ITERATIONS 5
#define PATH_LOSS_BUFFER_ZONE 5
#define LED_ON 1
#define LED_OFF 0
#define PERIPHERAL_NAME "path_loss_peripheral"

static struct bt_conn *default_conn;
/*s*/
/* Set up logging */
LOG_MODULE_REGISTER(central_unit, LOG_LEVEL_INF);
/* s */
/* After a name match has occurred, this is the functionality that occurs: */
static void scan_filter_match(struct bt_scan_device_info *device_info,
	struct bt_scan_filter_match *filter_match, bool connectable)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));
	LOG_INF("Found peripheral: %s", addr);

	bt_scan_stop();

	struct bt_conn_le_create_param *conn_params = BT_CONN_LE_CREATE_PARAM(
		BT_CONN_LE_OPT_NONE, BT_GAP_SCAN_FAST_INTERVAL, BT_GAP_SCAN_FAST_INTERVAL);

	int err = bt_conn_le_create(device_info->recv_info->addr, conn_params,
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

/* This function runs after a successful or failed connection attempt */
static void connected(struct bt_conn *conn, uint8_t conn_err)
{
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

	/* Define path loss monitoring parameters */
	struct bt_conn_le_path_loss_reporting_param pl_param = {
			.high_threshold = HIGH_PATH_LOSS_BORDER,
			.high_hysteresis = PATH_LOSS_BUFFER_ZONE,
			.low_threshold = LOW_PATH_LOSS_BORDER,
			.low_hysteresis = PATH_LOSS_BUFFER_ZONE,
			.min_time_spent = NUM_ITERATIONS};

	/* Set the parameters */

	int err = bt_conn_le_set_path_loss_mon_param(conn, &pl_param);

	if (err) {
		LOG_ERR("Failed to set path loss monitoring parameters (er: %d)", err);
		return;
	}

	/* Enable path loss monitoring */
	err = bt_conn_le_set_path_loss_mon_enable(conn, true);
	if (err) {
		LOG_ERR("Failed to enable path loss monitoring (err: %d)", err);
		return;
	}
	LOG_INF("Path loss monitoring enabled");
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
}

/* Initializes the scanning process */
static void scan_init(void)
{
	/* Scan parameters with fast scanning intervals and windows */
	struct bt_le_scan_param scan_param = {
			.type = BT_LE_SCAN_TYPE_ACTIVE,
			.interval = BT_GAP_SCAN_FAST_INTERVAL,
			.window = BT_GAP_SCAN_FAST_WINDOW,
	};

	/* Struct to initialize the scan module */
	/* Disable automatic connection. Instead handle
	 * connection manually through scan_filter_match().
	 */
	struct bt_scan_init_param scan_init = {
			.connect_if_match =
	DISABLED,
			/* Links to a bt_le_scan_param structure,
			 * defined earlier
			 */
			.scan_param = &scan_param,
	};

	bt_scan_init(&scan_init);
	bt_scan_cb_register(&scan_cb);

	int err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_NAME, PERIPHERAL_NAME);

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

/* Initialize a path loss callback structure */
static void path_loss_threshold_report(
		struct bt_conn *conn,
		const struct bt_conn_le_path_loss_threshold_report *report)
{
	LOG_INF(">>> Zone changed to: %d | Path loss: %d dB", report->zone, report->path_loss);
	if (report->zone == 0) {
		dk_set_led(LOW_PATH_LOSS_LED, LED_ON);
		dk_set_led(MEDIUM_PATH_LOSS_LED, LED_OFF);
		dk_set_led(HIGH_PATH_LOSS_LED, LED_OFF);
	} else if (report->zone == 1) {
		dk_set_led(LOW_PATH_LOSS_LED, LED_OFF);
		dk_set_led(MEDIUM_PATH_LOSS_LED, LED_ON);
		dk_set_led(HIGH_PATH_LOSS_LED, LED_OFF);
	} else if (report->zone == 2) {
		dk_set_led(LOW_PATH_LOSS_LED, LED_OFF);
		dk_set_led(MEDIUM_PATH_LOSS_LED, LED_OFF);
		dk_set_led(HIGH_PATH_LOSS_LED, LED_ON);
	}
}

/* Register the callback functions */
BT_CONN_CB_DEFINE(conn_callbacks) = {
		.connected = connected,
		.disconnected = disconnected,
		.path_loss_threshold_report = path_loss_threshold_report,
};

int main(void)
{
	int err;

	LOG_INF("Starting Path Loss Monitoring sample");

	err = dk_leds_init();
	if (err) {
		LOG_ERR("LEDs init failed (err %d)", err);
		return 0;
	}

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
}
