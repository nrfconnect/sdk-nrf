/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>

static void scan_cb(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
		    struct net_buf_simple *ad)
{
	printk("%s: Device found: %s (RSSI %d), type %u, AD data len %u\n", __func__,
	       bt_addr_le_str(addr), rssi, type, ad->len);
}

static void scan_recv(const struct bt_le_scan_recv_info *info,
		      struct net_buf_simple *buf)
{
#if defined(CONFIG_BT_HCI_ADV_REPORT_CHAN_IDX)
	printk("%s: Device found: %s (chan_idx %u RSSI %d), type %u, AD data len %u\n", __func__,
	       bt_addr_le_str(info->addr), info->chan_idx, info->rssi, info->adv_type, buf->len);
#else /* !CONFIG_BT_HCI_ADV_REPORT_CHAN_IDX */
	printk("%s: Device found: %s (RSSI %d), type %u, AD data len %u\n", __func__,
	       bt_addr_le_str(info->addr), info->rssi, info->adv_type, buf->len);
#endif /* !CONFIG_BT_HCI_ADV_REPORT_CHAN_IDX */
}

static struct bt_le_scan_cb scan_callbacks = {
	.recv = scan_recv,
};

int main(void)
{
	struct bt_le_scan_param scan_param = {
		.type = BT_LE_SCAN_TYPE_PASSIVE,
		.options = BT_LE_SCAN_OPT_NONE,
		.interval = BT_GAP_SCAN_FAST_INTERVAL,
		.window = BT_GAP_SCAN_FAST_WINDOW,
	};
	int err;

	printk("LE Advertising Report with Channel Index sample\n");

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	bt_le_scan_cb_register(&scan_callbacks);

	printk("Starting scan\n");
	err = bt_le_scan_start(&scan_param, scan_cb);
	if (err) {
		printk("Scan start failed (err %d)\n", err);
		return 0;
	}

	return 0;
}
