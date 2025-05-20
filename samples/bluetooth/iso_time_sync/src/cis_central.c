/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** CIS central implementation for the time sync sample
 *
 * This file implements a device that acts as a CIS central.
 * The central can either be configured as a transmitter or a receiver.
 *
 * It connects to as many peripherals as it has available ISO channels.
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/bluetooth/hci.h>

#include "iso_time_sync.h"

#define ADV_NAME_STR_MAX_LEN (sizeof(CONFIG_BT_DEVICE_NAME))

static bool configured_for_tx;

static K_SEM_DEFINE(sem_connected, 0, 1);

static void scan_start(void);

static struct bt_iso_chan *free_iso_chan_find(void)
{
	const size_t max_channels = configured_for_tx ? CONFIG_BT_ISO_MAX_CHAN : 1;

	struct bt_iso_chan **iso_channels;

	if (configured_for_tx) {
		iso_channels = iso_tx_channels_get();
	} else {
		iso_channels = iso_rx_channels_get();
	}

	struct bt_iso_chan *iso_chan = NULL;

	for (size_t i = 0; i < max_channels; i++) {
		if (iso_channels[i]->state == BT_ISO_STATE_DISCONNECTED) {
			iso_chan = iso_channels[i];
			break;
		}
	}

	return iso_chan;
}

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

static void scan_recv(const struct bt_le_scan_recv_info *info, struct net_buf_simple *buf)
{
	char name_str[ADV_NAME_STR_MAX_LEN] = {0};

	bt_data_parse(buf, adv_data_parse_cb, name_str);

	if (strncmp(name_str, CONFIG_BT_DEVICE_NAME, ADV_NAME_STR_MAX_LEN) != 0) {
		return;
	}

	if (bt_le_scan_stop()) {
		return;
	}

	struct bt_conn *conn = NULL;
	int err = bt_conn_le_create(info->addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT,
				    &conn);
	if (err) {
		printk("Create conn to %s failed (%d)\n", name_str, err);
	} else {
		bt_conn_unref(conn);
	}
}

static struct bt_le_scan_cb scan_callbacks = {
	.recv = scan_recv,
};

static void scan_start(void)
{
	int err;

	if (free_iso_chan_find()) {
		err = bt_le_scan_start(BT_LE_SCAN_ACTIVE_CONTINUOUS, NULL);
		if (err == -EALREADY) {
			/** If the central is RXing, both the ISO and the ACL
			 * disconnection callbacks try to enable the scanner.
			 * If the ISO channel disconnects before the ACL
			 * connection, the application will attempt to enable
			 * the scanner again.
			 */
			printk("Scanning did not start because it has already started (err %d)\n",
				   err);
		} else if (err) {
			printk("Scanning failed to start (err %d)\n", err);
			return;
		}

		printk("CIS Central started scanning\n");
	}
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

	/* Find a free ISO channel */
	struct bt_iso_chan *iso_chan = free_iso_chan_find();

	if (iso_chan == NULL) {
		printk("No ISO channel available\n");
		return;
	}

	struct bt_iso_connect_param connect_param;

	connect_param.acl = conn;
	connect_param.iso_chan = iso_chan;

	err = bt_iso_chan_connect(&connect_param, 1);
	if (err) {
		printk("Failed to connect iso (%d)\n", err);
		return;
	}
	printk("Connecting ISO channel\n");
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s, reason 0x%02x %s\n", addr, reason, bt_hci_err_to_str(reason));

	scan_start();
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

void cis_central_start(bool do_tx, uint8_t retransmission_number, uint16_t max_transport_latency_ms)
{
	struct bt_iso_cig_param param;
	struct bt_iso_cig *cig;
	int err;

	configured_for_tx = do_tx;
	if (do_tx) {
		/** scan_start is registered as a callback that is triggered when an
		 * ISO channel connects to avoid a situation where the
		 * LE HCI Create CIS command is called while there is a pending CIS
		 * connection. The CIS central starts scanning for a new connection only after a
		 * pending CIS connection has completed. This must be done because the
		 * LE HCI Create CIS command is disallowed while a CIS connection is
		 * pending. See Bluetooth Core Specification, Vol 6, Part B, Section 7.8.99.
		 */
		iso_tx_init(retransmission_number, &scan_start);
	} else {
		/** scan_start is registered as a callback that is triggered when an
		 * ISO channel disconnects. This is needed because the ISO channel might
		 * disconnect after the ACL has completed its disconnection callback. If
		 * this is the case, there are no RX ISO channels available during the
		 * ACL disconnection callback, which will prevent scanning from starting.
		 */
		iso_rx_init(retransmission_number, &scan_start);
	}

	bt_le_scan_cb_register(&scan_callbacks);
	bt_conn_cb_register(&conn_callbacks);

	memset(&param, 0, sizeof(param));
	param.cis_channels = do_tx ? iso_tx_channels_get() : iso_rx_channels_get();
	param.num_cis = do_tx ? CONFIG_BT_ISO_MAX_CHAN : 1;
	param.sca = BT_GAP_SCA_UNKNOWN;
	param.packing = 0;
	param.framing = 0;
	param.c_to_p_latency = max_transport_latency_ms;
	param.p_to_c_latency = max_transport_latency_ms;
	param.c_to_p_interval = CONFIG_SDU_INTERVAL_US;
	param.p_to_c_interval = CONFIG_SDU_INTERVAL_US;

	err = bt_iso_cig_create(&param, &cig);
	if (err != 0) {
		printk("Failed to create CIG (%d)\n", err);
		return;
	}

	scan_start();

	printk("CIS central started scanning for peripheral(s)\n");
}
