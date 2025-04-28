/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "le_audio_rx.h"

#include <zephyr/kernel.h>
#include <nrfx_clock.h>
#include <zephyr/net_buf.h>

#include "streamctrl.h"
#include "audio_datapath.h"
#include "macros_common.h"
#include "audio_system.h"
#include "audio_sync_timer.h"
#include "audio_defines.h"

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

DATA_FIFO_DEFINE(ble_fifo_rx, CONFIG_BUF_BLE_RX_PACKET_NUM, WB_UP(sizeof(struct audio_data)));
NET_BUF_POOL_FIXED_DEFINE(ble_rx_pool, CONFIG_BUF_BLE_RX_PACKET_NUM, CONFIG_BT_ISO_RX_MTU, 0, NULL);

/* Callback for handling ISO RX */
void le_audio_rx_data_handler(struct audio_data *audio_frame, uint8_t channel_index)
{
	int ret;
	uint32_t blocks_alloced_num, blocks_locked_num;
	struct audio_data *audio_frame_received = NULL;
	struct net_buf *audio_buf;
	static struct rx_stats rx_stats[AUDIO_CH_NUM];
	static uint32_t num_overruns;
	static uint32_t num_thrown;

	if (!initialized) {
		ERR_CHK_MSG(-EPERM, "Data received but le_audio_rx is not initialized");
	}

	/* Capture timestamp of when audio frame is received */
	audio_frame->meta.data_rx_ts_us = audio_sync_timer_capture();

	rx_stats[channel_index].recv_cnt++;

	if (audio_frame->meta.bad_data) {
		rx_stats[channel_index].bad_frame_cnt++;
	}

	if ((rx_stats[channel_index].recv_cnt % 100) == 0 && rx_stats[channel_index].recv_cnt) {
		/* NOTE: The string below is used by the Nordic CI system */
		LOG_DBG("ISO RX SDUs: Ch: %d Total: %d Bad: %d", channel_index,
			rx_stats[channel_index].recv_cnt, rx_stats[channel_index].bad_frame_cnt);
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

	if (channel_index == 0 && (CONFIG_AUDIO_DEV == GATEWAY)) {
		/* Only the first device will be used as mic input on gateway */
		return;
	}

	ret = data_fifo_num_used_get(&ble_fifo_rx, &blocks_alloced_num, &blocks_locked_num);
	ERR_CHK(ret);

	if (blocks_alloced_num >= CONFIG_BUF_BLE_RX_PACKET_NUM) {
		/* FIFO buffer is full, swap out oldest frame for a new one */
		struct audio_data *stale_audio_frame;
		size_t stale_size;
		num_overruns++;

		if ((num_overruns % 100) == 1) {
			LOG_WRN("BLE ISO RX overrun: Num: %d", num_overruns);
		}

		ret = data_fifo_pointer_last_filled_get(&ble_fifo_rx, (void *)&stale_audio_frame,
							&stale_size, K_NO_WAIT);
		ERR_CHK(ret);

		struct net_buf *stale_buf = stale_audio_frame->data;

		net_buf_unref(stale_buf);
		data_fifo_block_free(&ble_fifo_rx, stale_audio_frame);
	}

	ret = data_fifo_pointer_first_vacant_get(&ble_fifo_rx, (void *)&audio_frame_received,
						 K_NO_WAIT);

	memcpy(audio_frame_received, audio_frame, sizeof(struct audio_data));
	ERR_CHK_MSG(ret, "Unable to get FIFO pointer");

	audio_buf = net_buf_alloc(&ble_rx_pool, K_NO_WAIT);
	if (audio_buf == NULL) {
		LOG_WRN("Out of RX buffers");
		return;
	}

	if (audio_frame->data_size && !audio_frame->meta.bad_data) {
		net_buf_add_mem(audio_buf, audio_frame->data, audio_frame->data_size);
	}

	audio_frame_received->data = audio_buf;

	ret = data_fifo_block_lock(&ble_fifo_rx, (void *)&audio_frame_received,
				   sizeof(struct audio_data));
	ERR_CHK_MSG(ret, "Failed to lock block");
}

/**
 * @brief	Receive data from BLE through a k_fifo and send to USB or audio datapath.
 */
static void audio_datapath_thread(void *dummy1, void *dummy2, void *dummy3)
{
	int ret;
	struct audio_data *audio_frame = NULL;
	size_t audio_frame_recv_size;

	while (1) {
		ret = data_fifo_pointer_last_filled_get(&ble_fifo_rx, (void *)&audio_frame,
							&audio_frame_recv_size, K_FOREVER);
		ERR_CHK(ret);

		if (IS_ENABLED(CONFIG_AUDIO_SOURCE_USB) && (CONFIG_AUDIO_DEV == GATEWAY)) {
			ret = audio_system_decode(audio_frame);
			ERR_CHK(ret);
		} else {
			audio_datapath_stream_out(audio_frame);
		}

		struct net_buf *audio_buf = audio_frame->data;

		net_buf_unref(audio_buf);
		data_fifo_block_free(&ble_fifo_rx, (void *)audio_frame);

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
	ret = k_thread_name_set(audio_datapath_thread_id, "AUDIO_DATAPATH");
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

	ret = data_fifo_init(&ble_fifo_rx);
	if (ret) {
		LOG_ERR("Failed to set up ble_rx FIFO");
		return ret;
	}

	ret = audio_datapath_thread_create();
	if (ret) {
		return ret;
	}

	initialized = true;

	return 0;
}
