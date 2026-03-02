/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "bt_le_audio_tx.h"

#include <stdlib.h>

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

#define HCI_ISO_BUF_PER_CHAN 2

#if defined(CONFIG_BT_ISO_MAX_CIG) && defined(CONFIG_BT_ISO_MAX_BIG)
#define GROUP_MAX (CONFIG_BT_ISO_MAX_BIG + CONFIG_BT_ISO_MAX_CIG)
#elif defined(CONFIG_BT_ISO_MAX_CIG)
#define GROUP_MAX CONFIG_BT_ISO_MAX_CIG
#elif defined(CONFIG_BT_ISO_MAX_BIG)
#define GROUP_MAX CONFIG_BT_ISO_MAX_BIG
#else
#error Neither CIG nor BIG defined
#endif

#if (defined(CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT) && defined(CONFIG_BT_BAP_UNICAST))
/* 1 since CIGs doesn't have the concept of subgroups */
#define SUBGROUP_MAX (1 + CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT)

#elif (defined(CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT) && !defined(CONFIG_BT_BAP_UNICAST))
#define SUBGROUP_MAX CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT

#else
/* 1 since CIGs doesn't have the concept of subgroups */
#define SUBGROUP_MAX 1
#endif

/* Time between performing a timestamp re-sync with the Bluetooth controller.
 * For gateway/central (unicast client/broadcast source) where this device is the master
 * Bluetooth LE clocking device, the wall clock and BLE clock should be in sync.
 * Hence, there is no strict need for regular re-sync after the first timestamp is obtained.
 * For simplicity and risk mitigation, a periodic re-sync is still performed.
 * However, for the headset/peripheral (unicast server/broadcast sink)
 * regular re-sync is needed to compensate for clock drift.
 */
#define TX_TS_RESYNC_US	 1000000
#define TX_MARGIN_MIN_US (CONFIG_TX_TGT_LEAD_TIME_US - CONFIG_TX_LEAD_TIME_DEVIATION_US)
#define TX_MARGIN_MAX_US (CONFIG_TX_TGT_LEAD_TIME_US + CONFIG_TX_LEAD_TIME_DEVIATION_US)

/* When starting a stream, I2S or USB feeding the TX function with data, will usually need some time
 * to stabilize and potentially drop data to meet just-in-time requirements.
 */
#define SUBSEQUENT_RAPID_CORRECTIONS_LIMIT 20

/* Since we can't assume that the number of streams are equally distributed on the subgroups ,we
 * need to allocate the max number per subgroup
 */
#define STREAMS_MAX (CONFIG_BT_ISO_MAX_CHAN)

#define NET_BUF_POOL_MAX ((GROUP_MAX) * (SUBGROUP_MAX) * (STREAMS_MAX) * (HCI_ISO_BUF_PER_CHAN))

NET_BUF_POOL_FIXED_DEFINE(iso_tx_pool, NET_BUF_POOL_MAX, BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

struct tx_inf {
	uint16_t iso_conn_handle;
	struct bt_iso_tx_info iso_tx;
	struct bt_iso_tx_info iso_tx_readback;
	atomic_t iso_tx_pool_alloc;
	bool hci_wrn_printed;
};

static bool initialized;
static struct tx_inf tx_info_arr[GROUP_MAX][SUBGROUP_MAX][STREAMS_MAX];

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
			   struct tx_inf *tx_info, uint32_t ts_tx, bool send_ts_tx)
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

	buf = net_buf_alloc(&iso_tx_pool, K_NO_WAIT);
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

int bt_le_audio_tx_send(struct net_buf const *const audio_frame, struct le_audio_tx_info *tx,
			uint8_t num_tx)
{
	int ret;
	size_t data_size_pr_stream = 0;

	if (!initialized) {
		return -EACCES;
	}

	if (tx == NULL || audio_frame == NULL) {
		return -EINVAL;
	}

	/* Get number of locations in the audio frame */
	struct audio_metadata *meta = net_buf_user_data(audio_frame);
	uint8_t num_loc = audio_metadata_num_loc_get(meta);

	if (num_loc == 0 || (audio_frame->len % num_loc != 0)) {
		LOG_ERR("Invalid number (%d) of locations in audio frame (%d)", num_loc,
			audio_frame->len);
		return -EINVAL;
	}

	if (meta == NULL || meta->prod_inf == NULL) {
		LOG_ERR("Audio metadata or producer info is missing");
		return -EINVAL;
	}

	bool prod_can_rate_adj = meta->prod_inf->rate_adjustable;
	data_size_pr_stream = audio_frame->len / num_loc;
	uint32_t common_interval_us = 0;

	ret = tx_precheck(tx, num_tx, num_loc, data_size_pr_stream, &common_interval_us);
	if (ret) {
		return ret;
	}

	/* The timestamp used when sending*/
	static uint32_t timestamp_ctlr_esti_us;
	static bool timestamp_ctlr_esti_us_valid;
	uint8_t sent_streams = 0;
	struct tx_inf *tx_info = NULL;

	/* Increment the tx timestamp with the interval.
	 * If this TX function is called too fast or slow, the estimate will deteriorate faster.
	 */
	timestamp_ctlr_esti_us += common_interval_us;

	LOG_INF_RATELIMIT_RATE(1000, "tx %d valid %d", num_tx, timestamp_ctlr_esti_us_valid);

	for (int i = 0; i < num_tx; i++) {
		tx_info = &tx_info_arr[tx[i].idx.lvl1][tx[i].idx.lvl2][tx[i].idx.lvl3];

		/* Check if same audio is sent to all locations */
		if (num_loc == 1) {
			ret = iso_stream_send(audio_frame->data, data_size_pr_stream,
					      tx[i].cap_stream, tx_info, timestamp_ctlr_esti_us,
					      timestamp_ctlr_esti_us_valid && prod_can_rate_adj);
		} else {
			ret = iso_stream_send(audio_frame->data +
						      (tx[i].audio_location * data_size_pr_stream),
					      data_size_pr_stream, tx[i].cap_stream, tx_info,
					      timestamp_ctlr_esti_us,
					      timestamp_ctlr_esti_us_valid && prod_can_rate_adj);
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

	/* Get the current (wall clock time) */
	uint32_t timestamp_now_us = audio_sync_timer_capture();
	uint32_t timestamp_ctlr_real_us = 0;
	static uint32_t timestamp_ctlr_real_us_last;

	/* If the timestamp is not valid or it has been more than TX_TS_RESYNC_US since the last
	 * timestamp, we re-aquire a timestamp from the controller.
	 */
	if (!timestamp_ctlr_esti_us_valid ||
	    (timestamp_ctlr_esti_us - timestamp_ctlr_real_us_last) >= TX_TS_RESYNC_US) {
		static uint32_t timestamp_last_correction;
		static uint32_t subequent_rapid_corrections;

		ret = controller_timestamp_tx_get(tx, num_tx, &timestamp_ctlr_real_us, tx_info);
		if (ret) {
			LOG_ERR("Failed to get timestamp after sending: %d", ret);
			return ret;
		}

		uint32_t time_since_last_corr_us = timestamp_now_us - timestamp_last_correction;
		int32_t corr_diff = (int32_t)(timestamp_ctlr_real_us - timestamp_ctlr_esti_us);
		int32_t corr_ppm = (int32_t)(((int64_t)corr_diff * 1000000LL) /
					     (int64_t)time_since_last_corr_us);

		if (time_since_last_corr_us < TX_TS_RESYNC_US) {
			subequent_rapid_corrections++;
		} else {
			subequent_rapid_corrections = 0;
		}

		if (subequent_rapid_corrections > SUBSEQUENT_RAPID_CORRECTIONS_LIMIT) {
			LOG_WRN_RATELIMIT_RATE(5000,
					       "%d subsequent time corrections. Expected for "
					       "synchronous USB",
					       subequent_rapid_corrections);
		}

		LOG_INF_RATELIMIT_RATE(1000,
				       "TX clock updated by %d ppm - corr_diff: %d us, "
				       "estimate: %u corrected to %u",
				       corr_ppm, corr_diff, timestamp_ctlr_esti_us,
				       timestamp_ctlr_real_us);

		timestamp_ctlr_real_us_last = timestamp_ctlr_real_us;
		/* Update the estimate to the real (most current) value */
		timestamp_ctlr_esti_us = timestamp_ctlr_real_us;
		timestamp_ctlr_esti_us_valid = true;
		timestamp_last_correction = timestamp_now_us;
	}

	/* At this point, the controller estimated timestamp is valid */
	struct sdu_ref_msg msg;

	msg.tx_sync_ts_us = timestamp_ctlr_esti_us;
	msg.curr_ts_us = timestamp_now_us;
	msg.adjust = true;

	ret = zbus_chan_pub(&sdu_ref_chan, &msg, K_NO_WAIT);
	if (ret) {
		LOG_WRN("Failed to publish timestamp: %d", ret);
	}

	int32_t diff_us = (int32_t)(timestamp_ctlr_esti_us - timestamp_now_us);

	LOG_DBG_RATELIMIT_RATE(1000,
			       "Controller timestamp: %u us, Current timestamp: %u us, Diff: %d us",
			       timestamp_ctlr_esti_us, timestamp_now_us, diff_us);

	if (diff_us < 0 && prod_can_rate_adj) {
		/* Late TX / too little data provided. Controller will flush data */
		LOG_INF("TX late. Diff: %d us, ctlr_est: %u now: %u", diff_us,
			timestamp_ctlr_esti_us, timestamp_now_us);
		timestamp_ctlr_esti_us_valid = false;
		timestamp_ctlr_esti_us = 0;
	} else if ((diff_us < TX_MARGIN_MIN_US) && prod_can_rate_adj) {
		/* Late TX / too little data provided. Data may be lost */
		LOG_INF("TX close to next window. Diff: %d us, ctlr_est: %u now: %u", diff_us,
			timestamp_ctlr_esti_us, timestamp_now_us);
		timestamp_ctlr_esti_us_valid = false;
		timestamp_ctlr_esti_us = 0;
	} else if ((diff_us > TX_MARGIN_MAX_US) && prod_can_rate_adj) {
		/* Early TX / too much data. Controller will buffer and latency increase */
		LOG_INF("TX early. Diff: %d us, ctlr_est: %u now: %u", diff_us,
			timestamp_ctlr_esti_us, timestamp_now_us);
		/* Not critical, but can update timestamp to better align with controller */
		timestamp_ctlr_esti_us_valid = false;
		timestamp_ctlr_esti_us = 0;
	} else {
		/* Timestamp is valid and within an acceptable range */
	}

	return 0;
}

int bt_le_audio_tx_stream_started(struct stream_index idx)
{
	if (!initialized) {
		return -EACCES;
	}

	atomic_clear(&tx_info_arr[idx.lvl1][idx.lvl2][idx.lvl3].iso_tx_pool_alloc);

	tx_info_arr[idx.lvl1][idx.lvl2][idx.lvl3].hci_wrn_printed = false;
	tx_info_arr[idx.lvl1][idx.lvl2][idx.lvl3].iso_conn_handle = HANDLE_INVALID;
	tx_info_arr[idx.lvl1][idx.lvl2][idx.lvl3].iso_tx.seq_num = 0;
	tx_info_arr[idx.lvl1][idx.lvl2][idx.lvl3].iso_tx_readback.seq_num = 0;
	return 0;
}

int bt_le_audio_tx_stream_sent(struct stream_index stream_idx)
{
	if (!initialized) {
		return -EACCES;
	}

	atomic_dec(
		&tx_info_arr[stream_idx.lvl1][stream_idx.lvl2][stream_idx.lvl3].iso_tx_pool_alloc);
	return 0;
}

void bt_le_audio_tx_init(void)
{
	if (initialized) {
		/* If TX is disabled and enabled again this should be called to reset the state */
		LOG_DBG("Already initialized");
	}

	for (int i = 0; i < GROUP_MAX; i++) {
		for (int j = 0; j < SUBGROUP_MAX; j++) {
			for (int k = 0; k < STREAMS_MAX; k++) {
				tx_info_arr[i][j][k].hci_wrn_printed = false;
				tx_info_arr[i][j][k].iso_conn_handle = HANDLE_INVALID;
				tx_info_arr[i][j][k].iso_tx.ts = 0;
				tx_info_arr[i][j][k].iso_tx.offset = 0;
				tx_info_arr[i][j][k].iso_tx.seq_num = 0;
				tx_info_arr[i][j][k].iso_tx_readback.ts = 0;
				tx_info_arr[i][j][k].iso_tx_readback.offset = 0;
				tx_info_arr[i][j][k].iso_tx_readback.seq_num = 0;
			}
		}
	}

	initialized = true;
}
