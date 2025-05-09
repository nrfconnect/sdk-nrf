/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/bluetooth/iso.h>
#include <sdc_hci_vs.h>
#include <sdc_hci.h>

#include "iso_combined_bis_and_cis.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_iso_tx, LOG_LEVEL_INF);

static void iso_sent(struct bt_iso_chan *chan);
static void iso_connected(struct bt_iso_chan *chan);
static void iso_disconnected(struct bt_iso_chan *chan, uint8_t reason);

NET_BUF_POOL_FIXED_DEFINE(tx_pool, 1, BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static uint32_t num_sdus_sent;
static iso_tx_set_sdu_data_cb_t set_sdu_data;

static struct bt_iso_chan_ops iso_ops = {
	.connected = iso_connected,
	.sent = iso_sent,
	.disconnected = iso_disconnected,
};

static struct bt_iso_chan_io_qos iso_tx_qos = {
	.sdu = CONFIG_BT_ISO_TX_MTU,
	.phy = BT_GAP_LE_PHY_2M,
};

static struct bt_iso_chan_qos iso_qos = {
	.tx = &iso_tx_qos,
};

/* Create lists of ISO channels and pointers to ISO channels. */
static struct bt_iso_chan iso_channels[] = {{.ops = &iso_ops, .qos = &iso_qos}};

static struct bt_iso_chan *iso_channel_pointers[] = {&iso_channels[0]};

/* Store info about the ISO channels in use. */
static uint8_t roles[1];
static struct bt_iso_info iso_infos[1];

static int send_next_sdu(struct bt_iso_chan *chan)
{
	struct net_buf *buf;
	int ret = 0;

	buf = net_buf_alloc(&tx_pool, K_FOREVER);
	if (!buf) {
		LOG_ERR("Data buffer allocate timeout");
		ret = -ENOMEM;
	}

	if (set_sdu_data && !ret) {
		net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);
		set_sdu_data(buf);
		ret = bt_iso_chan_send(chan, buf, 0);
		if (ret < 0) {
			LOG_ERR("Unable to send data");
			net_buf_unref(buf);
		}
	}

	return ret;
}

static void send_next_sdu_on_a_channel(void)
{
	int err;

	if (iso_channels[0].state != BT_ISO_STATE_CONNECTED) {
		LOG_ERR("Failed sending SDU, stream is not connected");
		return;
	}

	err = send_next_sdu(&iso_channels[0]);
	if (err) {
		LOG_ERR("Failed sending SDU, counter %d", num_sdus_sent);
		return;
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
	uint8_t chan_index = ARRAY_INDEX(iso_channels, chan);

	err = bt_iso_chan_get_info(chan, &iso_infos[chan_index]);
	if (err) {
		LOG_ERR("Failed obtaining ISO info");
		return;
	}

	err = bt_conn_get_info(chan->iso, &conn_info);
	if (err) {
		LOG_ERR("Failed obtaining conn info");
		return;
	}

	roles[chan_index] = conn_info.role;

	/* Send the first SDU immediately.
	 */
	send_next_sdu_on_a_channel();
	LOG_INF("ISO TX Channel connected");

	err = bt_iso_setup_data_path(chan, BT_HCI_DATAPATH_DIR_HOST_TO_CTLR, &hci_path);
	if (err != 0) {
		LOG_ERR("Failed to setup ISO TX data path: %d", err);
	}
}

static void iso_disconnected(struct bt_iso_chan *chan, uint8_t reason)
{
	LOG_INF("ISO Channel index %d disconnected with reason 0x%02x",
		ARRAY_INDEX(iso_channels, chan), reason);
}

static void iso_sent(struct bt_iso_chan *chan)
{
	if (num_sdus_sent % 100 == 0) {
		LOG_INF("Sent SDU, counter: %d", num_sdus_sent);
	}

	num_sdus_sent++;

	send_next_sdu_on_a_channel();
}

void iso_tx_init(uint8_t retransmission_number, iso_tx_set_sdu_data_cb_t set_sdu_data_cb)
{
	iso_tx_qos.rtn = retransmission_number;
	set_sdu_data = set_sdu_data_cb;
}

struct bt_iso_chan **iso_tx_channels_get(void)
{
	return iso_channel_pointers;
}
