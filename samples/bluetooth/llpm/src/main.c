/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/console/console.h>
#include <string.h>
#include <zephyr/sys/printk.h>
#include <zephyr/types.h>

#if defined(CONFIG_USB_DEVICE_STACK)
#include <zephyr/usb/usb_device.h>
#include <zephyr/drivers/uart.h>
#endif

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <bluetooth/services/latency.h>
#include <bluetooth/services/latency_client.h>
#include <bluetooth/scan.h>
#include <bluetooth/gatt_dm.h>
#include <bluetooth/hci_vs_sdc.h>

#define DEVICE_NAME	CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)
#define INTERVAL_MIN      0x6    /* 6 units,  7.5 ms */
#define INTERVAL_MIN_US  7500    /* 7.5 ms */
#define INTERVAL_LLPM  0x0D01    /* Proprietary  1 ms */
#define INTERVAL_LLPM_US 1000


static K_SEM_DEFINE(test_ready_sem, 0, 1);
static bool test_ready;
static struct bt_conn *default_conn;
static struct bt_latency latency;
static struct bt_latency_client latency_client;
static struct bt_le_conn_param *conn_param =
	BT_LE_CONN_PARAM(INTERVAL_LLPM, INTERVAL_LLPM, 0, 400);
static struct bt_conn_info conn_info = {0};

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_LATENCY_VAL),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static struct {
	uint32_t latency;
	uint32_t crc_mismatches;
} llpm_latency;

void scan_filter_match(struct bt_scan_device_info *device_info,
		       struct bt_scan_filter_match *filter_match,
		       bool connectable)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));

	printk("Filters matched. Address: %s connectable: %d\n",
	       addr, connectable);
}

void scan_filter_no_match(struct bt_scan_device_info *device_info,
			  bool connectable)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));

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

	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_2, ad, ARRAY_SIZE(ad),
			      sd, ARRAY_SIZE(sd));
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
#if defined(CONFIG_BT_SMP)
		err = bt_conn_set_security(conn, BT_SECURITY_L2);
		if (err) {
			printk("Failed to set security: %d\n", err);
		}
#endif /* CONFIG_BT_SMP */
	}

	printk("Connected as %s\n",
	       conn_info.role == BT_CONN_ROLE_CENTRAL ? "central" : "peripheral");
	__ASSERT_NO_MSG(conn_info.le.interval == INTERVAL_LLPM);
	printk("Conn. interval is 1 ms\n");
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

static void le_param_updated(struct bt_conn *conn, uint16_t interval,
			     uint16_t latency, uint16_t timeout)
{
	if (interval == INTERVAL_LLPM) {
		printk("Connection interval updated: LLPM (1 ms)\n");
	} else {
		__ASSERT_NO_MSG(interval == INTERVAL_MIN);
		printk("Connection interval updated: 7.5 ms\n");
	}
}

static int enable_llpm_mode(void)
{
	int err;
	sdc_hci_cmd_vs_llpm_mode_set_t cmd_enable;

	cmd_enable.enable = true;

	err = hci_vs_sdc_llpm_mode_set(&cmd_enable);
	if (err) {
		return err;
	}

	printk("LLPM mode enabled\n");
	return 0;
}

static int vs_change_connection_interval(uint16_t interval_us)
{
	int err;
	uint16_t conn_handle;
	sdc_hci_cmd_vs_conn_update_t cmd_conn_update;

	err = bt_hci_get_conn_handle(default_conn, &conn_handle);
	if (err) {
		printk("Failed obtaining conn_handle (err %d)\n", err);
		return err;
	}

	cmd_conn_update.conn_handle         = conn_handle;
	cmd_conn_update.conn_interval_us    = interval_us;
	cmd_conn_update.conn_latency        = 0;
	cmd_conn_update.supervision_timeout = 300;

	err = hci_vs_sdc_conn_update(&cmd_conn_update);
	if (err) {
		printk("Update connection parameters failed (err %d)\n", err);
		return err;
	}

	return 0;
}

static bool on_vs_evt(struct net_buf_simple *buf)
{
	uint8_t code;
	sdc_hci_subevent_vs_qos_conn_event_report_t *evt;

	code = net_buf_simple_pull_u8(buf);
	if (code != SDC_HCI_SUBEVENT_VS_QOS_CONN_EVENT_REPORT) {
		return false;
	}

	evt = (void *)buf->data;
	llpm_latency.crc_mismatches += evt->crc_error_count;

	return true;
}

static int enable_qos_conn_evt_report(void)
{
	int err;
	sdc_hci_cmd_vs_qos_conn_event_report_enable_t cmd_enable;

	err = bt_hci_register_vnd_evt_cb(on_vs_evt);
	if (err) {
		printk("Failed registering vendor specific callback (err %d)\n",
		       err);
		return err;
	}

	cmd_enable.enable = true;

	err = hci_vs_sdc_qos_conn_event_report_enable(&cmd_enable);
	if (err) {
		printk("Could not enable QoS reports (err %d)\n", err);
		return err;
	}

	printk("Connection event reports enabled\n");
	return 0;
}

static void latency_response_handler(const void *buf, uint16_t len)
{
	uint32_t latency_time;

	if (len == sizeof(latency_time)) {
		/* compute how long the time spent */
		latency_time = *((uint32_t *)buf);
		uint32_t cycles_spent = k_cycle_get_32() - latency_time;
		llpm_latency.latency =
			(uint32_t)k_cyc_to_ns_floor64(cycles_spent) / 2000;
	}
}

static const struct bt_latency_client_cb latency_client_cb = {
	.latency_response = latency_response_handler
};

static void test_run(void)
{
	int err;
	if (!test_ready) {
		/* disconnected while blocking inside _getchar() */
		return;
	}

	test_ready = false;

	printk("Press any key to start measuring transmission latency\n");
	console_getchar();

	/* Start sending the timestamp to its peer */
	while (default_conn) {
		uint32_t time = k_cycle_get_32();

		err = bt_latency_request(&latency_client, &time, sizeof(time));
		if (err && err != -EALREADY) {
			printk("Latency failed (err %d)\n", err);
		}

		k_sleep(K_MSEC(200)); /* wait for latency response */

		if (llpm_latency.latency) {
			printk("Transmission Latency: %u (us), CRC mismatches: %u\n",
			       llpm_latency.latency,
			       llpm_latency.crc_mismatches);
		} else {
			printk("Did not receive a latency response\n");
		}

		memset(&llpm_latency, 0, sizeof(llpm_latency));
		if (conn_info.role == BT_CONN_ROLE_CENTRAL) {
			static int counter;
			static uint16_t interval_us = INTERVAL_LLPM_US;

			counter++;
			/* Arbitrarily chosen number of measurments
			 * to do before switching interval
			 */
			if (counter == 20) {
				counter = 0;
				if (interval_us == INTERVAL_LLPM_US) {
					interval_us = INTERVAL_MIN_US;
				} else {
					interval_us = INTERVAL_LLPM_US;
				}
				if (vs_change_connection_interval(interval_us)) {
					printk("Enable LLPM short connection interval failed\n");
					return;
				}
			}
		}
	}
}

#if defined(CONFIG_BT_SMP)
void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	printk("Security changed: level %i, err: %i %s\n", level, err, bt_security_err_to_str(err));

	if (err != 0) {
		printk("Failed to encrypt link\n");
		bt_conn_disconnect(conn, BT_HCI_ERR_PAIRING_NOT_SUPPORTED);
		return;
	}
	/*Start service discovery when link is encrypted*/
	err = bt_gatt_dm_start(default_conn, BT_UUID_LATENCY, &discovery_cb,
			       &latency_client);
	if (err) {
		printk("Discover failed (err %d)\n", err);
	}
}
#endif /* CONFIG_BT_SMP */

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.le_param_updated = le_param_updated,
#if defined(CONFIG_BT_SMP)
	.security_changed = security_changed,
#endif /* CONFIG_BT_SMP */
};

int main(void)
{
	int err;

#if defined(CONFIG_USB_DEVICE_STACK)
	const struct device *uart_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
	uint32_t dtr = 0;

	if (usb_enable(NULL)) {
		return 0;
	}

	/* Poll if the DTR flag was set, optional */
	while (!dtr) {
		uart_line_ctrl_get(uart_dev, UART_LINE_CTRL_DTR, &dtr);
		k_msleep(100);
	}
#endif

	console_init();

	printk("Starting Bluetooth LLPM sample\n");

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

	if (enable_llpm_mode()) {
		printk("Enable LLPM mode failed.\n");
		return 0;
	}

	while (true) {
		printk("Choose device role - type c (central) or p (peripheral): ");

		char input_char = console_getchar();

		printk("\n");

		if (input_char == 'c') {
			printk("Central. Starting scanning\n");
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

	if (enable_qos_conn_evt_report()) {
		printk("Enable LLPM QoS failed.\n");
		return 0;
	}

	for (;;) {
		k_sem_take(&test_ready_sem, K_FOREVER);
		test_run();
	}
}
