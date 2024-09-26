/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/console/console.h>
#include <zephyr/sys/printk.h>
#include <string.h>
#include <stdlib.h>
#include <zephyr/types.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/crypto.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <bluetooth/services/throughput.h>
#include <bluetooth/scan.h>
#include <bluetooth/gatt_dm.h>

#include <zephyr/shell/shell_uart.h>

#include <dk_buttons_and_leds.h>

#include "bt_throughput_test.h"

#define CONN_LATENCY 0
#define SUPERVISION_TIMEOUT 1000

#define DEVICE_NAME	CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define THROUGHPUT_CONFIG_TIMEOUT 20

static K_SEM_DEFINE(throughput_sem, 0, 1);

static volatile bool data_length_req;
static volatile bool test_ready;
static struct bt_conn *default_conn;
static struct bt_throughput throughput;
static const struct bt_uuid *uuid128 = BT_UUID_THROUGHPUT;
static struct bt_gatt_exchange_params exchange_params;

static struct bt_le_conn_param *conn_param =
	BT_LE_CONN_PARAM(CONFIG_INTERVAL_MIN, CONFIG_INTERVAL_MAX, 0, 400);

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL,
		0xBB, 0x4A, 0xFF, 0x4F, 0xAD, 0x03, 0x41, 0x5D,
		0xA9, 0x6C, 0x9D, 0x6C, 0xDD, 0xDA, 0x83, 0x04),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static void button_handler_cb(uint32_t button_state, uint32_t has_changed);

static const char *phy2str(uint8_t phy)
{
	switch (phy) {
	case 0: return "No packets";
	case BT_GAP_LE_PHY_1M: return "LE 1M";
	case BT_GAP_LE_PHY_2M: return "LE 2M";
	case BT_GAP_LE_PHY_CODED: return "LE Coded";
	default: return "Unknown";
	}
}

static void instruction_print(void)
{
	/* printk("\nType 'config' to change the configuration parameters.\n");
	 * printk("You can use the Tab key to autocomplete your input.\n");
	 * printk("Type 'run' when you are ready to run the test.\n");
	 */
}

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

	printk("Filter not match. Address: %s connectable: %d\n",
				addr, connectable);
}

void scan_connecting_error(struct bt_scan_device_info *device_info)
{
	printk("Connecting failed\n");
}

BT_SCAN_CB_INIT(scan_cb, scan_filter_match, scan_filter_no_match,
		scan_connecting_error, NULL);

static void exchange_func(struct bt_conn *conn, uint8_t att_err,
			  struct bt_gatt_exchange_params *params)
{
	struct bt_conn_info info = {0};
	int err;

	printk("MTU exchange %s\n", att_err == 0 ? "successful" : "failed");

	err = bt_conn_get_info(conn, &info);
	if (err) {
		printk("Failed to get connection info %d\n", err);
		return;
	}

	if (info.role == BT_CONN_ROLE_CENTRAL) {
		instruction_print();
		test_ready = true;
	}
}

static void discovery_complete(struct bt_gatt_dm *dm,
			       void *context)
{
	int err;
	struct bt_throughput *throughput = context;

	printk("Service discovery completed\n");

	bt_gatt_dm_data_print(dm);
	bt_throughput_handles_assign(dm, throughput);
	bt_gatt_dm_data_release(dm);

	exchange_params.func = exchange_func;

	err = bt_gatt_exchange_mtu(default_conn, &exchange_params);
	if (err) {
		printk("MTU exchange failed (err %d)\n", err);
	} else {
		printk("MTU exchange pending\n");
	}
}

static void discovery_service_not_found(struct bt_conn *conn,
					void *context)
{
	printk("Service not found\n");
}

static void discovery_error(struct bt_conn *conn,
			    int err,
			    void *context)
{
	printk("Error while discovering GATT database: (%d)\n", err);
}

struct bt_gatt_dm_cb discovery_cb = {
	.completed         = discovery_complete,
	.service_not_found = discovery_service_not_found,
	.error_found       = discovery_error,
};

static void connected(struct bt_conn *conn, uint8_t hci_err)
{
	struct bt_conn_info info = {0};
	int err;

	if (hci_err) {
		if (hci_err == BT_HCI_ERR_UNKNOWN_CONN_ID) {
			/* Canceled creating connection */
			return;
		}

		printk("Connection failed (err 0x%02x)\n", hci_err);
		return;
	}

	if (default_conn) {
		printk("Connection exists, disconnect second connection\n");
		bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		return;
	}

	default_conn = bt_conn_ref(conn);

	err = bt_conn_get_info(default_conn, &info);
	if (err) {
		printk("Failed to get connection info %d\n", err);
		return;
	}

	printk("Connected as %s\n",
	       info.role == BT_CONN_ROLE_CENTRAL ? "central" : "peripheral");
	printk("Conn. interval is %u units\n", info.le.interval);

	if (info.role == BT_CONN_ROLE_CENTRAL) {
		err = bt_gatt_dm_start(default_conn,
				       BT_UUID_THROUGHPUT,
				       &discovery_cb,
				       &throughput);

		if (err) {
			printk("Discover failed (err %d)\n", err);
		}
	}
}

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
		.connect_if_match = 1,
		.scan_param = &scan_param,
		.conn_param = conn_param
	};

	bt_scan_init(&scan_init);
	bt_scan_cb_register(&scan_cb);

	err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, uuid128);
	if (err) {
		printk("Scanning filters cannot be set\n");

		return;
	}

	err = bt_scan_filter_enable(BT_SCAN_UUID_FILTER, false);
	if (err) {
		printk("Filters cannot be turned on\n");
	}
}

static void scan_start(void)
{
	int err;

	err = bt_scan_start(BT_SCAN_TYPE_SCAN_PASSIVE);
	if (err) {
		printk("Starting scanning failed (err %d)\n", err);
		return;
	}
}

static void adv_start(void)
{
	const struct bt_le_adv_param *adv_param =
		BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE |
				BT_LE_ADV_OPT_ONE_TIME,
				BT_GAP_ADV_FAST_INT_MIN_2,
				BT_GAP_ADV_FAST_INT_MAX_2,
				NULL);
	int err;

	err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), sd,
			      ARRAY_SIZE(sd));
	if (err) {
		printk("Failed to start advertiser (%d)\n", err);
		return;
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason 0x%02x)\n", reason);

	test_ready = false;
	if (default_conn) {
		bt_conn_unref(default_conn);
		default_conn = NULL;
	}
}

static bool le_param_req(struct bt_conn *conn, struct bt_le_conn_param *param)
{
	printk("Connection parameters update request received.\n");
	printk("Minimum interval: %d, Maximum interval: %d\n",
	       param->interval_min, param->interval_max);
	printk("Latency: %d, Timeout: %d\n", param->latency, param->timeout);

	return true;
}

static void le_param_updated(struct bt_conn *conn, uint16_t interval,
			     uint16_t latency, uint16_t timeout)
{
	printk("Connection parameters updated.\n"
	       " interval: %d, latency: %d, timeout: %d\n",
	       interval, latency, timeout);

	k_sem_give(&throughput_sem);
}

static void le_phy_updated(struct bt_conn *conn,
			   struct bt_conn_le_phy_info *param)
{
	printk("LE PHY updated: TX PHY %s, RX PHY %s\n",
	       phy2str(param->tx_phy), phy2str(param->rx_phy));

	k_sem_give(&throughput_sem);
}

static void le_data_length_updated(struct bt_conn *conn,
				   struct bt_conn_le_data_len_info *info)
{
	if (!data_length_req) {
		return;
	}

	printk("LE data len updated: TX (len: %d time: %d)"
	       " RX (len: %d time: %d)\n", info->tx_max_len,
	       info->tx_max_time, info->rx_max_len, info->rx_max_time);

	data_length_req = false;
	k_sem_give(&throughput_sem);
}

static uint8_t throughput_read(const struct bt_throughput_metrics *met)
{
	printk("[peer] received %u bytes (%u KB)"
	       " in %u GATT writes at %u bps\n",
	       met->write_len, met->write_len / 1024, met->write_count,
	       met->write_rate);

	k_sem_give(&throughput_sem);

	return BT_GATT_ITER_STOP;
}

static void throughput_received(const struct bt_throughput_metrics *met)
{
	static uint32_t kb;

	if (met->write_len == 0) {
		kb = 0;
		printk("\n");

		return;
	}

	if ((met->write_len / 1024) != kb) {
		kb = (met->write_len / 1024);
		printk("=");
	}
}

static void throughput_send(const struct bt_throughput_metrics *met)
{
	printk("\n[local] received %u bytes (%u KB)"
		" in %u GATT writes at %u bps\n",
		met->write_len, met->write_len / 1024,
		met->write_count, met->write_rate);
}

static const struct bt_throughput_cb throughput_cb = {
	.data_read = throughput_read,
	.data_received = throughput_received,
	.data_send = throughput_send
};

static struct button_handler button = {
	.cb = button_handler_cb,
};

void select_role(bool is_central)
{
	int err;
	static bool role_selected;

	if (role_selected) {
		printk("\nCannot change role after it was selected once.\n");
		return;
	} else if (is_central) {
		printk("\nCentral. Starting scanning\n");
		scan_start();
	} else {
		printk("\nPeripheral. Starting advertising\n");
		adv_start();
	}

	role_selected = true;

	/* The role has been selected, button are not needed any more. */
	err = dk_button_handler_remove(&button);
	if (err) {
		printk("Button disable error: %d\n", err);
	}
}

static void button_handler_cb(uint32_t button_state, uint32_t has_changed)
{
	ARG_UNUSED(has_changed);

	if (button_state & DK_BTN1_MSK) {
		select_role(true);
	} else if (button_state & DK_BTN2_MSK) {
		select_role(false);
	}
}

static void buttons_init(void)
{
	int err;

	err = dk_buttons_init(NULL);
	if (err) {
		printk("Buttons initialization failed.\n");
		return;
	}

	/* Add dynamic buttons handler. Buttons should be activated only when
	 * during the board role choosing.
	 */
	dk_button_handler_add(&button);
}

int connection_configuration_set(const struct bt_le_conn_param *conn_param,
			const struct bt_conn_le_phy_param *phy,
			const struct bt_conn_le_data_len_param *data_len)
{
	int err;
	struct bt_conn_info info = {0};

	err = bt_conn_get_info(default_conn, &info);
	if (err) {
		printk("Failed to get connection info %d", err);
		return err;
	}

	if (info.role != BT_CONN_ROLE_CENTRAL) {
		printk("'run' command shall be executed only on the central board");
	}

	err = bt_conn_le_phy_update(default_conn, phy);
	if (err) {
		printk("PHY update failed: %d\n", err);
		return err;
	}

	printk("PHY update pending");
	err = k_sem_take(&throughput_sem, K_SECONDS(THROUGHPUT_CONFIG_TIMEOUT));
	if (err) {
		printk("PHY update timeout");
		return err;
	}

	if (info.le.data_len->tx_max_len != data_len->tx_max_len) {
		data_length_req = true;

		err = bt_conn_le_data_len_update(default_conn, data_len);
		if (err) {
			printk("LE data length update failed: %d",
				    err);
			return err;
		}

		printk("LE Data length update pending");
		err = k_sem_take(&throughput_sem, K_SECONDS(THROUGHPUT_CONFIG_TIMEOUT));
		if (err) {
			printk("LE Data Length update timeout");
			return err;
		}
	}

	if (info.le.interval != conn_param->interval_max) {
		err = bt_conn_le_param_update(default_conn, conn_param);
		if (err) {
			printk("Connection parameters update failed: %d",
				    err);
			return err;
		}

		printk("Connection parameters update pending");
		err = k_sem_take(&throughput_sem, K_SECONDS(THROUGHPUT_CONFIG_TIMEOUT));
		if (err) {
			printk("Connection parameters update timeout");
			return err;
		}
	}

	return 0;
}

int bt_throughput_test_run(void)
{
	int err;
	uint64_t stamp;
	int64_t delta;
	uint32_t data = 0;

	/* a dummy data buffer */
	static char dummy[495];

	if (!default_conn) {
		printk("Device is disconnected %s",
			    "Connect to the peer device before running test");
		return -EFAULT;
	}

	if (!test_ready) {
		printk("Device is not ready."
			"Please wait for the service discovery and MTU exchange end");
		return 0;
	}

	printk("\n==== Starting throughput test ====\n");

	/* reset peer metrics */
	err = bt_throughput_write(&throughput, dummy, 1);
	if (err) {
		printk("Reset peer metrics failed.");
		return err;
	}

	/* get cycle stamp */
	stamp = k_uptime_get_32();

	delta = 0;
	while (true) {
		err = bt_throughput_write(&throughput, dummy, 495);
		if (err) {
			printk("GATT write failed (err %d)", err);
			break;
		}
		data += 495;
		if (k_uptime_get_32() - stamp > CONFIG_BLE_TEST_DURATION) {
			break;
		}
	}

	delta = k_uptime_delta(&stamp);

	printk("\nDone\n");
	printk("[local] sent %u bytes (%u KB) in %lld ms at %llu kbps\n",
	       data, data / 1024, delta, ((uint64_t)data * 8 / delta));

	/* read back char from peer */
	err = bt_throughput_read(&throughput);
	if (err) {
		printk("GATT read failed (err %d)", err);
		return err;
	}

	k_sem_take(&throughput_sem, K_SECONDS(THROUGHPUT_CONFIG_TIMEOUT));

	instruction_print();

	return 0;
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.le_param_req = le_param_req,
	.le_param_updated = le_param_updated,
	.le_phy_updated = le_phy_updated,
	.le_data_len_updated = le_data_length_updated
};

int bt_throughput_test_init(void)
{
	int err;
	int64_t stamp;

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return err;
	}

	printk("Bluetooth initialized\n");

	scan_init();

	err = bt_throughput_init(&throughput, &throughput_cb);
	if (err) {
		printk("Throughput service initialization failed.\n");
		return err;
	}

	buttons_init();

	select_role(true);

	printk("Waiting for connection.\n");
	stamp = k_uptime_get_32();
	while (k_uptime_delta(&stamp) / MSEC_PER_SEC < THROUGHPUT_CONFIG_TIMEOUT) {
		if (default_conn) {
			break;
		}
		k_sleep(K_SECONDS(1));
	}

	if (!default_conn) {
		printk("Cannot set up connection.\n");
		return -ENOTCONN;
	}
	return connection_configuration_set(
			BT_LE_CONN_PARAM(CONFIG_INTERVAL_MIN,
			CONFIG_INTERVAL_MAX,
			CONN_LATENCY, SUPERVISION_TIMEOUT),
			BT_CONN_LE_PHY_PARAM_2M,
			BT_LE_DATA_LEN_PARAM_MAX);
}

int bt_throughput_test_exit(void)
{
	int err;

	if (!default_conn) {
		printk("Not connected!\n");
		return -ENOTCONN;
	}

	err = bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	if (err) {
		printk("Cannot disconnect!\n");
		return err;
	}
	return 0;
}
