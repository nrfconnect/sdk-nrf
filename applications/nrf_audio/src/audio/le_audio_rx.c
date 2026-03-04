/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "le_audio_rx.h"

#include <stdlib.h>
#include <zephyr/kernel.h>
#include <nrfx_clock.h>

#include "streamctrl.h"
#include "audio_datapath.h"
#include "macros_common.h"
#include "audio_system.h"
#include "audio_sync_timer.h"
#include "audio_defines.h"
#include "le_audio.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(le_audio_rx, CONFIG_LE_AUDIO_RX_LOG_LEVEL);

struct rx_stats {
	uint32_t recv_cnt;
	uint32_t bad_frame_cnt;
};

static bool initialized;
static struct k_thread audio_datapath_thread_data;
static k_tid_t audio_datapath_thread_id;
K_THREAD_STACK_DEFINE(audio_datapath_thread_stack, CONFIG_AUDIO_DATAPATH_STACK_SIZE);

K_MSGQ_DEFINE(ble_q_rx, sizeof(struct net_buf *), CONFIG_BUF_BLE_RX_PACKET_NUM, sizeof(void *));
NET_BUF_POOL_FIXED_DEFINE(ble_rx_pool, CONFIG_BUF_BLE_RX_PACKET_NUM,
			  (CONFIG_BT_ISO_RX_MTU * CONFIG_BT_AUDIO_CONCURRENT_RX_STREAMS_MAX),
			  sizeof(struct audio_metadata), NULL);

static int location_to_net_buf_index(enum bt_audio_location total_loc,
				     enum bt_audio_location srch_loc, uint8_t *index)
{
	uint8_t idx = 0;

	while (total_loc) {
		if (srch_loc & 0x01) {
			*index = idx;
			return 0;
		}

		/* Count the number of active locations */
		if (total_loc & 0x01) {
			idx++;
		}

		total_loc >>= 1;
		srch_loc >>= 1;
	}

	return -EINVAL;
}

static int audio_frame_add(struct net_buf *audio_frame, struct net_buf const *const audio_frame_rx,
			   struct audio_metadata *meta, enum bt_audio_location stream_locations)
{
	int ret;
	uint32_t offset = 0;

	if (audio_frame == NULL || audio_frame_rx == NULL || meta == NULL) {
		LOG_ERR("Invalid input parameters");
		return -EINVAL;
	}

	if (meta->locations != stream_locations) {
		/* Get net_buf_index for given location */
		uint8_t net_buf_index = 0;

		ret = location_to_net_buf_index(stream_locations, meta->locations, &net_buf_index);
		if (ret) {
			LOG_ERR("Invalid location index, ret: %d", ret);
			return ret;
		}

		/* Calculate offset in net_buf */
		offset = net_buf_index * meta->bytes_per_location;

		if (offset > audio_frame->size - audio_frame_rx->len) {
			LOG_ERR("Calculated offset too large: %d", offset);
			return -EINVAL;
		}
	}

	if (audio_frame_rx->len && !meta->bad_data) {
		/* Copy audio data to the correct offset */
		memcpy(&audio_frame->data[offset], audio_frame_rx->data, audio_frame_rx->len);
	}

	return 0;
}

/* Callback for handling ISO RX */
void le_audio_rx_data_handler(struct net_buf *audio_frame_rx, struct audio_metadata *meta,
			      uint8_t location_index)
{
	int ret;
	static struct net_buf *audio_frame;
	static struct rx_stats rx_stats[CONFIG_BT_AUDIO_CONCURRENT_RX_STREAMS_MAX];
	static uint32_t num_overruns;
	static uint32_t num_thrown;

	if (!initialized) {
		ERR_CHK_MSG(-EPERM, "Data received but le_audio_rx is not initialized");
	}

	rx_stats[location_index].recv_cnt++;

	if (meta->bad_data) {
		rx_stats[location_index].bad_frame_cnt++;
	}

	if ((rx_stats[location_index].recv_cnt % 100) == 0 && rx_stats[location_index].recv_cnt) {
		/* NOTE: The string below is used by the Nordic CI system */
		LOG_DBG("ISO RX SDUs: Loc: %d Total: %d Bad: %d", location_index,
			rx_stats[location_index].recv_cnt, rx_stats[location_index].bad_frame_cnt);
	}

	if (stream_state_get() != STATE_STREAMING) {
		/* Throw away data */
		num_thrown++;
		if ((num_thrown % 100) == 1) {
			LOG_WRN("Not in streaming state (%d), thrown %d packet(s)",
				stream_state_get(), num_thrown);
		}
		return;
	}

	if (location_index != 0 && (CONFIG_AUDIO_DEV == GATEWAY)) {
		/* Only the first device will be used as mic input on gateway */
		return;
	}

	if (k_msgq_num_free_get(&ble_q_rx) == 0) {
		struct net_buf *stale_buf = NULL;
		/* FIFO buffer is full, swap out oldest frame for a new one */
		ret = k_msgq_get(&ble_q_rx, (void *)&stale_buf, K_NO_WAIT);
		/* Checking return value of k_msgq_get() is not necessary here,
		 * as we are already checking for free space above.
		 */
		if (stale_buf != NULL) {
			num_overruns++;

			if ((num_overruns % 100) == 1) {
				LOG_WRN("BLE ISO RX overrun: Num: %d", num_overruns);
			}

			net_buf_unref(stale_buf);
		}
	}

	uint8_t num_active_streams = 0;
	enum bt_audio_location stream_locations = 0;

	ret = le_audio_concurrent_sync_num_get(&num_active_streams, &stream_locations);
	if (ret) {
		LOG_ERR("Failed to get number of active streams: %d", ret);
		return;
	}

	/* Check if we already have received a frame with the same timestamp */
	if (audio_frame != NULL) {
		struct audio_metadata *existing_meta = net_buf_user_data(audio_frame);
		uint32_t ref_ts_diff = abs((meta->ref_ts_us - existing_meta->ref_ts_us));

		if (ref_ts_diff < SDU_REF_CH_DELTA_MAX_US) {
			/* We already have a frame with this timestamp, so we will add these
			 * together if the locations are different
			 */
			if (existing_meta->locations != meta->locations) {
				/* Add location and bad_frame status to the joint audio frame */
				existing_meta->locations |= meta->locations;
				existing_meta->bad_data |= meta->bad_data;

				ret = audio_frame_add(audio_frame, audio_frame_rx, meta,
						      stream_locations);
				if (ret) {
					LOG_ERR("Failed to add audio frame data");
					net_buf_unref(audio_frame);
					audio_frame = NULL;
					return;
				}

				/* Check if we need to send the frame */
				goto check_send;
			} else {
				LOG_ERR("Received audio frame with same timestamp and same "
					"locations, discarding new data");
				return;
			}
		} else {
			/* We have a new frame with a different timestamp, so we will send the
			 * old frame and create a new one. This will usually happen if we miss a
			 * frame or we suddenly go from one to two or more streams.
			 */
			LOG_WRN("Received audio frame with different timestamp, sending old frame, "
				"old ts: %u, new ts: %u",
				existing_meta->ref_ts_us, meta->ref_ts_us);

			ret = k_msgq_put(&ble_q_rx, (void *)&audio_frame, K_NO_WAIT);
			ERR_CHK_MSG(ret, "Failed to put audio frame into queue");
			audio_frame = NULL;
		}
	}

	/* Capture timestamp of when audio frame is received */
	meta->data_rx_ts_us = audio_sync_timer_capture();

	audio_frame = net_buf_alloc(&ble_rx_pool, K_NO_WAIT);
	if (audio_frame == NULL) {
		LOG_WRN("Out of RX buffers");
		return;
	}

	/* Store metadata from the first frame */
	memcpy(net_buf_user_data(audio_frame), meta, sizeof(struct audio_metadata));

	/* Set aside memory in net_buf for all locations */
	net_buf_add(audio_frame, meta->bytes_per_location * num_active_streams);

	ret = audio_frame_add(audio_frame, audio_frame_rx, meta, stream_locations);
	if (ret) {
		LOG_ERR("Failed to add audio frame data");
		net_buf_unref(audio_frame);
		audio_frame = NULL;
		return;
	}

	/* Check if we have received all frames, send if we have */
check_send:
	if (num_active_streams == audio_metadata_num_loc_get(net_buf_user_data(audio_frame))) {
		/* We have received all frames we are waiting for, pass data on to
		 * the next module
		 */
		ret = k_msgq_put(&ble_q_rx, (void *)&audio_frame, K_NO_WAIT);
		ERR_CHK_MSG(ret, "Failed to put audio frame into queue");
		audio_frame = NULL;
	}
}

/**
 * @brief	Receive data from BLE through a k_fifo and send to USB or audio datapath.
 */
static void audio_datapath_thread(void *dummy1, void *dummy2, void *dummy3)
{
	int ret;
	struct net_buf *audio_frame = NULL;

	while (1) {
		ret = k_msgq_get(&ble_q_rx, (void *)&audio_frame, K_FOREVER);
		ERR_CHK(ret);

		if (IS_ENABLED(CONFIG_AUDIO_SOURCE_USB) && (CONFIG_AUDIO_DEV == GATEWAY)) {
			ret = audio_system_decode(audio_frame);
			ERR_CHK(ret);
		} else {
			audio_datapath_stream_out(audio_frame);
		}

		net_buf_unref(audio_frame);

		STACK_USAGE_PRINT("audio_datapath_thread", &audio_datapath_thread_data);
	}
}

static int audio_datapath_thread_create(void)
{
	int ret;

	audio_datapath_thread_id = k_thread_create(
		&audio_datapath_thread_data, audio_datapath_thread_stack,
		CONFIG_AUDIO_DATAPATH_STACK_SIZE, (k_thread_entry_t)audio_datapath_thread, NULL,
		NULL, NULL, K_PRIO_PREEMPT(CONFIG_AUDIO_DATAPATH_THREAD_PRIO), 0, K_NO_WAIT);
	ret = k_thread_name_set(audio_datapath_thread_id, "Audio_datapath");
	if (ret) {
		LOG_ERR("Failed to create audio_datapath thread");
		return ret;
	}

	return 0;
}

int le_audio_rx_init(void)
{
	int ret;

	if (initialized) {
		return -EALREADY;
	}

	ret = audio_datapath_thread_create();
	if (ret) {
		return ret;
	}

	initialized = true;

	return 0;
}
