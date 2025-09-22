/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/console/console.h>
#include <string.h>
#include <stdlib.h>
#include <zephyr/sys/printk.h>
#include <zephyr/types.h>

#if defined(CONFIG_USB_DEVICE_STACK)
#include <zephyr/usb/usb_device.h>
#include <zephyr/drivers/uart.h>
#endif

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/sys/byteorder.h>
#include <bluetooth/services/latency.h>
#include <bluetooth/services/latency_client.h>
#include <bluetooth/scan.h>
#include <bluetooth/gatt_dm.h>
#include <bluetooth/hci_vs_sdc.h>
#include <zephyr/logging/log.h>
#include <bluetooth/hci_vs_sdc.h>
#include "filter_algo.h"

#define DEVICE_NAME		   CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN		   (sizeof(DEVICE_NAME) - 1)
#define INTERVAL_UNITS		   0x6	/* 6 units,  7.5 ms */
#define FILTER_EVALUATION_INTERVAL 2000 /* Evaluate every 2000 packets */
#define CHMAP_BT_CONN_CH_COUNT	   37
#define CHMAP_BLE_BITMASK_SIZE	   5

static K_SEM_DEFINE(test_ready_sem, 0, 1);
static bool test_ready;
static struct bt_conn *default_conn;
static struct bt_latency latency;
static struct bt_latency_client latency_client;
static struct bt_le_conn_param *conn_param =
	/* Use standard 7.5 ms connection interval (6 * 1.25 ms) */
	BT_LE_CONN_PARAM(INTERVAL_UNITS, INTERVAL_UNITS, 0, 400);
static struct bt_conn_info conn_info = {0};

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_LATENCY_VAL),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static struct chmap_instance filter_chmap_instance;
static uint32_t total_packets_sent = 0;
static uint32_t packets_since_last_evaluation = 0;

LOG_MODULE_REGISTER(paramtest, LOG_LEVEL_INF);

static void init_channel_map(void)
{
	memset(&filter_chmap_instance, 0, sizeof(filter_chmap_instance));
	channel_map_filter_algo_init(&filter_chmap_instance);

	total_packets_sent = 0;
	packets_since_last_evaluation = 0;

	LOG_INF("Channel map and algorithm initialized");
	LOG_INF("Algorithm will evaluate every %d packets", FILTER_EVALUATION_INTERVAL);
}

static int read_conn_channel_map(struct bt_conn *conn, uint8_t out_map[5])
{
	if (!conn || !out_map) {
		return -EINVAL;
	}

	uint16_t handle;
	int err = bt_hci_get_conn_handle(conn, &handle);
	if (err) {
		return err;
	}

	struct net_buf *buf = bt_hci_cmd_create(BT_HCI_OP_LE_READ_CHAN_MAP,
						sizeof(struct bt_hci_cp_le_read_chan_map));
	if (!buf) {
		return -ENOBUFS;
	}

	struct bt_hci_cp_le_read_chan_map *cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(handle);

	struct net_buf *rsp = NULL;
	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_READ_CHAN_MAP, buf, &rsp);
	if (err) {
		if (rsp) {
			net_buf_unref(rsp);
		}
		return err;
	}

	const struct bt_hci_rp_le_read_chan_map *rp = (void *)rsp->data;
	memcpy(out_map, rp->ch_map, sizeof(rp->ch_map));
	net_buf_unref(rsp);
	return 0;
}

void algorithm_evaluation_and_update(void)
{

	int eval_result = channel_map_filter_algo_evaluate(&filter_chmap_instance);

	if (eval_result == 1) { /* Algorithm performed evaluation, get new channel map */
		uint8_t *new_channel_map =
			channel_map_filter_algo_get_channel_map(&filter_chmap_instance);

		if (new_channel_map) { /* Apply new map */
			LOG_INF("Applying new channel map");

			int err = bt_le_set_chan_map(new_channel_map);
			if (err) {
				LOG_ERR("Failed to apply channel map (err %d)\n", err);
			} else {
				LOG_INF("Successfully applied channel map\n");
				set_suggested_bitmask_to_current_bitmask(&filter_chmap_instance);
			}
		}
	} else if (eval_result == 0) {
		return;

	} else {
		LOG_ERR("Algorithm evaluation failed (err %d)\n", eval_result);
	}

	packets_since_last_evaluation = 0;
}

void scan_filter_match(struct bt_scan_device_info *device_info,
		       struct bt_scan_filter_match *filter_match, bool connectable)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));

	printk("Filters matched. Address: %s connectable: %d\n\n", addr, connectable);
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
		.connect_if_match = true, .scan_param = &scan_param, .conn_param = conn_param};

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

	printk("Service discovery completed\n\n");

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
	.completed = discovery_complete,
	.service_not_found = discovery_service_not_found,
	.error_found = discovery_error,
};

static void adv_start(void)
{
	int err;

	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_2, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
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

	printk("Scanning successfully started\n\n");
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
	printk("Connection interval: %d units (%.1f ms)\n", conn_info.le.interval,
	       conn_info.le.interval * 1.25);
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

static bool on_vs_evt(struct net_buf_simple *buf)
{

	if (conn_info.role !=
	    BT_CONN_ROLE_CENTRAL) { /* if peripheral gets inside callback, simply leave*/
		return true;
	}

	uint8_t code;
	sdc_hci_subevent_vs_qos_conn_event_report_t *evt;

	code = net_buf_simple_pull_u8(buf);
	if (code != SDC_HCI_SUBEVENT_VS_QOS_CONN_EVENT_REPORT) {
		return false;
	}

	evt = (void *)buf->data;

	uint8_t channel_index = evt->channel_index;
	if (channel_index >= CHMAP_BT_CONN_CH_COUNT) {
		printk("Invalid channel index: %d (max allowed: %d)\n", channel_index,
		       CHMAP_BT_CONN_CH_COUNT - 1);
		return false;
	}

	total_packets_sent++;
	packets_since_last_evaluation++;
	filter_chmap_instance.processed_samples_count++; /* total count (all packets)*/
	filter_chmap_instance.bt_channels[channel_index]
		.total_packets_sent++; /*local count (per channel packets)*/

	if (evt->crc_error_count > 0) { /* CRC error occurred */
		filter_chmap_instance.bt_channels[channel_index].total_crc_errors++;
	} else {
		filter_chmap_instance.bt_channels[channel_index].total_crc_ok++;
	}

	if (evt->rx_timeout > 0) { /* rx-timeout occurred */
		filter_chmap_instance.bt_channels[channel_index].total_rx_timeouts +=
			evt->rx_timeout;
	}

	if (packets_since_last_evaluation >= FILTER_EVALUATION_INTERVAL) {
		algorithm_evaluation_and_update();
	}

	return true;
}

static int enable_qos_conn_evt_report(void)
{
	int err;
	sdc_hci_cmd_vs_qos_conn_event_report_enable_t cmd_enable;

	err = bt_hci_register_vnd_evt_cb(on_vs_evt);
	if (err) {
		printk("Failed registering vendor specific callback (err %d)\n", err);
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

static void test_run(void)
{
	int err;
	if (!test_ready) {
		/* disconnected while blocking inside _getchar() */
		return;
	}

	test_ready = false;
	uint8_t previous_active_map[CHMAP_BLE_BITMASK_SIZE] = {0};
	uint8_t active_map[CHMAP_BLE_BITMASK_SIZE] = {0};

	/* Start sending data to trigger packet events */
	while (default_conn) {
		uint32_t time = k_cycle_get_32();

		err = bt_latency_request(&latency_client, &time, sizeof(time));
		if (err && err != -EALREADY) {
			printk("Latency request failed (err %d)\n", err);
		}

		if (conn_info.role == BT_CONN_ROLE_PERIPHERAL) {
			/* Read back controller's active channel map for verification */
			err = read_conn_channel_map(default_conn, active_map);
			if (!err) {
				if (memcmp(active_map, previous_active_map,
					   CHMAP_BLE_BITMASK_SIZE) != 0) {

					printk("Detected Channel Map Update. (format, CH36 -> "
					       "CH0)\n");

					/* Print channel map in Hexadecimal */
					printk("LL channel map, HEX: %02x %02x %02x %02x "
					       "%02x\n",
					       active_map[4], active_map[3], active_map[2],
					       active_map[1], active_map[0]);

					/* Print channel map in Bits */
					char bitstr[CHMAP_BT_CONN_CH_COUNT + 1];
					for (int ch = CHMAP_BT_CONN_CH_COUNT - 1; ch >= 0; ch--) {
						uint8_t bit =
							(active_map[ch / 8] >> (ch % 8)) & 0x01;
						bitstr[CHMAP_BT_CONN_CH_COUNT - 1 - ch] =
							bit ? '1' : '0';
					}
					bitstr[CHMAP_BT_CONN_CH_COUNT] = '\0';
					printk("LL channel map, BITS: %s\n\n", bitstr);

					/* Copy & replace map for next evaluation */
					memcpy(previous_active_map, active_map,
					       CHMAP_BLE_BITMASK_SIZE);
				}
			} else {
				printk("Reading LL channel map failed (err %d)\n", err);
			}
		}
		k_sleep(K_MSEC(1)); /* Wait between requests */
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
	/* Start service discovery when link is encrypted */
	err = bt_gatt_dm_start(default_conn, BT_UUID_LATENCY, &discovery_cb, &latency_client);
	if (err) {
		printk("Discover failed (err %d)\n", err);
	}
}
#endif /* CONFIG_BT_SMP */

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
#if defined(CONFIG_BT_SMP)
	.security_changed = security_changed,
#endif /* CONFIG_BT_SMP */
};

int main(void)
{
	int err;
	LOG_INF("Starting Bluetooth Channel Map Update Sample\n");

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

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	LOG_INF("Bluetooth initialized\n");

	err = bt_latency_init(&latency, NULL);
	if (err) {
		printk("Latency service initialization failed (err %d)\n", err);
		return 0;
	}

	err = bt_latency_client_init(&latency_client, NULL);
	if (err) {
		printk("Latency client initialization failed (err %d)\n", err);
		return 0;
	}

	while (true) {
		LOG_INF("Choose device role - type c (central) or p (peripheral): ");

		char input_char = console_getchar();

		if (input_char == 'c') {
			LOG_INF("Central. Starting scanning");
			init_channel_map();
			scan_init();
			scan_start();

			if (enable_qos_conn_evt_report()) { /* Enable QoS only central */
				printk("Enable QoS reports failed.\n");
				return 0;
			}

			break;
		} else if (input_char == 'p') {
			LOG_INF("Peripheral. Starting advertising\n");
			adv_start();
			break;
		}

		LOG_WRN("Invalid role\n");
	}

	for (;;) {
		k_sem_take(&test_ready_sem, K_FOREVER);
		test_run();
	}
}