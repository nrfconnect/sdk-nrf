/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "bt_le_audio_tx.h"

#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/zbus/zbus.h>
#include <../subsys/bluetooth/audio/bap_stream.h>

#include "zbus_common.h"
#include "audio_sync_timer.h"
#include "sdc_hci_vs.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_le_audio_tx, CONFIG_BLE_LOG_LEVEL);

ZBUS_CHAN_DEFINE(sdu_ref_chan, struct sdu_ref_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
		 ZBUS_MSG_INIT(0));

#define HANDLE_INVALID 0xFFFF

#define HCI_ISO_BUF_ALLOC_PER_CHAN 2

/* For being able to dynamically define iso_tx_pools */
#define NET_BUF_POOL_ITERATE(i, _)                                                                 \
	NET_BUF_POOL_FIXED_DEFINE(iso_tx_pool_##i, HCI_ISO_BUF_ALLOC_PER_CHAN,                     \
				  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU), 8, NULL);
#define NET_BUF_POOL_PTR_ITERATE(i, ...) IDENTITY(&iso_tx_pool_##i)
LISTIFY(CONFIG_BT_ISO_MAX_CHAN, NET_BUF_POOL_ITERATE, (;))
/* clang-format off */
static struct net_buf_pool *iso_tx_pools[] = { LISTIFY(CONFIG_BT_ISO_MAX_CHAN,
						       NET_BUF_POOL_PTR_ITERATE, (,)) };
/* clang-format on */

struct tx_inf {
	uint16_t iso_conn_handle;
	struct bt_iso_tx_info iso_tx;
	struct bt_iso_tx_info iso_tx_readback;
	struct net_buf_pool *iso_tx_pool;
	atomic_t iso_tx_pool_alloc;
	bool hci_wrn_printed;
};

static bool initialized;
static struct tx_inf tx_info_arr[CONFIG_BT_ISO_MAX_CHAN];

/**
 * @brief Sends audio data over a single BAP stream.
 *
 * @param data			Audio data to send.
 * @param size			Size of data.
 * @param bap_stream		Pointer to BAP stream to use.
 * @param tx_info		Pointer to tx_info struct.
 * @param ts_tx		Timestamp to send. Note that for some controllers, BT_ISO_TIMESTAMP_NONE
 *			is used. This timestamp is used to ensure that SDUs are sent in the same
 *			connection interval.
 * @return		0 if successful, error otherwise.
 */
static int iso_stream_send(uint8_t const *const data, size_t size, struct bt_bap_stream *bap_stream,
			   struct tx_inf *tx_info, uint32_t ts_tx)
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

	if (ts_tx == 0) {
		ret = bt_bap_stream_send(bap_stream, buf, tx_info->iso_tx.seq_num);
	} else {
		ret = bt_bap_stream_send_ts(bap_stream, buf, tx_info->iso_tx.seq_num, ts_tx);
	}

	if (ret < 0) {
		if (ret != -ENOTCONN) {
			LOG_WRN("Failed to send audio data: %d stream %p", ret, bap_stream);
		}
		net_buf_unref(buf);
		atomic_dec(&tx_info->iso_tx_pool_alloc);
		return ret;
	} else {
		tx_info->iso_tx.seq_num++;
	}

	return 0;
}

static int get_tx_sync_sdc(uint16_t iso_conn_handle, struct bt_iso_tx_info *info)
{
	int ret;
	struct net_buf *buf;
	struct net_buf *rsp;
	sdc_hci_cmd_vs_iso_read_tx_timestamp_t *cmd_read_tx_timestamp;

	buf = bt_hci_cmd_create(SDC_HCI_OPCODE_CMD_VS_ISO_READ_TX_TIMESTAMP,
				sizeof(*cmd_read_tx_timestamp));
	if (!buf) {
		LOG_ERR("Could not allocate command buffer");
		return -ENOBUFS;
	}

	cmd_read_tx_timestamp = net_buf_add(buf, sizeof(*cmd_read_tx_timestamp));
	cmd_read_tx_timestamp->conn_handle = iso_conn_handle;

	ret = bt_hci_cmd_send_sync(SDC_HCI_OPCODE_CMD_VS_ISO_READ_TX_TIMESTAMP, buf, &rsp);
	if (ret) {
		return ret;
	}

	if (rsp) {
		sdc_hci_cmd_vs_iso_read_tx_timestamp_return_t *rsp_params = (void *)&rsp->data[1];

		info->ts = rsp_params->tx_time_stamp;
		info->seq_num = rsp_params->packet_sequence_number;
		info->offset = 0;

		net_buf_unref(rsp);
		return 0;
	}

	return -EINVAL;
}

static int iso_conn_handle_set(struct bt_bap_stream *bap_stream, uint16_t *iso_conn_handle)
{
	int ret;

	if (*iso_conn_handle == HANDLE_INVALID) {
		struct bt_bap_ep_info ep_info;

		ret = bt_bap_ep_get_info(bap_stream->ep, &ep_info);
		if (ret) {
			LOG_WRN("Unable to get info for ep");
			return -EACCES;
		}

		ret = bt_hci_get_conn_handle(ep_info.iso_chan->iso, iso_conn_handle);
		if (ret) {
			LOG_ERR("Failed obtaining conn_handle (ret:%d)", ret);
			return ret;
		}
	} else {
		/* Already set. */
	}

	return 0;
}

int bt_le_audio_tx_send(struct bt_bap_stream **bap_streams, uint8_t *audio_mapping_mask,
			struct le_audio_encoded_audio enc_audio, uint8_t streams_to_tx)
{
	int ret;
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

	if (streams_to_tx > CONFIG_BT_ISO_MAX_CHAN) {
		return -ENOMEM;
	}

	data_size_pr_stream = enc_audio.size / enc_audio.num_ch;

	/* When sending ISO data, we always send ts = 0 to the first active transmitting channel.
	 * The controller will populate with a ts which is fetched using bt_iso_chan_get_tx_sync.
	 * This timestamp will be submitted to all the other channels in order to place data on all
	 * channels in the same ISO interval.
	 */

	uint32_t common_tx_sync_ts_us = 0;
	uint32_t curr_ts_us = 0;
	bool ts_common_acquired = false;
	uint32_t common_interval = 0;

	for (int i = 0; i < streams_to_tx; i++) {
		if (tx_info_arr[i].iso_tx.seq_num == 0) {
			/* Temporary fix until /zephyr/pull/68745/ is available
			 */
#if defined(CONFIG_BT_BAP_DEBUG_STREAM_SEQ_NUM)
			bap_streams[i]->_prev_seq_num = 0;
#endif /* CONFIG_BT_BAP_DEBUG_STREAM_SEQ_NUM */
		}

		if (!le_audio_ep_state_check(bap_streams[i]->ep, BT_BAP_EP_STATE_STREAMING)) {
			/* This bap_stream is not streaming*/
			continue;
		}

		if (audio_mapping_mask[i] > enc_audio.num_ch) {
			LOG_WRN("Unsupported audio_channel: %d", audio_mapping_mask[i]);
			continue;
		}

		uint32_t bitrate;

		ret = le_audio_bitrate_get(bap_streams[i]->codec_cfg, &bitrate);
		if (ret) {
			LOG_ERR("Failed to calculate bitrate: %d", ret);
			return ret;
		}

		if (data_size_pr_stream != LE_AUDIO_SDU_SIZE_OCTETS(bitrate)) {
			LOG_ERR("The encoded data size does not match the SDU size");
			return -EINVAL;
		}

		if (common_interval != 0 && (common_interval != bap_streams[i]->qos->interval)) {
			LOG_ERR("Not all channels have the same ISO interval");
			return -EINVAL;
		}
		common_interval = bap_streams[i]->qos->interval;

		/* Check if same audio is sent to all channels */
		if (enc_audio.num_ch == 1) {
			ret = iso_stream_send(enc_audio.data, data_size_pr_stream, bap_streams[i],
					      &tx_info_arr[i], common_tx_sync_ts_us);
		} else {
			ret = iso_stream_send(
				&enc_audio.data[data_size_pr_stream * audio_mapping_mask[i]],
				data_size_pr_stream, bap_streams[i], &tx_info_arr[i],
				common_tx_sync_ts_us);
		}

		if (ret) {
			/* DBG used here as prints are handled within iso_stream_send */
			LOG_DBG("Failed to send to idx: %d stream: %p, ret: %d ", i, bap_streams[i],
				ret);
			continue;
		}

		ret = iso_conn_handle_set(bap_streams[i], &tx_info_arr[i].iso_conn_handle);
		if (ret) {
			continue;
		}

		/* Strictly, it is only required to call get_tx_sync_sdc on the first streaming
		 * channel to get the timestamp which is sent to all other channels.
		 * However, to be able to detect errors, this is called on each TX.
		 */
		ret = get_tx_sync_sdc(tx_info_arr[i].iso_conn_handle,
				      &tx_info_arr[i].iso_tx_readback);
		if (ret) {
			if (ret != -ENOTCONN) {
				LOG_WRN("Unable to get tx sync. ret: %d stream: %p", ret,
					bap_streams[i]);
			}
			continue;
		}

		if (!ts_common_acquired) {
			curr_ts_us = audio_sync_timer_capture();
			common_tx_sync_ts_us = tx_info_arr[i].iso_tx_readback.ts;
			ts_common_acquired = true;
		}
	}

	if (ts_common_acquired) {
		struct sdu_ref_msg msg;

		msg.tx_sync_ts_us = common_tx_sync_ts_us;
		msg.curr_ts_us = curr_ts_us;
		msg.adjust = true;

		ret = zbus_chan_pub(&sdu_ref_chan, &msg, K_NO_WAIT);
		if (ret) {
			LOG_WRN("Failed to publish timestamp: %d", ret);
		}
	}

	return 0;
}

int bt_le_audio_tx_stream_stopped(uint8_t stream_idx)
{
	if (!initialized) {
		return -EACCES;
	}

	atomic_clear(&tx_info_arr[stream_idx].iso_tx_pool_alloc);

	return 0;
}

int bt_le_audio_tx_stream_started(uint8_t stream_idx)
{
	if (!initialized) {
		return -EACCES;
	}

	tx_info_arr[stream_idx].hci_wrn_printed = false;
	tx_info_arr[stream_idx].iso_conn_handle = HANDLE_INVALID;
	tx_info_arr[stream_idx].iso_tx.seq_num = 0;
	tx_info_arr[stream_idx].iso_tx_readback.seq_num = 0;
	return 0;
}

int bt_le_audio_tx_stream_sent(uint8_t stream_idx)
{
	if (!initialized) {
		return -EACCES;
	}

	atomic_dec(&tx_info_arr[stream_idx].iso_tx_pool_alloc);
	return 0;
}

int bt_le_audio_tx_init(void)
{
	if (initialized) {
		return -EALREADY;
	}

	for (int i = 0; i < CONFIG_BT_ISO_MAX_CHAN; i++) {
		tx_info_arr[i].iso_tx_pool = iso_tx_pools[i];
		tx_info_arr[i].hci_wrn_printed = false;
		tx_info_arr[i].iso_conn_handle = HANDLE_INVALID;
		tx_info_arr[i].iso_tx.ts = 0;
		tx_info_arr[i].iso_tx.offset = 0;
		tx_info_arr[i].iso_tx.seq_num = 0;
		tx_info_arr[i].iso_tx_readback.ts = 0;
		tx_info_arr[i].iso_tx_readback.offset = 0;
		tx_info_arr[i].iso_tx_readback.seq_num = 0;
	}

	initialized = true;
	return 0;
}
