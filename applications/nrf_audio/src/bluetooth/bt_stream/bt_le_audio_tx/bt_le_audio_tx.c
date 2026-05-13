/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "bt_le_audio_tx.h"

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

/* Maximum allowed time deviation for calling tx_send.
 * E.g. 0.2 = 20 % deviation from the common interval
 * So, if the common interval is 10 ms, the function can be called with an interval between 8 ms and
 * 12 ms without considering it a gap and resyncing with the controller.
 */
#define INVOKE_TIME_LIMIT_DEVIATION_WINDOW 0.25f
#define INVOKE_TIME_LIM_MAX		   (1.0f + INVOKE_TIME_LIMIT_DEVIATION_WINDOW)
#define INVOKE_TIME_LIM_MIN		   (1.0f - INVOKE_TIME_LIMIT_DEVIATION_WINDOW)

/* Time between performing a timestamp re-sync with the Bluetooth controller.
 * For gateway/central (unicast client/broadcast source) where this device is the master
 * Bluetooth LE clocking device, the wall clock and BLE clock should be in sync.
 * Hence, there is no strict need for regular re-sync after the first timestamp is obtained.
 * For simplicity and risk mitigation, a periodic re-sync is still performed.
 * However, for the headset/peripheral (unicast server/broadcast sink)
 * regular re-sync is needed to compensate for clock drift.
 */
#define TX_TS_RESYNC_US 1000000U

/* When starting a stream, I2S or USB feeding the TX function with data, will usually need some time
 * to stabilize and potentially drop data to meet just-in-time requirements.
 */
#define SUBSEQUENT_RAPID_CORRECTIONS_LIMIT 20U

/**
 * @brief Sends audio data over a single BAP stream.
 *
 * @param data		Audio data to send.
 * @param size		Size of data.
 * @param cap_stream	Pointer to CAP stream to use.
 * @param ctx		Pointer to TX context.
 * @param tx_info	Pointer to tx_info struct.
 * @param ts_tx		Timestamp to send. Note that for some controllers, BT_ISO_TIMESTAMP_NONE
 *			is used. This timestamp is used to ensure that SDUs are sent in the same
 *			connection interval.
 * @param send_ts_tx	Whether to send the ts_tx timestamp or not.
 *
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
	if (atomic_get(&tx_info->iso_tx_pool_alloc) >= HCI_ISO_BUF_PER_CHAN) {
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

	buf = net_buf_alloc(ctx->iso_tx_pool, K_NO_WAIT);
	if (buf == NULL) {
		/* This should never occur because of the iso_tx_pool_alloc check above */
		LOG_WRN("Out of TX buffers");
		return -ENOMEM;
	}

	net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);
	net_buf_add_mem(buf, data, size);

	atomic_inc(&tx_info->iso_tx_pool_alloc);

	if (send_ts_tx) {
		ret = bt_cap_stream_send_ts(cap_stream, buf, 0U, ts_tx);
	} else {
		ret = bt_cap_stream_send(cap_stream, buf, 0U);
	}

	if (ret < 0) {
		if (ret != -ENOTCONN) {
			LOG_WRN("Failed to send audio data: %d stream %p", ret,
				(void *)&cap_stream->bap_stream);
		}
		net_buf_unref(buf);
		(void)atomic_dec(&tx_info->iso_tx_pool_alloc);
		return ret;
	}

	return 0;
}

static int get_tx_sync_sdc(uint16_t iso_conn_handle, struct bt_iso_tx_info *info)
{
	int ret;
	sdc_hci_cmd_vs_iso_read_tx_timestamp_t cmd_read_tx_ts;
	sdc_hci_cmd_vs_iso_read_tx_timestamp_return_t rsp_params;

	cmd_read_tx_ts.conn_handle = iso_conn_handle;

	ret = hci_vs_sdc_iso_read_tx_timestamp(&cmd_read_tx_ts, &rsp_params);
	if (ret != 0) {
		return ret;
	}

	info->ts = rsp_params.tx_time_stamp;
	info->seq_num = rsp_params.packet_sequence_number;
	info->offset = 0U;

	return 0;
}

static int iso_conn_handle_set(const struct bt_bap_stream *bap_stream, uint16_t *iso_conn_handle)
{
	int ret;

	if (*iso_conn_handle == HANDLE_INVALID) {
		struct bt_bap_ep_info ep_info;

		ret = bt_bap_ep_get_info(bap_stream->ep, &ep_info);
		if (ret != 0) {
			LOG_WRN("Unable to get info for ep");
			return -EACCES;
		}

		ret = bt_hci_get_conn_handle(ep_info.iso_chan->iso, iso_conn_handle);
		if (ret != 0) {
			LOG_ERR("Failed obtaining conn_handle (ret:%d)", ret);
			return ret;
		}
	} else {
		/* Already set. */
	}

	return 0;
}

static int verify_stream_idx(struct stream_index idx)
{
	if (idx.lvl1 >= GROUP_MAX || idx.lvl2 >= SUBGROUP_MAX || idx.lvl3 >= TX_STREAMS_MAX) {
		return -ESPIPE;
	}

	return 0;
}

static int tx_entries_validate(struct le_audio_tx_info const *const tx, uint8_t num_tx)
{
	for (int i = 0; i < num_tx; i++) {
		int ret = verify_stream_idx(tx[i].idx);

		if (ret != 0) {
			LOG_ERR("Invalid stream index for tx[%d]", i);
			return -EINVAL;
		}

		if (tx[i].cap_stream == NULL || tx[i].cap_stream->bap_stream.ep == NULL ||
		    tx[i].cap_stream->bap_stream.qos == NULL ||
		    tx[i].cap_stream->bap_stream.codec_cfg == NULL) {
			LOG_ERR("Invalid tx stream configuration for tx[%d]", i);
			return -EINVAL;
		}
	}

	return 0;
}

/**
 * @brief Validate all TX streams before sending audio data.
 *
 * Iterates over all streams in @p tx and, for each stream in the STREAMING state:
 * - Increments @p num_streaming_state.
 * - Verifies that @p data_size_pr_stream matches the SDU size derived from the
 *   stream's codec configuration (bitrate and frame duration).
 * - Verifies that the streams share the same ISO interval, and
 *   writes the common interval to @p common_interval_us.
 *
 * Streams not in the STREAMING state are silently skipped.
 */
static int tx_precheck(struct le_audio_tx_info const *const tx, uint8_t num_tx,
		       size_t data_size_pr_stream, uint32_t *common_interval_us,
		       uint8_t *num_streaming_state)
{
	int ret;

	for (int i = 0; i < num_tx; i++) {
		const struct bt_bap_stream *bap_stream = &tx[i].cap_stream->bap_stream;

		if (unlikely(!le_audio_ep_state_check(bap_stream->ep, BT_BAP_EP_STATE_STREAMING))) {
			/* This bap_stream is not streaming */
			continue;
		}

		(*num_streaming_state)++;

		uint32_t bitrate;
		int dur_us;

		ret = le_audio_bitrate_get(bap_stream->codec_cfg, &bitrate);
		if (ret != 0) {
			LOG_ERR("Failed to calculate bitrate: %d", ret);
			return ret;
		}

		ret = le_audio_duration_us_get(bap_stream->codec_cfg, &dur_us);
		if (ret != 0) {
			LOG_ERR("Error retrieving frame duration: %d", ret);
			return ret;
		}

		if (unlikely(data_size_pr_stream != LE_AUDIO_SDU_SIZE_OCTETS(bitrate, dur_us))) {
			LOG_ERR("The encoded data size (%zu) does not match the SDU size (%d)",
				data_size_pr_stream, LE_AUDIO_SDU_SIZE_OCTETS(bitrate, dur_us));
			return -EINVAL;
		}

		if (unlikely(*common_interval_us != 0U &&
			     (*common_interval_us != bap_stream->qos->interval))) {
			LOG_ERR("Not all channels have the same ISO interval");
			return -EINVAL;
		}
		*common_interval_us = bap_stream->qos->interval;
	}

	return 0;
}

static int controller_ts_tx_get(const struct bt_bap_stream *bap_stream, uint32_t *ts_tx_us,
				struct tx_inf *tx_info)
{
	int ret;

	ret = iso_conn_handle_set(bap_stream, &tx_info->iso_conn_handle);
	if (ret != 0) {
		LOG_ERR("Failed to set iso_conn_handle: %d", ret);
		*ts_tx_us = 0U;
		return ret;
	}

	ret = get_tx_sync_sdc(tx_info->iso_conn_handle, &tx_info->iso_tx_readback);
	if (ret != 0) {
		LOG_ERR("Unable to get tx sync. ret: %d", ret);
		*ts_tx_us = 0U;
		return ret;
	}

	*ts_tx_us = tx_info->iso_tx_readback.ts;
	return 0;
}

static int num_loc_get(struct net_buf const *const audio_frame, uint8_t *const num_loc)
{
	struct audio_metadata *meta = net_buf_user_data(audio_frame);

	if (meta == NULL) {
		LOG_ERR("Audio metadata is missing");
		return -EINVAL;
	}

	*num_loc = audio_metadata_num_loc_get(meta);

	if (*num_loc == 0U || (audio_frame->len % *num_loc != 0U)) {
		LOG_ERR("Invalid number (%u) of locations in audio frame (%u)", *num_loc,
			audio_frame->len);
		return -EINVAL;
	}

	return 0;
}

static void tx_call_interval_check(struct bt_le_audio_tx_ctx *ctx, uint32_t ts_now_us,
				   uint32_t common_interval_us)
{
	uint32_t time_since_last_call_us = ts_now_us - ctx->ts_last_us;
	uint32_t upper_lim_us = (uint32_t)(common_interval_us * INVOKE_TIME_LIM_MAX);
	uint32_t lower_lim_us = (uint32_t)(common_interval_us * INVOKE_TIME_LIM_MIN);
	const uint32_t PRINT_WRN_LIMIT_US = 200000U;

	if (!IN_RANGE(time_since_last_call_us, lower_lim_us, upper_lim_us) &&
	    ctx->ts_last_us_valid) {
		/* This will happen if a stream is paused/restarted */
		ctx->ts_ctlr_esti_us_valid = false;

		if (time_since_last_call_us > PRINT_WRN_LIMIT_US) {
			/* Likely a result of a stream being paused/restarted */
			LOG_INF("Audio tx called with interval of %u us, (min %u us, max %u us) "
				"expected "
				"around %u us. Resync with controller",
				time_since_last_call_us, lower_lim_us, upper_lim_us,
				common_interval_us);
		} else {
			LOG_WRN("Audio tx called with interval of %u us, (min %u us, max %u us) "
				"expected "
				"around %u us. Resync with controller",
				time_since_last_call_us, lower_lim_us, upper_lim_us,
				common_interval_us);
		}
	}
}

static uint8_t tx_streams_send(struct bt_le_audio_tx_ctx *ctx,
			       struct net_buf const *const audio_frame, struct le_audio_tx_info *tx,
			       uint8_t num_tx, uint8_t num_loc, size_t data_size_pr_stream,
			       const struct bt_bap_stream **last_successful_bap_stream,
			       struct tx_inf **last_successful_tx_info)
{
	int ret;
	uint8_t sent_streams = 0U;

	for (int i = 0; i < num_tx; i++) {
		const struct bt_bap_stream *bap_stream = &tx[i].cap_stream->bap_stream;
		struct tx_inf *tx_info =
			&ctx->tx_info_arr[tx[i].idx.lvl1][tx[i].idx.lvl2][tx[i].idx.lvl3];

		/* Check if same audio is sent to all locations */
		if (num_loc == 1U) {
			ret = iso_stream_send(audio_frame->data, data_size_pr_stream,
					      tx[i].cap_stream, ctx, tx_info, ctx->ts_ctlr_esti_us,
					      ctx->ts_ctlr_esti_us_valid);
		} else {
			if (unlikely(tx[i].audio_location >= num_loc)) {
				LOG_ERR("audio_location (%u) out of bounds (num_loc: %u) for "
					"stream "
					"%d",
					tx[i].audio_location, num_loc, i);
				continue;
			}

			ret = iso_stream_send(audio_frame->data +
						      (tx[i].audio_location * data_size_pr_stream),
					      data_size_pr_stream, tx[i].cap_stream, ctx, tx_info,
					      ctx->ts_ctlr_esti_us, ctx->ts_ctlr_esti_us_valid);
		}

		if (ret != 0) {
			/* May happen if we try to send to a device which has disconnected */
			LOG_INF_RATELIMIT_RATE(1000,
					       "Failed to send to idx: %d stream: %p, ret: %d ", i,
					       (void *)&tx[i].cap_stream->bap_stream, ret);
		} else {
			*last_successful_bap_stream = bap_stream;
			*last_successful_tx_info = tx_info;
			sent_streams++;
		}
	}

	return sent_streams;
}

static int tx_ts_prepare(struct bt_le_audio_tx_ctx *ctx, uint32_t common_interval_us)
{
	/* Increment the tx timestamp with the interval.
	 * Designed for overflow.
	 * If this TX function is called too fast or slow, the estimate will deteriorate more
	 * quickly.
	 */
	ctx->ts_ctlr_esti_us += common_interval_us;
	if (ctx->flush_next) {
		LOG_DBG("Flushed %u us of audio data to maintain timestamp sync with controller",
			common_interval_us);
		ctx->flush_next = false;
		ctx->ts_ctlr_esti_us_valid = false;
		return -ECANCELED;
	}

	return 0;
}

static int tx_controller_sync_and_correct(struct bt_le_audio_tx_ctx *ctx,
					  const struct bt_bap_stream *last_successful_bap_stream,
					  uint32_t common_interval_us, uint32_t ts_now_us,
					  struct tx_inf *tx_info, int *status)
{
	int ret;
	uint32_t ts_ctlr_real_us = 0U;

	ctx->corr_diff_us = 0U;

	/* If the timestamp is not valid or it has been more than TX_TS_RESYNC_US since the last
	 * timestamp, we re-acquire a timestamp from the controller.
	 */
	if (!ctx->ts_ctlr_esti_us_valid ||
	    ((uint32_t)(ctx->ts_ctlr_esti_us - ctx->ts_ctlr_real_us_last) >= TX_TS_RESYNC_US) ||
	    ((uint32_t)(ts_now_us - ctx->ts_last_us) >= TX_TS_RESYNC_US)) {

		ret = controller_ts_tx_get(last_successful_bap_stream, &ts_ctlr_real_us, tx_info);
		if (ret != 0) {
			LOG_ERR("Failed to get timestamp after sending: %d", ret);
			return ret;
		}

		ctx->corr_diff_us = (int64_t)ts_ctlr_real_us - (int64_t)ctx->ts_ctlr_esti_us;
		LOG_DBG_RATELIMIT_RATE(1000U, "TX clock correction: %lld us real %u esti %u",
				       ctx->corr_diff_us, ts_ctlr_real_us, ctx->ts_ctlr_esti_us);

		if ((uint32_t)(ts_now_us - ctx->ts_last_correction) < TX_TS_RESYNC_US) {
			ctx->subsequent_rapid_corrections++;
		} else {
			ctx->subsequent_rapid_corrections = 0U;
		}

		if (ctx->subsequent_rapid_corrections > SUBSEQUENT_RAPID_CORRECTIONS_LIMIT) {
			LOG_ERR_RATELIMIT_RATE(1000U, "%d subsequent time corrections",
					       ctx->subsequent_rapid_corrections);
		}

		/* There are several options which can occur at this point:
		 *
		 * - corr_diff_us == 0: The estimate is correct, no correction needed.
		 *
		 * - corr_diff_us >> 0: Data was given to this module too slow/late.
		 * Hence, the controller schedules sending for the next ISO
		 * interval. An empty packet will be sent on air.
		 *
		 * - corr_diff_us << 0: Data was given to this module too fast/early.
		 *
		 * - corr_diff_us > or < 0: This means that the estimate has gotten a small
		 * correction. As a central/gateway/unicast client/broadcast source, the
		 * clocks shall be running in lockstep and this should never happen. As a
		 * peripheral/headset/unicast server/broadcast sink, this will occur to
		 * correct for drift.
		 */

		const int64_t half_common_interval_us = (int64_t)common_interval_us / 2;

		if (ctx->corr_diff_us == 0) {
			/* No correction needed, the estimate is correct. */
			LOG_DBG("TX clock no adjustment needed (%lld us)", ctx->corr_diff_us);
			ctx->ts_ctlr_esti_us_valid = true;
		} else if (ctx->corr_diff_us > half_common_interval_us) {
			LOG_DBG("TX clock corrected by (%lld us). Late: Empty packet(s) on air",
				ctx->corr_diff_us);
			ctx->ts_ctlr_esti_us_valid = false;
			*status = -ETIMEDOUT;
		} else if (ctx->corr_diff_us < -half_common_interval_us) {
			LOG_DBG("TX clock corrected by (%lld us). Early", ctx->corr_diff_us);
			ctx->ts_ctlr_esti_us_valid = false;
		} else {
			if (ctx->is_ble_clock_master) {
				/* The gateway is the Bluetooth central. This shall not happen */
				LOG_ERR("TX clock has been drift adjusted by (%lld us)",
					ctx->corr_diff_us);
				ctx->ts_ctlr_esti_us_valid = false;
			} else {
				LOG_DBG("TX clock has been drift adjusted by (%lld us)",
					ctx->corr_diff_us);
				ctx->ts_ctlr_esti_us_valid = true;
			}
		}

		ctx->ts_ctlr_real_us_last = ts_ctlr_real_us;
		/* Update the estimate to the real (most current) value */
		ctx->ts_ctlr_esti_us = ts_ctlr_real_us;
		ctx->ts_last_correction = ts_now_us;
	}

	return 0;
}

static void tx_publish_sdu_ref_and_finalize(struct bt_le_audio_tx_ctx *ctx, uint32_t ts_now_us)
{
	int ret;
	struct sdu_ref_msg msg;

	msg.tx_sync_ts_us = ctx->ts_ctlr_esti_us;
	msg.curr_ts_us = ts_now_us;
	msg.adjust = true;

	ret = zbus_chan_pub(&sdu_ref_chan, &msg, K_NO_WAIT);
	if (ret != 0) {
		LOG_WRN("Failed to publish timestamp: %d", ret);
	}

	ctx->ts_last_us = ts_now_us;
	ctx->ts_last_us_valid = true;
}

int bt_le_audio_tx_send(struct bt_le_audio_tx_ctx *ctx, struct net_buf const *const audio_frame,
			struct le_audio_tx_info *tx, uint8_t num_tx)
{
	int ret;
	int status = 0;
	size_t data_size_pr_stream = 0U;

	if (unlikely(ctx == NULL || tx == NULL || num_tx == 0U || audio_frame == NULL)) {
		return -EINVAL;
	}

	ret = tx_entries_validate(tx, num_tx);
	if (ret != 0) {
		return ret;
	}

	uint8_t num_loc;

	ret = num_loc_get(audio_frame, &num_loc);
	if (ret != 0) {
		return ret;
	}

	if (num_loc > num_tx) {
		/* May happen if we are trying to send N streams, but N-X locations are available */
		LOG_DBG("Number of locations (%u) more than number of streams (%u)", num_loc,
			num_tx);
	} else if (num_loc < num_tx) {
		if (num_loc != 1U) {
			LOG_ERR("If number of locations (%u) is less than number of streams (%u) "
				"it must be 1. Single loc. sent to all streams.",
				num_loc, num_tx);
			return -EINVAL;
		}

		if (!IS_ENABLED(CONFIG_MONO_TO_ALL_RECEIVERS)) {
			LOG_ERR("Num locations (%u) sent to all streams (%u)."
				"CONFIG_MONO_TO_ALL_RECEIVERS must be enabled",
				num_loc, num_tx);
			return -EINVAL;
		}
	}

	uint32_t common_interval_us = 0U;
	uint8_t num_streaming_state = 0U;

	data_size_pr_stream = audio_frame->len / num_loc;
	if (unlikely(data_size_pr_stream == 0U)) {
		LOG_ERR("Data size per stream is 0");
		return -EINVAL;
	}

	ret = tx_precheck(tx, num_tx, data_size_pr_stream, &common_interval_us,
			  &num_streaming_state);
	if (ret != 0) {
		return ret;
	}

	if (num_streaming_state == 0U) {
		LOG_DBG("No streams are in streaming state");
		return 0;
	}

	uint8_t sent_streams = 0U;
	const struct bt_bap_stream *last_successful_bap_stream = NULL;
	struct tx_inf *tx_info = NULL;

	if (common_interval_us == 0U) {
		LOG_ERR("common_interval is zero");
		return -EINVAL;
	}

	ret = tx_ts_prepare(ctx, common_interval_us);
	if (ret != 0) {
		return ret;
	}

	LOG_DBG_RATELIMIT_RATE(1000, "tx %d valid %d", num_tx, ctx->ts_ctlr_esti_us_valid);

	uint32_t ts_now_us = audio_sync_timer_capture();

	tx_call_interval_check(ctx, ts_now_us, common_interval_us);

	sent_streams = tx_streams_send(ctx, audio_frame, tx, num_tx, num_loc, data_size_pr_stream,
				       &last_successful_bap_stream, &tx_info);

	if (sent_streams == 0U) {
		/* If nothing was sent, do not continue with timestamp handling
		 * Info is printed in tx_streams_send().
		 */
		LOG_DBG("No streams were sent");
		return -EIO;
	}

	if (last_successful_bap_stream == NULL || tx_info == NULL) {
		LOG_ERR("No successful stream available for timestamp query");
		return -EIO;
	}

	/* Get the current (wall clock) time */
	ts_now_us = audio_sync_timer_capture();

	ret = tx_controller_sync_and_correct(ctx, last_successful_bap_stream, common_interval_us,
					     ts_now_us, tx_info, &status);
	if (ret != 0) {
		return ret;
	}

	if ((int32_t)(ts_now_us - ctx->ts_ctlr_esti_us) > 0) {
		LOG_WRN("Curr time ahead of ctlr time. Flush %u us of data. "
			"Now: %u us, ctlr: %u us",
			common_interval_us, ts_now_us, ctx->ts_ctlr_esti_us);
		ctx->flush_next = true;
		ctx->ts_ctlr_esti_us_valid = false;
	}

	tx_publish_sdu_ref_and_finalize(ctx, ts_now_us);
	return status;
}

int bt_le_audio_tx_stream_started(struct bt_le_audio_tx_ctx *ctx, struct stream_index idx)
{
	int ret;

	if (ctx == NULL) {
		return -EINVAL;
	}

	ret = verify_stream_idx(idx);
	if (ret != 0) {
		return ret;
	}

	(void)atomic_clear(&ctx->tx_info_arr[idx.lvl1][idx.lvl2][idx.lvl3].iso_tx_pool_alloc);

	ctx->tx_info_arr[idx.lvl1][idx.lvl2][idx.lvl3].hci_wrn_printed = false;
	ctx->tx_info_arr[idx.lvl1][idx.lvl2][idx.lvl3].iso_conn_handle = HANDLE_INVALID;
	ctx->tx_info_arr[idx.lvl1][idx.lvl2][idx.lvl3].iso_tx_readback.seq_num = 0U;
	return 0;
}

int bt_le_audio_tx_stream_sent(struct bt_le_audio_tx_ctx *ctx, struct stream_index stream_idx)
{
	int ret;

	if (ctx == NULL) {
		return -EINVAL;
	}

	ret = verify_stream_idx(stream_idx);
	if (ret != 0) {
		return ret;
	}

	(void)atomic_dec(&ctx->tx_info_arr[stream_idx.lvl1][stream_idx.lvl2][stream_idx.lvl3]
				  .iso_tx_pool_alloc);
	return 0;
}

int bt_le_audio_tx_init(struct bt_le_audio_tx_ctx *ctx, bool is_clock_master)
{
	if (ctx == NULL || ctx->iso_tx_pool == NULL) {
		return -EINVAL;
	}

	struct net_buf_pool *tmp = ctx->iso_tx_pool;

	(void)memset(ctx, 0, sizeof(*ctx));

	ctx->iso_tx_pool = tmp;
	ctx->is_ble_clock_master = is_clock_master;

	for (int i = 0; i < GROUP_MAX; i++) {
		for (int j = 0; j < SUBGROUP_MAX; j++) {
			for (int k = 0; k < TX_STREAMS_MAX; k++) {
				ctx->tx_info_arr[i][j][k].iso_conn_handle = HANDLE_INVALID;
			}
		}
	}

	return 0;
}
