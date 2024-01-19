/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "bt_le_audio_tx.h"

#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/zbus/zbus.h>
#include <../subsys/bluetooth/audio/bap_iso.h>
#include <../subsys/bluetooth/audio/bap_stream.h>

#include "nrf5340_audio_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_le_audio_tx, CONFIG_BLE_LOG_LEVEL);

ZBUS_CHAN_DEFINE(sdu_ref_chan, struct sdu_ref_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
		 ZBUS_MSG_INIT(0));

#ifdef CONFIG_BT_BAP_UNICAST_SERVER
#define SRC_STREAM_COUNT CONFIG_BT_ASCS_ASE_SRC_COUNT
#elif CONFIG_BT_BAP_UNICAST_CLIENT
#define SRC_STREAM_COUNT CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT
#elif CONFIG_BT_BAP_BROADCAST_SOURCE
#define SRC_STREAM_COUNT CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT
#else
#define SRC_STREAM_COUNT 0
#endif

#define HCI_ISO_BUF_ALLOC_PER_CHAN 2

/* For being able to dynamically define iso_tx_pools */
#define NET_BUF_POOL_ITERATE(i, _)                                                                 \
	NET_BUF_POOL_FIXED_DEFINE(iso_tx_pool_##i, HCI_ISO_BUF_ALLOC_PER_CHAN,                     \
				  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU), 8, NULL);
#define NET_BUF_POOL_PTR_ITERATE(i, ...) IDENTITY(&iso_tx_pool_##i)
LISTIFY(SRC_STREAM_COUNT, NET_BUF_POOL_ITERATE, (;))
/* clang-format off */
static struct net_buf_pool *iso_tx_pools[] = { LISTIFY(SRC_STREAM_COUNT,
						       NET_BUF_POOL_PTR_ITERATE, (,)) };
/* clang-format on */

struct bt_le_audio_tx_info {
	struct bt_iso_tx_info iso_tx;
	struct net_buf_pool *iso_tx_pool;
	atomic_t iso_tx_pool_alloc;
	bool hci_wrn_printed;
};

static bool initialized;
static struct bt_le_audio_tx_info tx_info[SRC_STREAM_COUNT];

static int iso_stream_send(uint8_t const *const data, size_t size, struct bt_bap_stream *bap_stream,
			   struct bt_le_audio_tx_info *tx_info)
{
	int ret;
	struct net_buf *buf;

	/* net_buf_alloc allocates buffers for APP->NET transfer over HCI RPMsg,
	 * but when these buffers are released it is not guaranteed that the
	 * data has actually been sent. The data might be queued on the NET core,
	 * and this can cause delays in the audio.
	 * When the sent callback is called the data has been sent, and we can free the buffer.
	 * Data will be discarded if allocation becomes too high, to avoid audio delays.
	 * If the NET and APP core operates in clock sync, discarding should not occur.
	 */
	if (atomic_get(&tx_info->iso_tx_pool_alloc) >= HCI_ISO_BUF_ALLOC_PER_CHAN) {
		if (!tx_info->hci_wrn_printed) {
			struct bt_iso_chan *iso_chan;

			iso_chan = bt_bap_stream_iso_chan_get(bap_stream);

			LOG_WRN("HCI ISO TX overrun on stream %p - Single print",
				(void *)bap_stream);
			tx_info->hci_wrn_printed = true;
		}
		return -ENOMEM;
	}

	tx_info->hci_wrn_printed = false;

	buf = net_buf_alloc(tx_info->iso_tx_pool, K_NO_WAIT);
	if (buf == NULL) {
		/* This should never occur because of the iso_tx_pool_alloc check above */
		LOG_WRN("Out of TX buffers");
		return -ENOMEM;
	}

	net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);
	net_buf_add_mem(buf, data, size);

	atomic_inc(&tx_info->iso_tx_pool_alloc);

	ret = bt_bap_stream_send(bap_stream, buf, tx_info->iso_tx.seq_num, BT_ISO_TIMESTAMP_NONE);
	if (ret < 0) {
		LOG_WRN("Failed to send audio data: %d", ret);
		net_buf_unref(buf);
		atomic_dec(&tx_info->iso_tx_pool_alloc);
	} else {
		tx_info->iso_tx.seq_num++;
	}

	return 0;
}

int bt_le_audio_tx_send(struct bt_bap_stream **bap_streams, struct le_audio_encoded_audio enc_audio,
			uint8_t streams_to_tx)
{
	int ret;
	uint32_t ts = 0;
	bool any_chan_streaming = false;
	bool got_tx_sync_ts = false;
	size_t data_size_pr_stream = 0;

	if (!initialized) {
		return -EACCES;
	}

	if (bap_streams == NULL) {
		return -EINVAL;
	}

	if (streams_to_tx == 0) {
		LOG_INF("No active streams");
		return 0;
	}

	if (streams_to_tx > SRC_STREAM_COUNT) {
		return -ENOMEM;
	}

	if ((enc_audio.num_ch == 1) || (enc_audio.num_ch == streams_to_tx)) {
		data_size_pr_stream = enc_audio.size / enc_audio.num_ch;
	} else {
		LOG_ERR("Num encoded channels must be 1 or equal to num streams");
		return -EINVAL;
	}

	if (data_size_pr_stream != LE_AUDIO_SDU_SIZE_OCTETS(CONFIG_LC3_BITRATE)) {
		LOG_ERR("The encoded data size does not match the SDU size");
		return -EINVAL;
	}

	for (int i = 0; i < streams_to_tx; i++) {
		if (le_audio_ep_state_check(bap_streams[i]->ep, BT_BAP_EP_STATE_STREAMING)) {
			any_chan_streaming = true;

			if (!got_tx_sync_ts) {
				struct bt_iso_chan *iso_chan;

				iso_chan = bt_bap_stream_iso_chan_get(bap_streams[i]);
				if (iso_chan == NULL) {
					LOG_ERR("Unable to get iso chan");
					continue;
				}

				ret = bt_iso_chan_get_tx_sync(iso_chan, &tx_info[i].iso_tx);
				if (ret) {
					continue;
				}
				ts = tx_info[i].iso_tx.ts;
				got_tx_sync_ts = true;
			}
		}
	}

	if (!any_chan_streaming) {
		LOG_DBG("No headset in stream state");
		return -ECANCELED;
	}

	if (ts != 0 && got_tx_sync_ts) {
		if (!IS_ENABLED(CONFIG_STREAM_BIDIRECTIONAL)) {
			struct sdu_ref_msg msg;

			msg.timestamp = ts;
			msg.adjust = true;

			ret = zbus_chan_pub(&sdu_ref_chan, &msg, K_NO_WAIT);
			if (ret) {
				LOG_WRN("Failed to publish timestamp: %d", ret);
			}
		}
	}

	for (int i = 0; i < streams_to_tx; i++) {
		if (le_audio_ep_state_check(bap_streams[i]->ep, BT_BAP_EP_STATE_STREAMING)) {
			/* Check if same audio is sent to all channels */
			if (enc_audio.num_ch == 1) {
				ret = iso_stream_send(enc_audio.data, data_size_pr_stream,
						      bap_streams[i], &tx_info[i]);
			} else {
				ret = iso_stream_send(&enc_audio.data[data_size_pr_stream * i],
						      data_size_pr_stream, bap_streams[i],
						      &tx_info[i]);
			}

			if (ret) {
				/* DBG used here as prints are handled within iso_stream_send */
				LOG_DBG("Failed to send to idx: %d stream: %p, ret: %d ", i,
					bap_streams[i], ret);
			}
		}
	}

	return 0;
}

int bt_le_audio_tx_stream_stopped(uint8_t stream_idx)
{
	if (!initialized) {
		return -EACCES;
	}

	atomic_clear(&tx_info[stream_idx].iso_tx_pool_alloc);
	tx_info[stream_idx].hci_wrn_printed = false;
	return 0;
}

int bt_le_audio_tx_stream_started(uint8_t stream_idx)
{
	if (!initialized) {
		return -EACCES;
	}

	tx_info[stream_idx].iso_tx.seq_num = 0;
	return 0;
}

int bt_le_audio_tx_stream_sent(uint8_t stream_idx)
{
	if (!initialized) {
		return -EACCES;
	}

	atomic_dec(&tx_info[stream_idx].iso_tx_pool_alloc);
	return 0;
}

int bt_le_audio_tx_init(void)
{
	if (initialized) {
		return -EALREADY;
	}

	for (int i = 0; i < SRC_STREAM_COUNT; i++) {
		tx_info[i].iso_tx_pool = iso_tx_pools[i];
		tx_info[i].hci_wrn_printed = false;
		tx_info[i].iso_tx.ts = 0;
		tx_info[i].iso_tx.offset = 0;
		tx_info[i].iso_tx.seq_num = 0;
	}

	initialized = true;
	return 0;
}
