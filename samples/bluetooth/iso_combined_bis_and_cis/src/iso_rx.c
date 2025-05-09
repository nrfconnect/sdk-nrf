/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#include <zephyr/kernel.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/device.h>
#include "iso_combined_bis_and_cis.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_iso_rx, LOG_LEVEL_INF);

static iso_rx_process_sdu_data_cb_t process_rcvd_sdu;

static uint32_t rcvd_sdu_cnt;
static uint32_t missed_sdu_cnt;
static uint32_t empty_sdu_cnt;

static void iso_recv(struct bt_iso_chan *chan, const struct bt_iso_recv_info *info,
		     struct net_buf *buf);
static void iso_connected(struct bt_iso_chan *chan);
static void iso_disconnected(struct bt_iso_chan *chan, uint8_t reason);

static struct bt_iso_chan_ops iso_ops = {
	.recv = iso_recv,
	.connected = iso_connected,
	.disconnected = iso_disconnected,
};

static struct bt_iso_chan_io_qos iso_rx_qos = {
	.sdu = CONFIG_BT_ISO_TX_MTU,
	.phy = BT_GAP_LE_PHY_2M,
};

static struct bt_iso_chan_qos iso_qos = {
	.rx = &iso_rx_qos,
};

static struct bt_iso_chan iso_chan[] = {
	{
		.ops = &iso_ops,
		.qos = &iso_qos,
	},
};

static struct bt_iso_chan *iso_channels[] = {
	&iso_chan[0],
};

static void iso_recv(struct bt_iso_chan *chan, const struct bt_iso_recv_info *info,
		     struct net_buf *buf)
{
	if (info->flags & BT_ISO_FLAGS_VALID) {
		if (process_rcvd_sdu) {
			process_rcvd_sdu(buf);
		}
		rcvd_sdu_cnt++;
		if (buf->len == 0) {
			empty_sdu_cnt++;
		}
	} else {
		missed_sdu_cnt++;
	}

	uint32_t total_sdu_count = rcvd_sdu_cnt + missed_sdu_cnt;

	if (total_sdu_count % 100 == 0) {
		LOG_INF("Received SDU: %d, empty SDU: %d, missed SDU: %d", rcvd_sdu_cnt,
			empty_sdu_cnt, missed_sdu_cnt);
	}
}

static void iso_connected(struct bt_iso_chan *chan)
{
	const struct bt_iso_chan_path hci_path = {
		.pid = BT_ISO_DATA_PATH_HCI,
		.format = BT_HCI_CODING_FORMAT_TRANSPARENT,
	};

	int err;
	struct bt_conn_info conn_info;
	struct bt_iso_info iso_info;

	err = bt_iso_chan_get_info(chan, &iso_info);
	if (err) {
		LOG_ERR("Failed obtaining ISO info");
		return;
	}

	err = bt_conn_get_info(chan->iso, &conn_info);
	if (err) {
		LOG_ERR("Failed obtaining conn info");
		return;
	}

	LOG_INF("ISO RX Channel connected");

	err = bt_iso_setup_data_path(chan, BT_HCI_DATAPATH_DIR_CTLR_TO_HOST, &hci_path);
	if (err != 0) {
		LOG_ERR("Failed to setup ISO RX data path: %d", err);
	}
}

static void iso_disconnected(struct bt_iso_chan *chan, uint8_t reason)
{
	LOG_INF("ISO Channel disconnected with reason 0x%02x", reason);
}

void iso_rx_init(uint8_t retransmission_number, iso_rx_process_sdu_data_cb_t process_sdu_data_cb)
{
	iso_rx_qos.rtn = retransmission_number;
	process_rcvd_sdu = process_sdu_data_cb;
}

struct bt_iso_chan **iso_rx_channels_get(void)
{
	return iso_channels;
}
