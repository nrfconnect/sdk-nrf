/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>

#include <zephyr/console/console.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <bluetooth/hci_vs_sdc.h>
#include <bluetooth/scan.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>

LOG_MODULE_REGISTER(app_main, LOG_LEVEL_INF);

#define DATA_CHANNEL_COUNT	      37
#define CHANNEL_CLASSIFICATION_SIZE   10
#define CHANNEL_MAP_COUNT	      3
#define CHANNEL_MAP_UPDATE_PERIOD     K_SECONDS(5)
#define REPORTING_MIN_SPACING	      5U  /* 1 second in 200 ms units */
#define REPORTING_MAX_DELAY	      5U  /* 1 second in 200 ms units */

static struct bt_conn *default_conn;
static uint8_t device_role;
static uint8_t channel_map_index;
static struct k_work_delayable channel_map_update_work;
static struct k_work central_channel_map_apply_work;
static uint8_t pending_central_chan_map[5];

static const uint8_t channel_maps[CHANNEL_MAP_COUNT][5] = {
	{0xFF, 0xFF, 0xFF, 0xFF, 0x1F},
	{0xFF, 0xFB, 0xEF, 0xBF, 0x1F},
	{0xDF, 0x7F, 0xFF, 0xFD, 0x1F},
};

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static const struct bt_le_conn_param conn_param = {
	.interval_min = 0x0018,
	.interval_max = 0x0018,
	.latency = 0,
	.timeout = 400,
};

static void role_activity_start(void);

static void channel_map_set_bit(uint8_t map[5], uint8_t channel, bool usable)
{
	if (channel >= DATA_CHANNEL_COUNT) {
		return;
	}

	if (usable) {
		map[channel / 8] |= BIT(channel % 8);
	} else {
		map[channel / 8] &= ~BIT(channel % 8);
	}
}

static void channel_map_print(const char *prefix, const uint8_t map[5])
{
	LOG_INF("%s %02x %02x %02x %02x %02x", prefix, map[0], map[1], map[2], map[3],
		map[4] & 0x1F);
}

static void
classification_to_channel_map(const uint8_t classification[CHANNEL_CLASSIFICATION_SIZE],
			      uint8_t map[5])
{
	memset(map, 0, sizeof(map[0]) * 5);

	for (uint8_t channel = 0; channel < DATA_CHANNEL_COUNT; channel++) {
		const uint8_t byte_index = (channel * 2U) / 8U;
		const uint8_t bit_offset = (channel * 2U) % 8U;
		const uint8_t value = (classification[byte_index] >> bit_offset) & 0x03U;
		const bool usable = (value == 0x00U) || (value == 0x01U);

		channel_map_set_bit(map, channel, usable);
	}

	map[4] &= 0x1F;
}

static void peripheral_channel_map_update(struct k_work *work)
{
	uint8_t map[5];
	int err;

	if (!default_conn) {
		return;
	}

	memcpy(map, channel_maps[channel_map_index], sizeof(map));

	LOG_INF("Using channel map %u", channel_map_index);
	channel_map_print("Peripheral channel map", map);

	err = bt_le_set_chan_map(map);
	if (err) {
		LOG_WRN("bt_le_set_chan_map failed (err %d)", err);
	}

	channel_map_index = (channel_map_index + 1U) % CHANNEL_MAP_COUNT;

	k_work_schedule(k_work_delayable_from_work(work), CHANNEL_MAP_UPDATE_PERIOD);
}

static void central_channel_map_apply_work_fn(struct k_work *work)
{
	uint8_t map[5];
	int err;

	ARG_UNUSED(work);

	if (!default_conn || device_role != BT_CONN_ROLE_CENTRAL) {
		return;
	}

	memcpy(map, pending_central_chan_map, sizeof(map));

	err = bt_le_set_chan_map(map);
	if (err) {
		LOG_WRN("Central bt_le_set_chan_map failed (err %d)", err);
	} else {
		LOG_INF("Central applied peripheral channel classification");
	}
}

static void central_channel_map_apply(const uint8_t map[5])
{
	memcpy(pending_central_chan_map, map, sizeof(pending_central_chan_map));
	k_work_submit(&central_channel_map_apply_work);
}

static bool on_vs_evt(struct net_buf_simple *buf)
{
	uint8_t subevent_code;

	subevent_code = net_buf_simple_pull_u8(buf);

	switch (subevent_code) {
	case SDC_HCI_SUBEVENT_VS_CHANNEL_CLASSIFICATION_REPORT: {
		sdc_hci_subevent_vs_channel_classification_report_t *evt = (void *)buf->data;
		struct bt_conn *conn;
		uint8_t map[5];

		conn = bt_hci_conn_lookup_handle(evt->conn_handle);
		if (!conn) {
			return true;
		}

		classification_to_channel_map(evt->channel_classification, map);
		channel_map_print("Central channel map from report", map);
		central_channel_map_apply(map);

		bt_conn_unref(conn);
		return true;
	}
	case SDC_HCI_SUBEVENT_VS_CHANNEL_REPORTING_ENABLE_COMPLETE: {
		sdc_hci_subevent_vs_channel_reporting_enable_complete_t *evt = (void *)buf->data;

		if (evt->status == BT_HCI_ERR_SUCCESS) {
			LOG_INF("Channel classification reporting enabled");
		} else {
			LOG_WRN("Channel classification reporting enable failed: 0x%02x %s",
				evt->status, bt_hci_err_to_str(evt->status));
		}
		return true;
	}
	default:
		return false;
	}
}

static int central_reporting_start(struct bt_conn *conn)
{
	sdc_hci_cmd_vs_channel_reporting_enable_t params;
	uint16_t conn_handle;
	int err;

	err = bt_hci_get_conn_handle(conn, &conn_handle);
	if (err) {
		return err;
	}

	params.conn_handle = conn_handle;
	params.enable = true;
	params.min_spacing = REPORTING_MIN_SPACING;
	params.max_delay = REPORTING_MAX_DELAY;

	return hci_vs_sdc_channel_reporting_enable(&params);
}

static void scan_filter_match(struct bt_scan_device_info *device_info,
			      struct bt_scan_filter_match *filter_match, bool connectable)
{
	char addr[BT_ADDR_LE_STR_LEN];

	ARG_UNUSED(filter_match);

	bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));
	LOG_INF("Filters matched. Address: %s connectable: %d", addr, connectable);
}

static void scan_connecting_error(struct bt_scan_device_info *device_info)
{
	ARG_UNUSED(device_info);

	LOG_WRN("Connection failed");
	role_activity_start();
}

BT_SCAN_CB_INIT(scan_cb, scan_filter_match, NULL, scan_connecting_error, NULL);

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
		.conn_param = &conn_param,
	};

	bt_scan_init(&scan_init);
	bt_scan_cb_register(&scan_cb);

	err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_NAME, CONFIG_BT_DEVICE_NAME);
	if (err) {
		LOG_WRN("Scanning filters cannot be set (err %d)", err);
		return;
	}

	err = bt_scan_filter_enable(BT_SCAN_NAME_FILTER, false);
	if (err) {
		LOG_WRN("Filters cannot be turned on (err %d)", err);
	}
}

static void scan_start(void)
{
	int err;

	err = bt_scan_start(BT_SCAN_TYPE_SCAN_PASSIVE);
	if (err) {
		LOG_WRN("Starting scanning failed (err %d)", err);
		return;
	}

	LOG_INF("Scanning started");
}

static void adv_start(void)
{
	int err;
	const struct bt_le_adv_param *adv_param = BT_LE_ADV_PARAM(
		BT_LE_ADV_OPT_CONN, BT_GAP_ADV_FAST_INT_MIN_2, BT_GAP_ADV_FAST_INT_MAX_2, NULL);

	err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		LOG_WRN("Advertising failed to start (err %d)", err);
		return;
	}

	LOG_INF("Advertising started");
}

static void role_activity_start(void)
{
	if (device_role == BT_CONN_ROLE_CENTRAL) {
		scan_start();
	} else {
		adv_start();
	}
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	int ret;

	if (err) {
		LOG_WRN("Connection failed, error 0x%02x %s", err, bt_hci_err_to_str(err));
		role_activity_start();
		return;
	}

	default_conn = bt_conn_ref(conn);

	LOG_INF("Connected as %s", device_role == BT_CONN_ROLE_CENTRAL ? "central" : "peripheral");

	if (device_role == BT_CONN_ROLE_CENTRAL) {
		ret = central_reporting_start(default_conn);
		if (ret) {
			LOG_WRN("Failed to start channel classification reporting (error %d)", ret);
		}
	} else {
		channel_map_index = 0;
		k_work_reschedule(&channel_map_update_work, CHANNEL_MAP_UPDATE_PERIOD);
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	ARG_UNUSED(conn);

	LOG_INF("Disconnected, reason 0x%02x %s", reason, bt_hci_err_to_str(reason));

	k_work_cancel_delayable(&channel_map_update_work);
	k_work_cancel(&central_channel_map_apply_work);
	channel_map_index = 0;

	if (default_conn) {
		bt_conn_unref(default_conn);
		default_conn = NULL;
	}

	role_activity_start();
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

int main(void)
{
	int err;

	console_init();

	LOG_INF("Starting Bluetooth Channel Classification sample");

	k_work_init_delayable(&channel_map_update_work, peripheral_channel_map_update);
	k_work_init(&central_channel_map_apply_work, central_channel_map_apply_work_fn);

	err = bt_enable(NULL);
	if (err) {
		LOG_WRN("Bluetooth init failed (err %d)", err);
		return 0;
	}

	LOG_INF("Bluetooth initialized");

	while (true) {
		LOG_INF("Choose device role - type c (central) or p (peripheral): ");

		const char input_char = console_getchar();

		LOG_INF("");

		if (input_char == 'c') {
			LOG_INF("Selected Central");

			err = bt_hci_register_vnd_evt_cb(on_vs_evt);
			if (err) {
				LOG_WRN("Failed to register HCI VS callback (err %d)", err);
				continue;
			}

			device_role = BT_CONN_ROLE_CENTRAL;
			scan_init();
			scan_start();
			break;
		}

		if (input_char == 'p') {
			LOG_INF("Selected Peripheral");
			device_role = BT_CONN_ROLE_PERIPHERAL;
			adv_start();
			break;
		}

		LOG_INF("Invalid role");
	}

	return 0;
}
