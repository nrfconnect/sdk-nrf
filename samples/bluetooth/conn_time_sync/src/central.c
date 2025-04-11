/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/sys/atomic.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <bluetooth/services/nus.h>
#include "conn_time_sync.h"
#include <bluetooth/hci_vs_sdc.h>

#define LED_TOGGLE_PERIOD_MS 120

#define LED_TOGGLE_TIME_OFFSET_US 50000

#define ADV_NAME_STR_MAX_LEN (sizeof(CONFIG_BT_DEVICE_NAME))

static void scan_start(void);

static bool led_value;
static uint8_t volatile conn_count;

static const struct bt_uuid *timed_char_uuid = BT_UUID_TIMED_ACTION_CHAR;

static struct {
	atomic_t last_anchor_point_in_use;
	uint16_t timed_action_char_handle;
	uint16_t last_anchor_point_event_counter;
	uint64_t last_anchor_point_timestamp;
	struct timed_action timed_action_msg;
	struct bt_gatt_discover_params discovery_params;
} conn_state[CONFIG_BT_MAX_CONN];

static void send_timestamp_to_peripheral(struct bt_conn *conn, void *data)
{
	int err;
	struct bt_conn_info conn_info;
	uint64_t toggle_time_us = *(uint64_t *)data;

	err = bt_conn_get_info(conn, &conn_info);
	if (err) {
		return;
	}

	uint8_t conn_index = bt_conn_index(conn);

	if (conn_info.state != BT_CONN_STATE_CONNECTED) {
		return;
	}

	if (conn_state[conn_index].timed_action_char_handle == 0) {
		/* Service discovery not yet complete. */
		return;
	}

	if (atomic_test_and_set_bit(&conn_state[conn_index].last_anchor_point_in_use, 0)) {
		return;
	}

	conn_state[conn_index].timed_action_msg.anchor_point_us_central_clock =
		conn_state[conn_index].last_anchor_point_timestamp;
	conn_state[conn_index].timed_action_msg.anchor_point_event_counter =
		conn_state[conn_index].last_anchor_point_event_counter;
	conn_state[conn_index].timed_action_msg.trigger_time_us_central_clock = toggle_time_us;
	conn_state[conn_index].timed_action_msg.led_value = led_value;

	atomic_clear_bit(&conn_state[conn_index].last_anchor_point_in_use, 0);

	err = bt_gatt_write_without_response(conn,
		conn_state[conn_index].timed_action_char_handle,
		&conn_state[conn_index].timed_action_msg,
		sizeof(struct timed_action),
		false);
	if (err) {
		printk("Failed writing to characteristic\n");
	} else {
		printk("Sent to conn index %d: ", conn_index);
		timed_action_print(&conn_state[conn_index].timed_action_msg);
	}
}

static void on_timestamp_send_timeout(struct k_work *work)
{
	uint64_t local_time_us = controller_time_us_get();
	uint64_t toggle_time_us = local_time_us + LED_TOGGLE_TIME_OFFSET_US;

	led_value = !led_value;

	timed_led_toggle_trigger_at(led_value, toggle_time_us);

	bt_conn_foreach(BT_CONN_TYPE_LE, send_timestamp_to_peripheral, &toggle_time_us);

	k_work_schedule(k_work_delayable_from_work(work), K_MSEC(LED_TOGGLE_PERIOD_MS));
}

K_WORK_DELAYABLE_DEFINE(timestamp_send_work, on_timestamp_send_timeout);

static bool adv_data_parse_cb(struct bt_data *data, void *user_data)
{
	char *name = user_data;
	uint8_t len;

	switch (data->type) {
	case BT_DATA_NAME_SHORTENED:
	case BT_DATA_NAME_COMPLETE:
		len = MIN(data->data_len, ADV_NAME_STR_MAX_LEN - 1);
		memcpy(name, data->data, len);
		name[len] = '\0';
		return false;
	default:
		return true;
	}
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	char name_str[ADV_NAME_STR_MAX_LEN] = {0};
	char addr_str[BT_ADDR_LE_STR_LEN];
	int err;

	/* We're only interested in connectable events */
	if (type != BT_GAP_ADV_TYPE_ADV_IND &&
	    type != BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
		return;
	}

	bt_data_parse(ad, adv_data_parse_cb, name_str);

	if (strncmp(name_str, CONFIG_BT_DEVICE_NAME, ADV_NAME_STR_MAX_LEN) != 0) {
		return;
	}

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
	printk("Device found: %s (RSSI %d)\n", addr_str, rssi);

	if (bt_le_scan_stop()) {
		return;
	}

	struct bt_conn *unused_conn = NULL;

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,
				BT_LE_CONN_PARAM(8, 8, 0, 200), &unused_conn);
	if (err) {
		printk("Create conn to %s failed (%d)\n", addr_str, err);
		scan_start();
	}

	if (unused_conn) {
		bt_conn_unref(unused_conn);
	}
}

static void scan_start(void)
{
	int err;

	err = bt_le_scan_start(BT_LE_SCAN_ACTIVE_CONTINUOUS, device_found);
	if (err) {
		printk("Scanning failed to start (err %d)\n", err);
		return;
	}

	printk("Scanning started\n");
}

static uint8_t on_service_discover(struct bt_conn *conn,
	const struct bt_gatt_attr *attr,
	struct bt_gatt_discover_params *params)
{
	if (attr) {
		uint8_t conn_index = bt_conn_index(conn);

		conn_state[conn_index].timed_action_char_handle =
			bt_gatt_attr_value_handle(attr);
		printk("Service discovery completed\n");
	} else {
		printk("Service discovery failed\n");
	}

	return BT_GATT_ITER_STOP;
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		printk("Failed to connect to %s 0x%02x %s\n", addr, err, bt_hci_err_to_str(err));

		scan_start();
		return;
	}

	printk("Connected: %s\n", addr);

	uint8_t conn_index = bt_conn_index(conn);

	conn_state[conn_index].discovery_params.uuid = timed_char_uuid;
	conn_state[conn_index].discovery_params.func = on_service_discover;
	conn_state[conn_index].discovery_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	conn_state[conn_index].discovery_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	conn_state[conn_index].discovery_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

	err = bt_gatt_discover(conn, &conn_state[conn_index].discovery_params);
	if (err) {
		printk("Discovery failed, %d\n", err);
	}

	const uint8_t peripheral_conn_count = 1;

	conn_count++;
	if (conn_count < CONFIG_BT_MAX_CONN - peripheral_conn_count) {
		scan_start();
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s, reason 0x%02x %s\n", addr, reason, bt_hci_err_to_str(reason));

	conn_count--;

	uint8_t conn_index = bt_conn_index(conn);

	/* Reset state */
	memset(&conn_state[conn_index], 0, sizeof(conn_state[0]));

	const uint8_t peripheral_conn_count = 1;

	if (conn_count == CONFIG_BT_MAX_CONN - peripheral_conn_count - 1) {
		scan_start();
	}
}

static bool on_conn_param_req(struct bt_conn *conn, struct bt_le_conn_param *param)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(param);

	return false;
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
	.le_param_req = on_conn_param_req,
};

static bool on_vs_evt(struct net_buf_simple *buf)
{
	uint8_t *subevent_code;
	struct bt_conn *conn;
	sdc_hci_subevent_vs_conn_anchor_point_update_report_t *evt = NULL;

	subevent_code = net_buf_simple_pull_mem(
		buf,
		sizeof(*subevent_code));

	switch (*subevent_code) {
	case SDC_HCI_SUBEVENT_VS_CONN_ANCHOR_POINT_UPDATE_REPORT:
		evt = (void *)buf->data;
		break;
	default:
		return false;
	}

	if (!evt) {
		return false;
	}

	conn = bt_hci_conn_lookup_handle(evt->conn_handle);
	if (!conn) {
		return true;
	}

	uint8_t conn_index = bt_conn_index(conn);

	bt_conn_unref(conn);
	conn = NULL;

	if (atomic_test_and_set_bit(&conn_state[conn_index].last_anchor_point_in_use, 0)) {
		return true;
	}

	conn_state[conn_index].last_anchor_point_timestamp = evt->anchor_point_us;
	conn_state[conn_index].last_anchor_point_event_counter = evt->event_counter;

	atomic_clear_bit(&conn_state[conn_index].last_anchor_point_in_use, 0);

	return true;
}

void central_start(void)
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	err = bt_hci_register_vnd_evt_cb(on_vs_evt);
	if (err) {
		printk("Failed to register HCI VS callback\n");
		return;
	}

	sdc_hci_cmd_vs_conn_anchor_point_update_event_report_enable_t enable_params = {
		.enable = true
	};

	err = hci_vs_sdc_conn_anchor_point_update_event_report_enable(&enable_params);
	if (err) {
		printk("Failed to enable connection anchor point update events\n");
		return;
	}

	timed_led_toggle_init();

	bt_conn_cb_register(&conn_callbacks);

	scan_start();

	k_work_schedule(&timestamp_send_work, K_MSEC(LED_TOGGLE_PERIOD_MS));
}
