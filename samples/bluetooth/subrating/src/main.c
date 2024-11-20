/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/console/console.h>
#include <zephyr/sys/printk.h>
#include <zephyr/types.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <bluetooth/services/latency.h>
#include <bluetooth/services/latency_client.h>
#include <bluetooth/scan.h>
#include <bluetooth/gatt_dm.h>

#define INTERVAL_UNITS 0x8		 /* 8 units,  10 ms */
#define CONN_TIMEOUT   ((5 * 1000) / 10) /* 5 seconds, 10ms units */

/* Which role should initiate subrate requests.
 * Can be BT_CONN_ROLE_PERIPHERAL or BT_CONN_ROLE_CENTRAL.
 */
#define SUBRATE_INITIATOR_ROLE BT_CONN_ROLE_PERIPHERAL

/* Number of latency service writes to perform after each subrate change. */
#define DATA_BURST_COUNT 10

static K_SEM_DEFINE(test_ready_sem, 0, 1);
static K_SEM_DEFINE(latency_received_sem, 0, 1);
static K_SEM_DEFINE(subrate_changed_sem, 0, 1);

static bool test_ready;
static struct bt_conn *default_conn;

static struct bt_latency latency;
static struct bt_latency_client latency_client;

static struct bt_le_conn_param *conn_param =
	BT_LE_CONN_PARAM(INTERVAL_UNITS, INTERVAL_UNITS, 0, CONN_TIMEOUT);
static struct bt_conn_info conn_info = {0};

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_LATENCY_VAL),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static const struct {
	uint16_t factor;
	uint16_t cn;
} subrate_configurations[] = {
	/* No subrating. */
	{1, 0},
	/* Subrate factor of 10 and continuation number 0.
	 * Expecting a loss of throughput due to the reduced number
	 * of connection events.
	 */
	{10, 0},
	/* Subrate factor of 10 and continuation number 1.
	 * Throughput will match that of the link without subrating.
	 */
	{10, 1},
};

void scan_filter_match(struct bt_scan_device_info *device_info,
		       struct bt_scan_filter_match *filter_match, bool connectable)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));

	printk("Filters matched. Address: %s connectable: %d\n", addr, connectable);
}

void scan_filter_no_match(struct bt_scan_device_info *device_info, bool connectable)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));

	printk("Filter does not match. Address: %s connectable: %d\n", addr, connectable);
}

void scan_connecting_error(struct bt_scan_device_info *device_info)
{
	printk("Connecting failed\n");
}

BT_SCAN_CB_INIT(scan_cb, scan_filter_match, scan_filter_no_match, scan_connecting_error, NULL);

static void scan_init(void)
{
	int err;
	struct bt_le_scan_param scan_param = {
		.type = BT_LE_SCAN_TYPE_PASSIVE,
		.options = BT_LE_SCAN_OPT_FILTER_DUPLICATE,
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
		printk("Scanning filters cannot be set (err %d)\n", err);
		return;
	}

	err = bt_scan_filter_enable(BT_SCAN_UUID_FILTER, false);
	if (err) {
		printk("Filters cannot be turned on (err %d)\n", err);
	}
}

static void discovery_complete(struct bt_gatt_dm *dm, void *context)
{
	struct bt_latency_client *latency = context;

	printk("Service discovery completed\n");

	bt_gatt_dm_data_print(dm);
	bt_latency_handles_assign(dm, latency);
	bt_gatt_dm_data_release(dm);

	/* Start testing when the GATT service is discovered */
	test_ready = true;
	k_sem_give(&test_ready_sem);
}

static void discovery_service_not_found(struct bt_conn *conn, void *context)
{
	printk("Service not found\n");
}

static void discovery_error(struct bt_conn *conn, int err, void *context)
{
	printk("Error while discovering GATT database: (err %d)\n", err);
}

struct bt_gatt_dm_cb discovery_cb = {
	.completed         = discovery_complete,
	.service_not_found = discovery_service_not_found,
	.error_found       = discovery_error,
};

static void adv_start(void)
{
	int err;

	const struct bt_le_adv_param *adv_param =
		BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONN,
				BT_GAP_ADV_FAST_INT_MIN_2,
				BT_GAP_ADV_FAST_INT_MAX_2,
				NULL);

	err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");
}

static void scan_start(void)
{
	int err;

	err = bt_scan_start(BT_SCAN_TYPE_SCAN_PASSIVE);
	if (err) {
		printk("Starting scanning failed (err %d)\n", err);
		return;
	}

	printk("Scanning successfully started\n");
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		printk("Connection failed, err 0x%02x %s\n", err, bt_hci_err_to_str(err));
		return;
	}

	default_conn = bt_conn_ref(conn);
	err = bt_conn_get_info(default_conn, &conn_info);
	if (err) {
		printk("Getting conn info failed (err %d)\n", err);
		return;
	}

	/* make sure we're not scanning or advertising */
	if (conn_info.role == BT_CONN_ROLE_CENTRAL) {
		bt_scan_stop();
	} else {
		bt_le_adv_stop();
	}

	printk("Connected as %s\n",
	       conn_info.role == BT_CONN_ROLE_CENTRAL ? "central" : "peripheral");

	err = bt_gatt_dm_start(default_conn, BT_UUID_LATENCY, &discovery_cb, &latency_client);
	if (err) {
		printk("Discovery failed (err %d)\n", err);
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected, reason 0x%02x %s\n", reason, bt_hci_err_to_str(reason));

	test_ready = false;

	if (default_conn) {
		bt_conn_unref(default_conn);
		default_conn = NULL;
	}

	if (conn_info.role == BT_CONN_ROLE_CENTRAL) {
		scan_start();
	} else {
		adv_start();
	}
}

static void subrate_defaults_set(void)
{
	int err = 0;

	const struct bt_conn_le_subrate_param params = {
		/* Minimum subrate factor. */
		.subrate_min = 1,
		/* Maximum subrate factor. */
		.subrate_max = 10,
		/* Maximum Peripheral latency in units of subrated connection intervals. */
		.max_latency = 10,
		/* Minimum number of underlying connection events to remain active
		 * after a packet containing a Link Layer PDU with a non-zero Length
		 * field is sent or received.
		 */
		.continuation_number = 0,
		/* Maximum supervision timeout. */
		.supervision_timeout = 3000,
	};

	/* Set up default subrating limits for new connections. */
	err = bt_conn_le_subrate_set_defaults(&params);
	if (err) {
		printk("bt_conn_le_subrate_set_defaults returned error %d\n", err);
	}
}

static void subrate_request(uint16_t factor, uint16_t cn)
{
	int err = 0;

	const struct bt_conn_le_subrate_param params = {
		.subrate_min = factor,
		.subrate_max = factor,
		.max_latency = 0,
		.continuation_number = cn,
		.supervision_timeout = CONN_TIMEOUT,
	};

	err = bt_conn_le_subrate_request(default_conn, &params);
	if (err) {
		printk("bt_conn_le_subrate_request returned error %d\n", err);
	}
}

static void subrate_changed(struct bt_conn *conn, const struct bt_conn_le_subrate_changed *params)
{
	if (params->status == BT_HCI_ERR_SUCCESS) {
		printk("Subrate parameters changed: "
		       "Subrate Factor: %d "
		       "Continuation Number: %d\n"
		       "Peripheral latency: 0x%04x "
		       "Supervision timeout: 0x%04x (%d ms)\n",
		       params->factor,
		       params->continuation_number,
		       params->peripheral_latency,
		       params->supervision_timeout,
		       params->supervision_timeout * 10);
	} else {
		printk("Subrate change failed with HCI error: 0x%02x %s\n", params->status,
		       bt_hci_err_to_str(params->status));
	}

	k_sem_give(&subrate_changed_sem);
}

static void latency_response_handler(const void *buf, uint16_t len)
{
	uint32_t latency_time;

	if (len == sizeof(latency_time)) {
		latency_time = *((uint32_t *)buf);
		uint32_t cycles_spent = k_cycle_get_32() - latency_time;
		uint32_t total_latency_us = (uint32_t)k_cyc_to_us_floor64(cycles_spent);

		/* The latency service uses ATT Write Requests.
		 * The ATT Write Response is sent in the next connection event.
		 * The round-trip can thus be delayed by the time remaining until
		 * the next connection event, plus an additional connection interval
		 * to receive the response.
		 */

		printk("Response round-trip time: %d us\n", total_latency_us);

		k_sem_give(&latency_received_sem);
	}
}

static const struct bt_latency_client_cb latency_client_cb = {
	.latency_response = latency_response_handler
};

static void exchange_data_burst(void)
{
	int err;
	int counter = 0;

	printk("Simulating burst of data.\n");

	while (default_conn && counter < DATA_BURST_COUNT) {
		uint32_t time = k_cycle_get_32();

		err = bt_latency_request(&latency_client, &time, sizeof(time));
		if (err && err != -EALREADY) {
			printk("Latency failed (err %d)\n", err);
		}

		if (k_sem_take(&latency_received_sem, K_MSEC(500)) != 0) {
			printk("Did not receive a latency response in time.\n");
		}

		counter++;
	}
}

static void test_run(void)
{
	int cfg_idx = 0;

	if (!test_ready) {
		/* disconnected while blocking inside _getchar() */
		return;
	}

	test_ready = false;

	while (default_conn) {
		subrate_request(subrate_configurations[cfg_idx].factor,
				subrate_configurations[cfg_idx].cn);

		if (k_sem_take(&subrate_changed_sem, K_SECONDS(5)) != 0) {
			printk("Did not receive subrate changed event in time.\n");
		}

		exchange_data_burst();

		k_sleep(K_SECONDS(1));

		cfg_idx = (cfg_idx + 1) % ARRAY_SIZE(subrate_configurations);
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.subrate_changed = subrate_changed,
};

int main(void)
{
	int err;

	console_init();

	printk("Starting Bluetooth Subrating sample\n");

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	printk("Bluetooth initialized\n");

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

	while (true) {
		printk("Choose device role - type c (central) or p (peripheral): ");

		char input_char = console_getchar();

		printk("\n");

		if (input_char == 'c') {
			printk("Central. Starting scanning\n");

			subrate_defaults_set();

			scan_init();
			scan_start();
			break;
		} else if (input_char == 'p') {
			printk("Peripheral. Starting advertising\n");

			adv_start();
			break;
		}

		printk("Invalid role\n");
	}

	for (;;) {
		k_sem_take(&test_ready_sem, K_FOREVER);

		if (conn_info.role == SUBRATE_INITIATOR_ROLE) {
			test_run();
		} else {
			return 0;
		}
	}
}
