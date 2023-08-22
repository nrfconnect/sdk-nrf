/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <nrfx_clock.h>

#include "nrf5340_audio_common.h"
#include "streamctrl.h"
#include "audio_datapath.h"
#include "macros_common.h"
#include "audio_system.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(le_audio_rx, CONFIG_LE_AUDIO_RX_LOG_LEVEL);

struct ble_iso_data {
	uint8_t data[CONFIG_BT_ISO_RX_MTU];
	size_t data_size;
	bool bad_frame;
	uint32_t sdu_ref;
	uint32_t recv_frame_ts;
} __packed;

struct rx_stats {
	uint32_t recv_cnt;
	uint32_t bad_frame_cnt;
	uint32_t data_size_mismatch_cnt;
};

static bool initialized;
static struct k_thread audio_datapath_thread_data;
static k_tid_t audio_datapath_thread_id;
K_THREAD_STACK_DEFINE(audio_datapath_thread_stack, CONFIG_AUDIO_DATAPATH_STACK_SIZE);

DATA_FIFO_DEFINE(ble_fifo_rx, CONFIG_BUF_BLE_RX_PACKET_NUM, WB_UP(sizeof(struct ble_iso_data)));

/* Callback for handling ISO RX */
void le_audio_rx_data_handler(uint8_t const *const p_data, size_t data_size, bool bad_frame,
			      uint32_t sdu_ref, enum audio_channel channel_index,
			      size_t desired_data_size)
{
	int ret;
	bool data_size_mismatch = false;
	uint32_t blocks_alloced_num, blocks_locked_num;
	struct ble_iso_data *iso_received = NULL;
	static struct rx_stats rx_stats[AUDIO_CH_NUM];

	if (!initialized) {
		ERR_CHK_MSG(-EPERM, "Data received but le_audio_rx is not initialized");
	}

	/* Capture timestamp of when audio frame is received */
	uint32_t recv_frame_ts = nrfx_timer_capture(&audio_sync_timer_instance,
						    AUDIO_SYNC_TIMER_CURR_TIME_CAPTURE_CHANNEL);

	rx_stats[channel_index].recv_cnt++;
	if (data_size != desired_data_size && !bad_frame) {
		data_size_mismatch = true;
		rx_stats[channel_index].data_size_mismatch_cnt++;
	}

	if (bad_frame) {
		rx_stats[channel_index].bad_frame_cnt++;
	}

	if ((rx_stats[channel_index].recv_cnt % 100) == 0 && rx_stats[channel_index].recv_cnt) {
		/* NOTE: The string below is used by the Nordic CI system */
		LOG_DBG("ISO RX SDUs: Ch: %d Total: %d Bad: %d Size mismatch %d", channel_index,
			rx_stats[channel_index].recv_cnt, rx_stats[channel_index].bad_frame_cnt,
			rx_stats[channel_index].data_size_mismatch_cnt);
	}

	if (data_size_mismatch) {
		/* Return if sizes do not match */
		LOG_WRN("Data size mismatch");
		return;
	}

	if (stream_state_get() != STATE_STREAMING) {
		/* Throw away data */
		LOG_WRN("Not in streaming state, throwing data: %d", stream_state_get());
		return;
	}

	if (channel_index != AUDIO_CH_L && (CONFIG_AUDIO_DEV == GATEWAY)) {
		/* Only left channel RX data in use on gateway */
		return;
	}

	ret = data_fifo_num_used_get(&ble_fifo_rx, &blocks_alloced_num, &blocks_locked_num);
	ERR_CHK(ret);

	if (blocks_alloced_num >= CONFIG_BUF_BLE_RX_PACKET_NUM) {
		/* FIFO buffer is full, swap out oldest frame for a new one */

		void *stale_data;
		size_t stale_size;

		LOG_WRN("BLE ISO RX overrun");

		ret = data_fifo_pointer_last_filled_get(&ble_fifo_rx, &stale_data, &stale_size,
							K_NO_WAIT);
		ERR_CHK(ret);

		data_fifo_block_free(&ble_fifo_rx, stale_data);
	}

	ret = data_fifo_pointer_first_vacant_get(&ble_fifo_rx, (void *)&iso_received, K_NO_WAIT);
	ERR_CHK_MSG(ret, "Unable to get FIFO pointer");

	if (data_size > ARRAY_SIZE(iso_received->data)) {
		ERR_CHK_MSG(-ENOMEM, "Data size too large for buffer");
		return;
	}

	memcpy(iso_received->data, p_data, data_size);

	iso_received->bad_frame = bad_frame;
	iso_received->data_size = data_size;
	iso_received->sdu_ref = sdu_ref;
	iso_received->recv_frame_ts = recv_frame_ts;

	ret = data_fifo_block_lock(&ble_fifo_rx, (void *)&iso_received,
				   sizeof(struct ble_iso_data));
	ERR_CHK_MSG(ret, "Failed to lock block");
}

/**
 * @brief	Receive data from BLE through a k_fifo and send to USB or audio datapath.
 */
static void audio_datapath_thread(void *dummy1, void *dummy2, void *dummy3)
{
	int ret;
	struct ble_iso_data *iso_received = NULL;
	size_t iso_received_size;

	while (1) {
		ret = data_fifo_pointer_last_filled_get(&ble_fifo_rx, (void *)&iso_received,
							&iso_received_size, K_FOREVER);
		ERR_CHK(ret);

		if (IS_ENABLED(CONFIG_AUDIO_SOURCE_USB) && (CONFIG_AUDIO_DEV == GATEWAY)) {
			ret = audio_system_decode(iso_received->data, iso_received->data_size,
						  iso_received->bad_frame);
			ERR_CHK(ret);
		} else {
			audio_datapath_stream_out(iso_received->data, iso_received->data_size,
						  iso_received->sdu_ref, iso_received->bad_frame,
						  iso_received->recv_frame_ts);
		}
		data_fifo_block_free(&ble_fifo_rx, (void *)iso_received);

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
