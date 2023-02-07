/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "streamctrl.h"

#include <zephyr/kernel.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <zephyr/debug/stack.h>

#include "ctrl_events.h"
#include "led.h"
#include "button_assignments.h"
#include "macros_common.h"
#include "audio_system.h"
#include "button_handler.h"
#include "data_fifo.h"
#include "board.h"
#include "le_audio.h"
#include "audio_datapath.h"
#include "audio_sync_timer.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(streamctrl, CONFIG_STREAMCTRL_LOG_LEVEL);

struct ble_iso_data {
	uint8_t data[CONFIG_BT_ISO_RX_MTU];
	size_t data_size;
	bool bad_frame;
	uint32_t sdu_ref;
	uint32_t recv_frame_ts;
} __packed;

DATA_FIFO_DEFINE(ble_fifo_rx, CONFIG_BUF_BLE_RX_PACKET_NUM, WB_UP(sizeof(struct ble_iso_data)));

static struct k_thread audio_datapath_thread_data;
static k_tid_t audio_datapath_thread_id;
K_THREAD_STACK_DEFINE(audio_datapath_thread_stack, CONFIG_AUDIO_DATAPATH_STACK_SIZE);

static enum stream_state strm_state = STATE_PAUSED;

#if (CONFIG_BLE_ISO_TEST_PATTERN)

struct iso_recv_stats {
	uint32_t iso_rx_packets_received;
	uint32_t iso_rx_packets_lost;
};

static struct iso_recv_stats stats_overall;

/* Print statistics from test pattern run */
static void stats_print(char const *const name, struct iso_recv_stats const *const stats)
{
	uint32_t total_packets;

	total_packets = stats->iso_rx_packets_received + stats->iso_rx_packets_lost;

	LOG_INF("%s: Received %u/%u (%.2f%%) - Lost %u", name, stats->iso_rx_packets_received,
		total_packets, (float)stats->iso_rx_packets_received * 100 / total_packets,
		stats->iso_rx_packets_lost);
}

/* Separate function for assessing link quality using a test pattern sent from gateway */
static void ble_test_pattern_receive(uint8_t const *const p_data, size_t data_size, bool bad_frame)
{
	uint32_t total_packets;
	static uint8_t expected_packet_value;

	stats_overall.iso_rx_packets_received++;

	if (bad_frame) {
		LOG_WRN("Received bad frame");
	}

	total_packets = stats_overall.iso_rx_packets_received + stats_overall.iso_rx_packets_lost;

	/* Only check first value in packet */
	if (p_data[0] == expected_packet_value) {
		expected_packet_value++;
	} else {
		/* First packet will always be a mismatch
		 * if gateway hasn't been reset before connection
		 */
		if (stats_overall.iso_rx_packets_received != 1) {
			stats_overall.iso_rx_packets_lost++;
			LOG_WRN("Missing packet: value: %d, expected: %d", p_data[0],
				expected_packet_value);
		}

		expected_packet_value = p_data[0] + 1;
	}

	if ((total_packets % 100) == 0) {
		stats_print("Overall ", &stats_overall);
	}
}
#endif /* (CONFIG_BLE_ISO_TEST_PATTERN) */

/* Callback for handling BLE RX */
static void le_audio_rx_data_handler(uint8_t const *const p_data, size_t data_size, bool bad_frame,
				     uint32_t sdu_ref, enum audio_channel channel_index)
{
	/* Capture timestamp of when audio frame is received */
	uint32_t recv_frame_ts = audio_sync_timer_curr_time_get();

	/* Since the audio datapath thread is preemptive, no actions on the
	 * FIFO can happen whilst in this handler.
	 */

	if (strm_state != STATE_STREAMING) {
		/* Throw away data */
		LOG_DBG("Not in streaming state, throwing data: %d", strm_state);
		return;
	}

	int ret;
	struct ble_iso_data *iso_received = NULL;

#if (CONFIG_AUDIO_DEV == GATEWAY)
	switch (channel_index) {
	case AUDIO_CH_L:
		/* Proceed */
		break;
	case AUDIO_CH_R:
		static uint32_t packet_count_r;

		packet_count_r++;
		if ((packet_count_r % 1000) == 0) {
			LOG_DBG("Packets received from right channel: %d", packet_count_r);
		}

		return;
	default:
		LOG_ERR("Channel index not supported");
		return;
	}
#endif /* (CONFIG_AUDIO_DEV == GATEWAY) */

#if (CONFIG_BLE_ISO_TEST_PATTERN)
	ble_test_pattern_receive(p_data, data_size, bad_frame);
	return;
#endif /* (CONFIG_BLE_ISO_TEST_PATTERN) */

	uint32_t blocks_alloced_num, blocks_locked_num;

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

		data_fifo_block_free(&ble_fifo_rx, &stale_data);
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

/* Thread to receive data from BLE through a k_fifo and send to audio datapath */
static void audio_datapath_thread(void *dummy1, void *dummy2, void *dummy3)
{
	int ret;
	struct ble_iso_data *iso_received = NULL;
	size_t iso_received_size;

	while (1) {
		ret = data_fifo_pointer_last_filled_get(&ble_fifo_rx, (void *)&iso_received,
							&iso_received_size, K_FOREVER);
		ERR_CHK(ret);

#if ((CONFIG_AUDIO_DEV == GATEWAY) && (CONFIG_AUDIO_SOURCE_USB))
		ret = audio_decode(iso_received->data, iso_received->data_size,
				   iso_received->bad_frame);
		ERR_CHK(ret);
#else
		audio_datapath_stream_out(iso_received->data, iso_received->data_size,
					  iso_received->sdu_ref, iso_received->bad_frame,
					  iso_received->recv_frame_ts);
#endif
		data_fifo_block_free(&ble_fifo_rx, (void *)&iso_received);

		STACK_USAGE_PRINT("audio_datapath_thread", &audio_datapath_thread_data);
	}
}

/* Function for handling all stream state changes */
static void stream_state_set(enum stream_state stream_state_new)
{
	strm_state = stream_state_new;
}

uint8_t stream_state_get(void)
{
	return strm_state;
}

void streamctrl_encoded_data_send(void const *const data, size_t size, uint8_t num_ch)
{
	int ret;
	static int prev_ret;

	struct encoded_audio enc_audio = { .data = data, .size = size, .num_ch = num_ch };

	if (strm_state == STATE_STREAMING) {
		ret = le_audio_send(enc_audio);

		if (ret != 0 && ret != prev_ret) {
			LOG_WRN("Problem with sending LE audio data, ret: %d", ret);
		}
		prev_ret = ret;
	}
}

#if (CONFIG_AUDIO_TEST_TONE)
#define TEST_TONE_BASE_FREQ_HZ 1000

static int test_tone_button_press(void)
{
	int ret;
	static uint32_t test_tone_hz;

	if (CONFIG_AUDIO_BIT_DEPTH_BITS != 16) {
		LOG_WRN("Tone gen only supports 16 bits");
		return -ECANCELED;
	}

	if (strm_state == STATE_STREAMING) {
		if (test_tone_hz == 0) {
			test_tone_hz = TEST_TONE_BASE_FREQ_HZ;
		} else if (test_tone_hz >= TEST_TONE_BASE_FREQ_HZ * 4) {
			test_tone_hz = 0;
		} else {
			test_tone_hz = test_tone_hz * 2;
		}

		if (test_tone_hz != 0) {
			LOG_INF("Test tone set at %d Hz", test_tone_hz);
		} else {
			LOG_INF("Test tone off");
		}

		ret = audio_encode_test_tone_set(test_tone_hz);
		ERR_CHK_MSG(ret, "Failed to generate test tone");
	}

	return 0;
}
#endif /* (CONFIG_AUDIO_TEST_TONE) */

/* Handle button activity events */
static void button_evt_handler(struct button_evt event)
{
	int ret;

	LOG_DBG("Got btn evt from queue - id = %d, action = %d", event.button_pin,
		event.button_action);

	if (event.button_action != BUTTON_PRESS) {
		LOG_WRN("Unhandled button action");
		return;
	}

	switch (event.button_pin) {
	case BUTTON_PLAY_PAUSE:
		if (IS_ENABLED(CONFIG_WALKIE_TALKIE_DEMO)) {
			LOG_DBG("Play/pause not supported in walkie-talkie mode");
			return;
		}
		/* Starts/pauses the audio stream */
		ret = le_audio_play_pause();
		if (ret) {
			LOG_WRN("Could not play/pause");
		}

		break;

	case BUTTON_VOLUME_UP:
		ret = le_audio_volume_up();
		if (ret) {
			LOG_WRN("Failed to increase volume");
		}

		break;

	case BUTTON_VOLUME_DOWN:
		ret = le_audio_volume_down();
		if (ret) {
			LOG_WRN("Failed to decrease volume");
		}

		break;

	case BUTTON_4:
#if (CONFIG_AUDIO_TEST_TONE)
		if (IS_ENABLED(CONFIG_WALKIE_TALKIE_DEMO)) {
			LOG_DBG("Test tone is disabled in walkie-talkie mode");
			break;
		}

		ret = test_tone_button_press();
#else
		ret = le_audio_user_defined_button_press(LE_AUDIO_USER_DEFINED_ACTION_1);
#endif /*CONFIG_AUDIO_TEST_TONE*/

		if (ret) {
			LOG_WRN("Failed button 4 press, ret: %d", ret);
		}

		break;

	case BUTTON_5:
#if (CONFIG_AUDIO_MUTE)
		ret = le_audio_volume_mute();
		if (ret) {
			LOG_WRN("Failed to mute volume");
		}
#else
		ret = le_audio_user_defined_button_press(LE_AUDIO_USER_DEFINED_ACTION_2);
		if (ret) {
			LOG_WRN("User defined button 5 action failed, ret: %d", ret);
		}
#endif
		break;

	default:
		LOG_WRN("Unexpected/unhandled button id: %d", event.button_pin);
	}
}

/* Handle Bluetooth LE audio events */
static void le_audio_evt_handler(enum le_audio_evt_type event)
{
	int ret;

	LOG_DBG("Received event = %d, current state = %d", event, strm_state);
	switch (event) {
	case LE_AUDIO_EVT_STREAMING:
		LOG_DBG("LE audio evt streaming");

		if (strm_state == STATE_STREAMING) {
			LOG_DBG("Got streaming event in streaming state");
			break;
		}

		audio_system_start();
		stream_state_set(STATE_STREAMING);
		ret = led_blink(LED_APP_1_BLUE);
		ERR_CHK(ret);

		break;

	case LE_AUDIO_EVT_NOT_STREAMING:
		LOG_DBG("LE audio evt not_streaming");

		if (strm_state == STATE_PAUSED) {
			LOG_DBG("Got not_streaming event in paused state");
			break;
		}

		stream_state_set(STATE_PAUSED);
		audio_system_stop();
		ret = led_on(LED_APP_1_BLUE);
		ERR_CHK(ret);

		break;

	case LE_AUDIO_EVT_CONFIG_RECEIVED:
		LOG_DBG("Config received");

		uint32_t bitrate;
		uint32_t sampling_rate;

		ret = le_audio_config_get(&bitrate, &sampling_rate);
		if (ret) {
			LOG_WRN("Failed to get config");
			break;
		}

		LOG_DBG("Sampling rate: %d Hz", sampling_rate);
		LOG_DBG("Bitrate: %d kbps", bitrate);
		break;

	default:
		LOG_WRN("Unexpected/unhandled event: %d", event);
		break;
	}
}

void streamctrl_event_handler(void)
{
	struct event_t my_event;

	/* As long as this timeout is K_FOREVER, ctrl_events_get should
	 * never return unless it has an event, that is why we can ignore the
	 * return value
	 */
	(void)ctrl_events_get(&my_event, K_FOREVER);

	switch (my_event.event_source) {
	case EVT_SRC_BUTTON:
		button_evt_handler(my_event.button_activity);
		break;

	case EVT_SRC_LE_AUDIO:
		le_audio_evt_handler(my_event.le_audio_activity.le_audio_evt_type);
		break;

	default:
		LOG_WRN("Unhandled event from queue - source = %d", my_event.event_source);
	}
}

int streamctrl_start(void)
{
	int ret;

	ret = data_fifo_init(&ble_fifo_rx);
	ERR_CHK_MSG(ret, "Failed to set up ble_rx FIFO");

	audio_datapath_thread_id =
		k_thread_create(&audio_datapath_thread_data, audio_datapath_thread_stack,
				CONFIG_AUDIO_DATAPATH_STACK_SIZE,
				(k_thread_entry_t)audio_datapath_thread, NULL, NULL, NULL,
				K_PRIO_PREEMPT(CONFIG_AUDIO_DATAPATH_THREAD_PRIO), 0, K_NO_WAIT);
	ret = k_thread_name_set(audio_datapath_thread_id, "AUDIO DATAPATH");
	ERR_CHK(ret);

	ret = le_audio_enable(le_audio_rx_data_handler);
	ERR_CHK_MSG(ret, "Failed to enable LE Audio");

	return 0;
}
