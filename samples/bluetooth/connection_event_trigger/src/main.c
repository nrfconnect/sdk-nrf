/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <string.h>
#include <zephyr/console/console.h>
#include <zephyr/sys/printk.h>
#include <zephyr/types.h>
#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <bluetooth/scan.h>

#include <hal/nrf_egu.h>
#include <sdc_hci_vs.h>

#if defined(DPPIC_PRESENT)
#define SWI_IRQn EGU0_IRQn
#else
#define SWI_IRQn SWI0_IRQn
#endif

#define NUM_TRIGGERS   (50)
#define INTERVAL_10MS  (0x8)
#define INTERVAL_100MS (0x50)
#define PPI_CH_ID      15

#define ADVERTISING_UUID128 BT_UUID_128_ENCODE(0x038a803f, 0xf6b3, 0x420b, 0xa95a, 0x10cc7b32b6db)

struct hci_cmd_vs_get_next_conn_event_counter_return {
	uint8_t status;
	uint16_t conn_handle;
	uint16_t next_conn_event_counter;
} __packed;

static volatile uint8_t timestamp_log_index = NUM_TRIGGERS;
static uint32_t timestamp_log[NUM_TRIGGERS];
static struct bt_conn *active_conn;
static volatile bool connection_established;
static struct k_work work;
static void work_handler(struct k_work *w);

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, ADVERTISING_UUID128),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static void egu0_handler(const void *context)
{
	nrf_egu_event_clear(NRF_EGU0, NRF_EGU_EVENT_TRIGGERED0);

	if (timestamp_log_index < NUM_TRIGGERS) {
		k_work_submit(&work);
	}
}

static void work_handler(struct k_work *w)
{
	timestamp_log[timestamp_log_index] = k_ticks_to_us_near32((uint32_t)k_uptime_ticks());
	timestamp_log_index++;
}

static int setup_connection_event_trigger(struct bt_conn *conn, bool enable)
{
	int err;
	struct net_buf *buf;
	struct net_buf *rsp = NULL;
	sdc_hci_cmd_vs_get_next_conn_event_counter_t *cmd_get_conn_event_counter;
	struct hci_cmd_vs_get_next_conn_event_counter_return *cmd_event_counter_return;
	sdc_hci_cmd_vs_set_conn_event_trigger_t *cmd_set_trigger;
	uint16_t conn_handle;

	err = bt_hci_get_conn_handle(conn, &conn_handle);
	if (err) {
		printk("Failed obtaining conn_handle (err %d)\n", err);
		return err;
	}

	buf = bt_hci_cmd_create(SDC_HCI_OPCODE_CMD_VS_GET_NEXT_CONN_EVENT_COUNTER,
				sizeof(*cmd_get_conn_event_counter));

	if (!buf) {
		printk("Could not allocate command buffer\n");
		return -ENOMEM;
	}

	cmd_get_conn_event_counter = net_buf_add(buf, sizeof(*cmd_get_conn_event_counter));
	cmd_get_conn_event_counter->conn_handle = conn_handle;

	err = bt_hci_cmd_send_sync(SDC_HCI_OPCODE_CMD_VS_GET_NEXT_CONN_EVENT_COUNTER, buf, &rsp);

	if (err) {
		printk("Error for command SDC_HCI_OPCODE_CMD_VS_GET_NEXT_CONN_EVENT_COUNTER (%d)\n",
		       err);
		return err;
	}

	cmd_event_counter_return =
		(struct hci_cmd_vs_get_next_conn_event_counter_return *)rsp->data;

	buf = bt_hci_cmd_create(SDC_HCI_OPCODE_CMD_VS_SET_CONN_EVENT_TRIGGER,
				sizeof(*cmd_set_trigger));

	if (!buf) {
		printk("Could not allocate command buffer\n");
		return -ENOMEM;
	}

	/* Configure event trigger to trigger NRF_EGU_TASK_TRIGGER0
	 * through (D)PPI channel PPI_CH_ID.
	 * This will generate a software interrupt: SWI_IRQn.
	 */

	cmd_set_trigger = net_buf_add(buf, sizeof(*cmd_set_trigger));
	cmd_set_trigger->conn_handle = conn_handle;
	cmd_set_trigger->role = SDC_HCI_VS_CONN_EVENT_TRIGGER_ROLE_CONN;
	cmd_set_trigger->ppi_ch_id = PPI_CH_ID;
	cmd_set_trigger->period_in_events = 1;
	cmd_set_trigger->conn_evt_counter_start =
		cmd_event_counter_return->next_conn_event_counter + 20;

	if (enable) {
		cmd_set_trigger->task_endpoint =
			nrf_egu_task_address_get(NRF_EGU0, NRF_EGU_TASK_TRIGGER0);
		IRQ_DIRECT_CONNECT(SWI_IRQn, 5, egu0_handler, 0);
		nrf_egu_int_enable(NRF_EGU0, NRF_EGU_INT_TRIGGERED0);
		NVIC_EnableIRQ(SWI_IRQn);
	} else {
		cmd_set_trigger->task_endpoint = 0;
		nrf_egu_int_disable(NRF_EGU0, NRF_EGU_INT_TRIGGERED0);
		NVIC_DisableIRQ(SWI_IRQn);
	}

	err = bt_hci_cmd_send_sync(SDC_HCI_OPCODE_CMD_VS_SET_CONN_EVENT_TRIGGER, buf, NULL);
	if (err) {
		printk("Error for command SDC_HCI_OPCODE_CMD_VS_SET_CONN_EVENT_TRIGGER (%d)\n",
		       err);
		return err;
	}

	printk("Successfully configured connection event trigger\n");

	net_buf_unref(rsp);
	return 0;
}

static int change_connection_interval(struct bt_conn *conn, uint16_t new_interval)
{
	int err;
	struct bt_le_conn_param *params = BT_LE_CONN_PARAM(new_interval, new_interval, 0, 400);

	err = bt_conn_le_param_update(conn, params);

	if (err) {
		printk("Error when updating connection parameters (%d)\n", err);
	}

	return err;
}

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		printk("Failed to connect to %s (%u)\n", addr, conn_err);

		if (conn == active_conn) {
			bt_conn_unref(active_conn);
			active_conn = NULL;
		}

		return;
	}

	printk("Connected: %s\n", addr);

	active_conn = bt_conn_ref(conn);
	connection_established = true;
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s (reason %u)\n", addr, reason);

	if (conn == active_conn) {
		bt_conn_unref(active_conn);
		active_conn = NULL;
	}
}

static void le_param_updated(struct bt_conn *conn, uint16_t interval, uint16_t latency,
			     uint16_t timeout)
{
	if (conn == active_conn) {
		printk("Connection parameters updated. New interval: %u ms\n",
					 interval * 1250 / 1000);
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.le_param_updated = le_param_updated,
};

static void adv_start(void)
{
	int err;

	err = bt_le_adv_start(
		BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_ONE_TIME,
			BT_GAP_ADV_FAST_INT_MIN_2,
			BT_GAP_ADV_FAST_INT_MAX_2,
			NULL),
		ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));

	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");
}

static void scan_filter_match(struct bt_scan_device_info *device_info,
			      struct bt_scan_filter_match *filter_match, bool connectable)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));

	printk("Filters matched. Address: %s connectable: %d\n", addr, connectable);
}

static void scan_connecting_error(struct bt_scan_device_info *device_info)
{
	printk("Connecting failed\n");
}

BT_SCAN_CB_INIT(scan_cb, scan_filter_match, NULL, scan_connecting_error, NULL);

static void scan_start(void)
{
	int err;

	struct bt_le_scan_param scan_param = {
		.type = BT_LE_SCAN_TYPE_PASSIVE,
		.options = BT_LE_SCAN_OPT_FILTER_DUPLICATE,
		.interval = 0x0010,
		.window = 0x0010,
	};

	struct bt_le_conn_param *conn_param =
		BT_LE_CONN_PARAM(INTERVAL_100MS, INTERVAL_100MS, 0, 400);

	struct bt_scan_init_param scan_init = {
		.connect_if_match = true,
		.scan_param = &scan_param,
		.conn_param = conn_param,
	};

	bt_scan_init(&scan_init);
	bt_scan_cb_register(&scan_cb);

	err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID,
				 BT_UUID_DECLARE_128(ADVERTISING_UUID128));

	if (err) {
		printk("Scanning filters cannot be set (err %d)\n", err);
		return;
	}

	err = bt_scan_filter_enable(BT_SCAN_UUID_FILTER, false);
	if (err) {
		printk("Filters cannot be turned on (err %d)\n", err);
	}

	err = bt_scan_start(BT_SCAN_TYPE_SCAN_PASSIVE);
	if (err) {
		printk("Starting scanning failed (err %d)\n", err);
		return;
	}

	printk("Scanning successfully started\n");
}

int main(void)
{
	int err;

	k_work_init(&work, work_handler);
	console_init();
	printk("Starting Connection Event Trigger Example.\n");

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	while (true) {
		printk("Choose device role - type c (central) or p (peripheral): ");

		char input_char = console_getchar();

		printk("\n");

		if (input_char == 'c') {
			printk("Central. Starting scanning\n");
			scan_start();
			break;
		} else if (input_char == 'p') {
			printk("Peripheral. Starting advertising\n");
			adv_start();
			break;
		}

		printk("Invalid role\n");
	}

	while (true) {
		if (connection_established) {
			printk("Connection established.\n");
			connection_established = false;
			break;
		}
	}

	for (;;) {
		printk("Press any key to switch to a 10ms connection interval and set up "
		       "connection event trigger:\n");

		(void)console_getchar();
		printk("\n");

		timestamp_log_index = 0;

		change_connection_interval(active_conn, INTERVAL_10MS);

		err = setup_connection_event_trigger(active_conn, true);

		if (err) {
			printk("Could not set up event trigger. (err %d)\n", err);
			return 0;
		}

		while (timestamp_log_index < NUM_TRIGGERS) {
			;
		}

		err = setup_connection_event_trigger(active_conn, false);

		if (err) {
			printk("Could not disable event trigger. (err %d)\n", err);
			return 0;
		}

		printk("Printing connection event trigger log.\n"
		       "+-------------+----------------+----------------------------------+\n"
		       "| Trigger no. | Timestamp (us) | Time since previous trigger (us) |\n"
		       "+-------------+----------------+----------------------------------+\n");

		for (uint16_t i = 1; i < NUM_TRIGGERS; i++) {
			printk("| %11d | %11u us | %29u us |\n", i, timestamp_log[i],
			       timestamp_log[i] - timestamp_log[i - 1]);
		}

		printk("+-------------+----------------+----------------------------------+\n");

		printk("\n");

		change_connection_interval(active_conn, INTERVAL_100MS);
	}
}
