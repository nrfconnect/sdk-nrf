/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/console/console.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <bluetooth/services/latency.h>
#include <bluetooth/services/latency_client.h>
#include <bluetooth/gatt_dm.h>
#include <bluetooth/radio_notification_cb.h>

static struct bt_latency latency;
static struct bt_latency_client latency_client;

void scan_start(void);
void adv_start(void);

static volatile bool latency_request_time_in_use;
static volatile bool discovery_completed;
static uint32_t latency_request_time;
static uint32_t conn_interval_us;
static char role;

static void discovery_complete(struct bt_gatt_dm *dm, void *context)
{
	struct bt_latency_client *latency = context;

	printk("Service discovery completed\n");

	bt_latency_handles_assign(dm, latency);
	bt_gatt_dm_data_release(dm);

	discovery_completed = true;
}

static void discovery_service_not_found(struct bt_conn *conn, void *context)
{
	printk("Service not found\n");
}

static void discovery_error(struct bt_conn *conn, int err, void *context)
{
	printk("Error while discovering GATT database: (err %d)\n", err);
}

static const struct bt_gatt_dm_cb discovery_cb = {
	.completed         = discovery_complete,
	.service_not_found = discovery_service_not_found,
	.error_found       = discovery_error,
};

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	int err;
	char addr[BT_ADDR_LE_STR_LEN];
	struct bt_conn_info info;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		printk("Failed to connect to %s, 0x%02x %s\n", addr, conn_err,
		       bt_hci_err_to_str(conn_err));
		if (role == 'c') {
			scan_start();
		} else {
			adv_start();
		}
		return;
	}

	printk("Connected: %s\n", addr);

	err = bt_conn_get_info(conn, &info);
	if (err) {
		printk("Failed getting conn info, %d\n", err);
		return;
	}

	conn_interval_us = BT_CONN_INTERVAL_TO_US(info.le.interval);

	/*Start service discovery when link is encrypted*/
	err = bt_gatt_dm_start(conn, BT_UUID_LATENCY, &discovery_cb,
			       &latency_client);
	if (err) {
		printk("Discover failed (err %d)\n", err);
	}
}

static void on_conn_param_updated(struct bt_conn *conn,
				  uint16_t interval,
				  uint16_t latency,
				  uint16_t timeout)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(latency);
	ARG_UNUSED(timeout);

	conn_interval_us = BT_CONN_INTERVAL_TO_US(interval);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s, reason 0x%02x %s\n", addr, reason, bt_hci_err_to_str(reason));

	latency_request_time_in_use = false;
	discovery_completed = false;

	if (role == 'c') {
		scan_start();
	} else {
		adv_start();
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.le_param_updated = on_conn_param_updated,
	.disconnected = disconnected,
};

static void latency_response_handler(const void *buf, uint16_t len)
{
	uint32_t latency_time;

	if (len == sizeof(latency_time)) {
		/* compute how long the time spent */

		latency_time = *((uint32_t *)buf);
		uint32_t cycles_spent = k_cycle_get_32() - latency_time;
		uint32_t total_latency_us = (uint32_t)k_cyc_to_us_floor64(cycles_spent);

		/* The latency service uses ATT Write Requests.
		 * The ATT Write Response is sent in the next connection event.
		 * The actual expected latency is therefore the total latency minus the connection
		 * interval.
		 */
		uint32_t latency_minus_conn_interval = total_latency_us - conn_interval_us;

		printk("Latency: %d us, round trip: %d us\n",
			latency_minus_conn_interval, total_latency_us);

		latency_request_time_in_use = false;
	}
}

static const struct bt_latency_client_cb latency_client_cb = {
	.latency_response = latency_response_handler
};

static void radio_notification_conn_cb(struct bt_conn *conn)
{
	int err;

	ARG_UNUSED(conn);

	if (!discovery_completed || latency_request_time_in_use) {
		return;
	}

	latency_request_time = k_cycle_get_32();
	err = bt_latency_request(&latency_client,
				 &latency_request_time,
				 sizeof(latency_request_time));
	if (err) {
		printk("Latency request failed (err %d)\n", err);
	}
	latency_request_time_in_use = true;
}

static const struct bt_radio_notification_conn_cb callbacks = {
	.prepare = radio_notification_conn_cb,
};

int main(void)
{
	int err;

	printk("Starting radio notification callback sample.\n");

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	err = bt_radio_notification_conn_cb_register(&callbacks,
		BT_RADIO_NOTIFICATION_CONN_CB_PREPARE_DISTANCE_US_RECOMMENDED);
	if (err) {
		printk("Failed registering radio notification callback (err %d)\n", err);
		return 0;
	}

	err = bt_latency_init(&latency, NULL);
	if (err) {
		printk("Latency service initialization failed (err %d)\n", err);
		return 0;
	}

	err = bt_latency_client_init(&latency_client, &latency_client_cb);
	if (err) {
		printk("Latency client initialization failed (err %d)\n", err);
		return 0;
	}

	console_init();
	do {
		printk("Choose device role - type c (central) or p (peripheral): ");

		role = console_getchar();

		switch (role) {
		case 'p':
			printk("\nPeripheral. Starting advertising\n");
			adv_start();
			break;
		case 'c':
			printk("\nCentral. Starting scanning\n");
			scan_start();
			break;
		default:
			printk("\n");
			break;
		}
	} while (role != 'c' && role != 'p');
}
