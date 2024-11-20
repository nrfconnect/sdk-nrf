/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** CIS peripheral implementation for the time sync sample
 *
 * This file implements a device that acts as a CIS peripheral.
 * The peripheral can either be configured as a transmitter or a receiver.
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/bluetooth/hci.h>

#include "iso_time_sync.h"

static bool configured_for_tx;

static const struct bt_data ad[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		printk("Failed to connect to %s 0x%02x %s\n", addr, err, bt_hci_err_to_str(err));
		return;
	}

	printk("Connected: %s\n", addr);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected from %s, reason 0x%02x %s\n", addr, reason, bt_hci_err_to_str(reason));
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

static int iso_accept(const struct bt_iso_accept_info *info, struct bt_iso_chan **chan)
{
	printk("Incoming request from %p\n", (void *)info->acl);

	struct bt_iso_chan **iso_channels =
		configured_for_tx ? iso_tx_channels_get() : iso_rx_channels_get();

	if (iso_channels[0]->iso) {
		printk("No channels available\n");
		return -ENOMEM;
	}

	*chan = iso_channels[0];

	return 0;
}

static struct bt_iso_server iso_server = {
#if defined(CONFIG_BT_SMP)
	.sec_level = BT_SECURITY_L1,
#endif /* CONFIG_BT_SMP */
	.accept = iso_accept,
};

void cis_peripheral_start(bool do_tx)
{
	int err;
	uint8_t unused_rtn = 0;

	configured_for_tx = do_tx;

	/* It is the central that sets the RTN */
	if (do_tx) {
		iso_tx_init(unused_rtn, NULL);
	} else {
		iso_rx_init(unused_rtn, NULL);
	}

	bt_conn_cb_register(&conn_callbacks);

	err = bt_iso_server_register(&iso_server);
	if (err) {
		printk("Unable to register ISO server (err %d)\n", err);
		return;
	}

	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_2, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("CIS peripheral started advertising\n");
}
