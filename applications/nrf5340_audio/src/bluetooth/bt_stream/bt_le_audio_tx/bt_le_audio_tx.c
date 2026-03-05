/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "bt_le_audio_tx.h"

#include <string.h>

#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/zbus/zbus.h>
#include <../subsys/bluetooth/audio/bap_stream.h>
#include <bluetooth/hci_vs_sdc.h>
#include <audio_defines.h>

#include "zbus_common.h"
#include "audio_sync_timer.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_le_audio_tx, CONFIG_BT_LE_AUDIO_TX_LOG_LEVEL);

ZBUS_CHAN_DEFINE(sdu_ref_chan, struct sdu_ref_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
		 ZBUS_MSG_INIT(0));

#define HANDLE_INVALID 0xFFFF

/* Time between performing a timestamp re-sync with the Bluetooth controller.
 * For gateway/central (unicast client/broadcast source) where this device is the master
 * Bluetooth LE clocking device, the wall clock and BLE clock should be in sync.
 * Hence, there is no strict need for regular re-sync after the first timestamp is obtained.
 * For simplicity and risk mitigation, a periodic re-sync is still performed.
 * However, for the headset/peripheral (unicast server/broadcast sink)
 * regular re-sync is needed to compensate for clock drift.
 */
#define TX_TS_RESYNC_US 1000000

/* When starting a stream, I2S or USB feeding the TX function with data, will usually need some time
 * to stabilize and potentially drop data to meet just-in-time requirements.
 */
#define SUBSEQUENT_RAPID_CORRECTIONS_LIMIT 20

struct tx_inf {
	uint16_t iso_conn_handle;
	struct bt_iso_tx_info iso_tx;
	struct bt_iso_tx_info iso_tx_readback;
	atomic_t iso_tx_pool_alloc;
	bool hci_wrn_printed;
};

struct bt_le_audio_tx_ctx {
	struct bt_le_audio_tx_cfg cfg;
	struct tx_inf tx_info_arr[BT_LE_AUDIO_TX_GROUP_MAX][BT_LE_AUDIO_TX_SUBGROUP_MAX]
				 [BT_LE_AUDIO_TX_STREAMS_MAX];
	uint32_t timestamp_ctlr_esti_us;
	bool flush_next;
	bool timestamp_ctlr_esti_us_valid;
	uint32_t timestamp_ctlr_real_us_last;
	uint32_t timestamp_last_correction;
	uint32_t subequent_rapid_corrections;
	int32_t corr_diff_us;
};

/**
 * @brief Sends audio data over a single BAP stream.
 *
 * @param data		Audio data to send.
 * @param size		Size of data.
 * @param bap_stream	Pointer to BAP stream to use.
 * @param tx_info	Pointer to tx_info struct.
 * @param ts_tx		Timestamp to send. Note that for some controllers, BT_ISO_TIMESTAMP_NONE
 *			is used. This timestamp is used to ensure that SDUs are sent in the same
 *			connection interval.
 * @return		0 if successful, error otherwise.
 */
static int iso_stream_send(uint8_t const *const data, size_t size, struct bt_cap_stream *cap_stream,
			   struct bt_le_audio_tx_ctx *ctx, struct tx_inf *tx_info, uint32_t ts_tx,
			   bool send_ts_tx)
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
	if (atomic_get(&tx_info->iso_tx_pool_alloc) >= BT_LE_AUDIO_TX_HCI_ISO_BUF_PER_CHAN) {
		if (!tx_info->hci_wrn_printed) {
			struct bt_iso_chan *iso_chan;

			iso_chan = bt_bap_stream_iso_chan_get(&cap_stream->bap_stream);

			LOG_WRN("HCI ISO TX overrun on stream %p - Single print",
				(void *)&cap_stream->bap_stream);
			tx_info->hci_wrn_printed = true;
		}
		return -ENOMEM;
	}

	tx_info->hci_wrn_printed = false;

	buf = net_buf_alloc(ctx->cfg.iso_tx_pool, K_NO_WAIT);
	if (buf == NULL) {
		/* This should never occur because of the iso_tx_pool_alloc check above */
		LOG_WRN("Out of TX buffers");
		return -ENOMEM;
	}

	net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);
	net_buf_add_mem(buf, data, size);

	atomic_inc(&tx_info->iso_tx_pool_alloc);

	if (send_ts_tx) {
		ret = bt_cap_stream_send_ts(cap_stream, buf, tx_info->iso_tx.seq_num, ts_tx);
	} else {
		ret = bt_cap_stream_send(cap_stream, buf, tx_info->iso_tx.seq_num);
	}

	if (ret < 0) {
		if (ret != -ENOTCONN) {
			LOG_WRN("Failed to send audio data: %d stream %p", ret,
				(void *)&cap_stream->bap_stream);
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
	sdc_hci_cmd_vs_iso_read_tx_timestamp_t cmd_read_tx_timestamp;
	sdc_hci_cmd_vs_iso_read_tx_timestamp_return_t rsp_params;

	cmd_read_tx_timestamp.conn_handle = iso_conn_handle;

	ret = hci_vs_sdc_iso_read_tx_timestamp(&cmd_read_tx_timestamp, &rsp_params);
	if (ret) {
		return ret;
	}

	info->ts = rsp_params.tx_time_stamp;
	info->seq_num = rsp_params.packet_sequence_number;
	info->offset = 0;

	return 0;
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

static int tx_precheck(struct le_audio_tx_info const *const tx, uint8_t num_tx, uint8_t num_loc,
		       size_t data_size_pr_stream, uint32_t *common_interval_us)
{
	int ret;

	for (int i = 0; i < num_tx; i++) {
		if (!le_audio_ep_state_check(tx[i].cap_stream->bap_stream.ep,
					     BT_BAP_EP_STATE_STREAMING)) {
			/* This bap_stream is not streaming*/
			continue;
		}

		if (tx[i].audio_location > num_loc) {
			LOG_WRN("Unsupported audio_location: %d", tx[i].audio_location);
			continue;
		}

		uint32_t bitrate;
		int dur_us;

		ret = le_audio_bitrate_get(tx[i].cap_stream->bap_stream.codec_cfg, &bitrate);
		if (ret) {
			LOG_ERR("Failed to calculate bitrate: %d", ret);
			return ret;
		}

		ret = le_audio_duration_us_get(tx[i].cap_stream->bap_stream.codec_cfg, &dur_us);
		if (ret) {
			LOG_ERR("Error retrieving frame duration: %d", ret);
			return ret;
		}

		if (data_size_pr_stream != LE_AUDIO_SDU_SIZE_OCTETS(bitrate, dur_us)) {
			LOG_ERR("The encoded data size (%zu) does not match the SDU size (%d)",
				data_size_pr_stream, LE_AUDIO_SDU_SIZE_OCTETS(bitrate, dur_us));
			return -EINVAL;
		}

		if (*common_interval_us != 0 &&
		    (*common_interval_us != tx[i].cap_stream->bap_stream.qos->interval)) {
			LOG_ERR("Not all channels have the same ISO interval");
			return -EINVAL;
		}
		*common_interval_us = tx[i].cap_stream->bap_stream.qos->interval;
	}

	return 0;
}

int controller_timestamp_tx_get(struct le_audio_tx_info const *const tx, uint8_t num_tx,
				uint32_t *timestamp_tx_us, struct tx_inf *tx_info)
{
	int ret;

	ret = iso_conn_handle_set(&tx[num_tx - 1].cap_stream->bap_stream,
				  &tx_info->iso_conn_handle);
	if (ret) {
		LOG_ERR("Failed to set iso_conn_handle: %d", ret);
		*timestamp_tx_us = 0;
		return ret;
	}

	ret = get_tx_sync_sdc(tx_info->iso_conn_handle, &tx_info->iso_tx_readback);
	if (ret) {
		LOG_ERR("Unable to get tx sync. ret: %d", ret);
		*timestamp_tx_us = 0;
		return ret;
	}

	*timestamp_tx_us = tx_info->iso_tx_readback.ts;
	return 0;
}

int bt_le_audio_tx_send(struct bt_le_audio_tx_ctx *ctx, struct net_buf const *const audio_frame,
			struct le_audio_tx_info *tx, uint8_t num_tx)
{
	int ret;
	size_t data_size_pr_stream = 0;

	if (ctx == NULL) {
		return -EACCES;
	}

	if (tx == NULL || audio_frame == NULL) {
		return -EINVAL;
	}

	/* Get number of locations in the audio frame */
	struct audio_metadata *meta = net_buf_user_data(audio_frame);

	if (meta == NULL) {
		LOG_ERR("Audio metadata is missing");
		return -EINVAL;
	}

	uint8_t num_loc = audio_metadata_num_loc_get(meta);

	if (num_loc == 0 || (audio_frame->len % num_loc != 0)) {
		LOG_ERR("Invalid number (%d) of locations in audio frame (%d)", num_loc,
			audio_frame->len);
		return -EINVAL;
	}

	uint32_t common_interval_us = 0;

	data_size_pr_stream = audio_frame->len / num_loc;
	if (data_size_pr_stream == 0) {
		LOG_ERR("Data size per stream is 0");
		return -EINVAL;
	}

	ret = tx_precheck(tx, num_tx, num_loc, data_size_pr_stream, &common_interval_us);
	if (ret) {
		return ret;
	}

	uint8_t sent_streams = 0;
	struct tx_inf *tx_info = NULL;

	if (common_interval_us == 0) {
		LOG_ERR("common_interval is zero");
		return -EINVAL;
	}

	/* Increment the tx timestamp with the interval.
	 * If this TX function is called too fast or slow, the estimate will deteriorate faster.
	 */
	ctx->timestamp_ctlr_esti_us += common_interval_us;
	if (ctx->flush_next) {
		LOG_DBG("Flushed %d us of audio data to maintain timestamp sync with controller",
			common_interval_us);
		ctx->flush_next = false;
		ctx->timestamp_ctlr_esti_us_valid = false;
		return -EAGAIN;
	}

	LOG_DBG_RATELIMIT_RATE(1000, "tx %d valid %d", num_tx, ctx->timestamp_ctlr_esti_us_valid);

	for (int i = 0; i < num_tx; i++) {
		tx_info = &ctx->tx_info_arr[tx[i].idx.lvl1][tx[i].idx.lvl2][tx[i].idx.lvl3];

		/* Check if same audio is sent to all locations */
		if (num_loc == 1) {
			ret = iso_stream_send(audio_frame->data, data_size_pr_stream,
					      tx[i].cap_stream, ctx, tx_info,
					      ctx->timestamp_ctlr_esti_us,
					      ctx->timestamp_ctlr_esti_us_valid);
		} else {
			ret = iso_stream_send(
				audio_frame->data + (tx[i].audio_location * data_size_pr_stream),
				data_size_pr_stream, tx[i].cap_stream, ctx, tx_info,
				ctx->timestamp_ctlr_esti_us, ctx->timestamp_ctlr_esti_us_valid);
		}

		if (ret) {
			LOG_ERR("Failed to send to idx: %d stream: %p, ret: %d ", i,
				(void *)&tx[i].cap_stream->bap_stream, ret);
			continue;
		} else {
			sent_streams++;
		}
	}

	if (sent_streams == 0) {
		/* If nothing was sent, do not continue with timestamp handling */
		LOG_ERR("No streams were sent");
		return -EIO;
	}

	/* Get the current (wall clock) time */
	uint32_t timestamp_now_us = audio_sync_timer_capture();
	uint32_t timestamp_ctlr_real_us = 0;
	ctx->corr_diff_us = 0;

	/* If the timestamp is not valid or it has been more than TX_TS_RESYNC_US since the last
	 * timestamp, we re-aquire a timestamp from the controller.
	 */
	if (!ctx->timestamp_ctlr_esti_us_valid ||
	    (ctx->timestamp_ctlr_esti_us - ctx->timestamp_ctlr_real_us_last) >= TX_TS_RESYNC_US) {

		ret = controller_timestamp_tx_get(tx, num_tx, &timestamp_ctlr_real_us, tx_info);
		if (ret) {
			LOG_ERR("Failed to get timestamp after sending: %d", ret);
			return ret;
		}

		uint32_t time_since_last_corr_us =
			timestamp_now_us - ctx->timestamp_last_correction;
		ctx->corr_diff_us = (int32_t)(timestamp_ctlr_real_us - ctx->timestamp_ctlr_esti_us);

		if (time_since_last_corr_us < TX_TS_RESYNC_US) {
			ctx->subequent_rapid_corrections++;
		} else {
			ctx->subequent_rapid_corrections = 0;
		}

		if (ctx->subequent_rapid_corrections > SUBSEQUENT_RAPID_CORRECTIONS_LIMIT) {
			LOG_ERR_RATELIMIT_RATE(1000, "%d subsequent time corrections",
					       ctx->subequent_rapid_corrections);
		}

		/* There are several options which can occur at this point:
		 *
		 * - corr_diff_us >> 0: This means that data was given to this module too slow/late.
		 * Hence, the controller schedules the sending for the next ISO interval.
		 * An empty packet will be sent on air, and we will correct the timestamp estimate.
		 * - corr_diff_us << 0: This should not happen as an earlier timestamp has already
		 * been given to the controller.
		 * - corr_diff_us > or < 0: This means that the estimate has gotten a small
		 * correction. As a central/gateway/unicast client/broadcast source, the clocks
		 * shall be running in lockstep and this should never happen. As a
		 * peripheral/headset/unicast server/broadcast sink, this will happen to correct for
		 * drift.
		 */

		if (ctx->corr_diff_us == 0) {
			/* No correction needed, the estimate is correct. */
			LOG_DBG("TX clock no adjustment needed (%d us)", ctx->corr_diff_us);
			ctx->timestamp_ctlr_esti_us_valid = true;
		} else if (ctx->corr_diff_us > (common_interval_us / 2)) {
			LOG_WRN("TX clock has been updated by (%d us). Empty packet on air",
				ctx->corr_diff_us);
			ctx->timestamp_ctlr_esti_us_valid = false;
		} else if (ctx->corr_diff_us < (common_interval_us / 2)) {
			LOG_ERR("TX clock has been updated by (%d us). Shall not happen",
				ctx->corr_diff_us);
			ctx->timestamp_ctlr_esti_us_valid = false;
			return -EIO;
		} else {
			if (CONFIG_AUDIO_DEV == HEADSET) {
				LOG_INF("TX clock has been drift adjusted by (%d us)",
					ctx->corr_diff_us);
				ctx->timestamp_ctlr_esti_us_valid = true;
			}
			if (CONFIG_AUDIO_DEV == GATEWAY) {
				LOG_ERR("TX clock has been drift adjusted by (%d us)",
					ctx->corr_diff_us);
				ctx->timestamp_ctlr_esti_us_valid = false;
			}
		}

		ctx->timestamp_ctlr_real_us_last = timestamp_ctlr_real_us;
		/* Update the estimate to the real (most current) value */
		ctx->timestamp_ctlr_esti_us = timestamp_ctlr_real_us;

		ctx->timestamp_last_correction = timestamp_now_us;
	}

	if (timestamp_now_us > (ctx->timestamp_ctlr_esti_us + common_interval_us)) {
		LOG_WRN("Curr time ahead of ctlr time. Flush %d us of data. "
			"Now: %u us, ctlr: %u us",
			common_interval_us, timestamp_now_us, ctx->timestamp_ctlr_esti_us);
		ctx->flush_next = true;
		ctx->timestamp_ctlr_esti_us_valid = false;
	}

	/* At this point, the controller estimated timestamp is valid */
	struct sdu_ref_msg msg;

	msg.tx_sync_ts_us = ctx->timestamp_ctlr_esti_us;
	msg.curr_ts_us = timestamp_now_us;
	msg.adjust = true;

	ret = zbus_chan_pub(&sdu_ref_chan, &msg, K_NO_WAIT);
	if (ret) {
		LOG_WRN("Failed to publish timestamp: %d", ret);
	}

	return 0;
}

int bt_le_audio_tx_stream_started(struct bt_le_audio_tx_ctx *ctx, struct stream_index idx)
{
	if (ctx == NULL) {
		return -EACCES;
	}

	atomic_clear(&ctx->tx_info_arr[idx.lvl1][idx.lvl2][idx.lvl3].iso_tx_pool_alloc);

	ctx->tx_info_arr[idx.lvl1][idx.lvl2][idx.lvl3].hci_wrn_printed = false;
	ctx->tx_info_arr[idx.lvl1][idx.lvl2][idx.lvl3].iso_conn_handle = HANDLE_INVALID;
	ctx->tx_info_arr[idx.lvl1][idx.lvl2][idx.lvl3].iso_tx.seq_num = 0;
	ctx->tx_info_arr[idx.lvl1][idx.lvl2][idx.lvl3].iso_tx_readback.seq_num = 0;
	return 0;
}

int bt_le_audio_tx_stream_sent(struct bt_le_audio_tx_ctx *ctx, struct stream_index stream_idx)
{
	if (ctx == NULL) {
		return -EACCES;
	}

	atomic_dec(&ctx->tx_info_arr[stream_idx.lvl1][stream_idx.lvl2][stream_idx.lvl3]
			    .iso_tx_pool_alloc);
	return 0;
}

int bt_le_audio_tx_init(struct bt_le_audio_tx_ctx *ctx)
{
	if (ctx == NULL || ctx->cfg.iso_tx_pool == NULL) {
		return -EINVAL;
	}

	struct bt_le_audio_tx_cfg cfg = ctx->cfg;

	(void)memset(ctx, 0, sizeof(*ctx));

	ctx->cfg = cfg;

	for (int i = 0; i < BT_LE_AUDIO_TX_GROUP_MAX; i++) {
		for (int j = 0; j < BT_LE_AUDIO_TX_SUBGROUP_MAX; j++) {
			for (int k = 0; k < BT_LE_AUDIO_TX_STREAMS_MAX; k++) {
				ctx->tx_info_arr[i][j][k].iso_conn_handle = HANDLE_INVALID;
			}
		}
	}

	return 0;
}
