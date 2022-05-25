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
#include "audio_codec.h"
#include "button_handler.h"
#include "data_fifo.h"
#include "board.h"
#include "hw_codec.h"
#include "ble_trans.h"
#include "ble_acl_common.h"
#include "ble_core.h"
#include "ble_audio_services.h"
#include "audio_datapath.h"
#include "audio_sync_timer.h"
#include "audio_usb.h"
#include "ble_hci_vsc.h"
#include "channel_assignment.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(streamctrl, CONFIG_LOG_STREAMCTRL_LEVEL);

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

static enum stream_state m_stream_state;

#if (CONFIG_BLE_ISO_TEST_PATTERN)

struct iso_recv_stats {
	uint32_t iso_rx_packets_received;
	uint32_t iso_rx_packets_lost;
};

static struct iso_recv_stats stats_overall;

static void stats_print(char const *const name, struct iso_recv_stats const *const stats)
{
	uint32_t total_packets;

	total_packets = stats->iso_rx_packets_received + stats->iso_rx_packets_lost;

	LOG_INF("%s: Received %u/%u (%.2f%%) - Lost %u", name, stats->iso_rx_packets_received,
		total_packets, (float)stats->iso_rx_packets_received * 100 / total_packets,
		stats->iso_rx_packets_lost);
}

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

#define TEST_TONE_BASE_FREQ_HZ 1000

#if ((CONFIG_AUDIO_DEV == HEADSET) || CONFIG_TRANSPORT_CIS)
static void ble_iso_rx_data_handler(uint8_t const *const p_data, size_t data_size, bool bad_frame,
				    uint32_t sdu_ref)
{
	/* Capture timestamp of when audio frame is received */
	uint32_t recv_frame_ts = audio_sync_timer_curr_time_get();

	/* Since the audio datapath thread is preemptive, no actions on the
	 * FIFO can happen whilst in this handler.
	 */

	if (m_stream_state != STATE_STREAMING) {
		/* Throw away data */
		LOG_DBG("Not in streaming state, throwing data: %d", m_stream_state);
		return;
	}

	int ret;
	struct ble_iso_data *iso_received = NULL;

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

		ret = data_fifo_block_free(&ble_fifo_rx, &stale_data);
		ERR_CHK(ret);
	}

	ret = data_fifo_pointer_first_vacant_get(&ble_fifo_rx, (void *)&iso_received, K_NO_WAIT);
	ERR_CHK_MSG(ret, "Unable to get FIFO pointer");

	if (data_size > ARRAY_SIZE(iso_received->data)) {
		ERR_CHK_MSG(-ENOMEM, "Data size too large for buffer");
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
#endif /* ((CONFIG_AUDIO_DEV == HEADSET) || CONFIG_TRANSPORT_CIS) */

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

#if (CONFIG_STREAM_BIDIRECTIONAL)
#if ((CONFIG_AUDIO_DEV == GATEWAY) && (CONFIG_AUDIO_SOURCE_USB))
		ret = audio_decode(iso_received->data, iso_received->data_size,
				   iso_received->bad_frame);
		ERR_CHK(ret);
#else
		audio_datapath_stream_out(iso_received->data, iso_received->data_size,
					  iso_received->sdu_ref, iso_received->bad_frame,
					  iso_received->recv_frame_ts);
#endif /* ((CONFIG_AUDIO_DEV == GATEWAY) && (CONFIG_AUDIO_SOURCE_USB)) */
#else
#if (CONFIG_AUDIO_DEV == HEADSET)
		audio_datapath_stream_out(iso_received->data, iso_received->data_size,
					  iso_received->sdu_ref, iso_received->bad_frame,
					  iso_received->recv_frame_ts);
#endif /* (CONFIG_AUDIO_DEV == HEADSET) */
#endif /* (CONFIG_STREAM_BIDIRECTIONAL) */

		ret = data_fifo_block_free(&ble_fifo_rx, (void *)&iso_received);
		ERR_CHK(ret);

		STACK_USAGE_PRINT("audio_datapath_thread", &audio_datapath_thread_data);
	}
}

/* Function for handling all stream state changes */
static void stream_state_set(enum stream_state stream_state_new)
{
	m_stream_state = stream_state_new;
}

uint8_t stream_state_get(void)
{
	return m_stream_state;
}

void streamctrl_encoded_data_send(void const *const data, size_t len)
{
	int ret;
	static int prev_ret;

	if (m_stream_state == STATE_STREAMING) {
#if (CONFIG_AUDIO_DEV == GATEWAY)
#if (CONFIG_AUDIO_SOURCE_I2S && !CONFIG_STREAM_BIDIRECTIONAL)
		uint32_t sdu_ref = 0;

		ret = ble_trans_iso_tx_anchor_get(BLE_TRANS_CHANNEL_STEREO, &sdu_ref, NULL);
		if (ret == -EIO) {
			/* Streaming has not yet started */
		} else if (ret) {
			LOG_WRN("Error getting ISO TX anchor point: %d", ret);
		}

		audio_datapath_sdu_ref_update(sdu_ref);
#endif /* (CONFIG_AUDIO_SOURCE_I2S && !CONFIG_STREAM_BIDIRECTIONAL) */
		ret = ble_trans_iso_tx(data, len, BLE_TRANS_CHANNEL_STEREO);
#elif (CONFIG_AUDIO_DEV == HEADSET)
		ret = ble_trans_iso_tx(data, len, BLE_TRANS_CHANNEL_RETURN_MONO);
#endif /* (CONFIG_AUDIO_DEV == GATEWAY) */

		if (ret != 0 && ret != prev_ret) {
			LOG_WRN("Problem with sending BLE data, ret: %d", ret);
		}
		prev_ret = ret;
	} else {
		/* Not currently streaming, throwing away data */
	}
}

/**** Initialization and setup ****/

/* Initialize the BLE transport */
static int m_ble_transport_init(void)
{
	int ret;

	ret = ble_trans_iso_lost_notify_enable();
	if (ret) {
		return ret;
	}

	/* Setting TX power for advertising if config is set to anything other than 0 */
	if (CONFIG_BLE_ADV_TX_POWER_DBM) {
		ret = ble_hci_vsc_set_adv_tx_pwr(CONFIG_BLE_ADV_TX_POWER_DBM);
		ERR_CHK(ret);
	}

#if (CONFIG_TRANSPORT_BIS)
#if (CONFIG_AUDIO_DEV == GATEWAY)
	ret = ble_trans_iso_init(TRANS_TYPE_BIS, DIR_TX, NULL);
	if (ret) {
		return ret;
	}
	ret = ble_trans_iso_start();
	if (ret) {
		return ret;
	}
#elif (CONFIG_AUDIO_DEV == HEADSET)
	ret = ble_trans_iso_init(TRANS_TYPE_BIS, DIR_RX, ble_iso_rx_data_handler);
	if (ret) {
		return ret;
	}
#endif /* (CONFIG_AUDIO_DEV == GATEWAY) */
#elif (CONFIG_TRANSPORT_CIS)

#if (!CONFIG_BLE_LE_POWER_CONTROL_ENABLED)
	ret = ble_core_le_pwr_ctrl_disable();
	if (ret) {
		return ret;
	}
#endif /* (!CONFIG_BLE_LE_POWER_CONTROL_ENABLED) */
	ret = ble_acl_common_init();
	if (ret) {
		return ret;
	}
	ble_acl_common_start();
#if (CONFIG_STREAM_BIDIRECTIONAL)
#if (CONFIG_AUDIO_DEV == GATEWAY)
	ret = ble_trans_iso_init(TRANS_TYPE_CIS, DIR_BIDIR, ble_iso_rx_data_handler);
#elif (CONFIG_AUDIO_DEV == HEADSET)
	enum audio_channel channel;

	ret = channel_assignment_get(&channel);
	if (ret) {
		return ret;
	}

	if (channel == AUDIO_CHANNEL_LEFT) {
		ret = ble_trans_iso_init(TRANS_TYPE_CIS, DIR_BIDIR, ble_iso_rx_data_handler);
	} else {
		ret = ble_trans_iso_init(TRANS_TYPE_CIS, DIR_RX, ble_iso_rx_data_handler);
	}
#endif /* (CONFIG_AUDIO_DEV == GATEWAY) */
#else
#if (CONFIG_AUDIO_DEV == GATEWAY)
	ret = ble_trans_iso_init(TRANS_TYPE_CIS, DIR_TX, ble_iso_rx_data_handler);
#elif (CONFIG_AUDIO_DEV == HEADSET)
	ret = ble_trans_iso_init(TRANS_TYPE_CIS, DIR_RX, ble_iso_rx_data_handler);
#endif /* (CONFIG_AUDIO_DEV == GATEWAY) */
#endif /* (CONFIG_STREAM_BIDIRECTIONAL) */
	if (ret) {
		return ret;
	}
#if (CONFIG_AUDIO_DEV == GATEWAY)
	ret = ble_trans_iso_cig_create();
	if (ret) {
		return ret;
	}
#endif /* (CONFIG_AUDIO_DEV == GATEWAY) */
#endif /* (CONFIG_TRANSPORT_BIS) */
	return 0;
}

/**** Event handlers ****/
/* Handle button activity events */
static void m_button_evt_handler(struct button_evt event)
{
	int ret;
	static uint32_t test_tone_hz;

	LOG_DBG("Got btn evt from queue - id = %d, action = %d", event.button_pin,
		event.button_action);

	if (event.button_action != BUTTON_PRESS) {
		LOG_WRN("Unhandled button action");
		return;
	}

	switch (event.button_pin) {
	case BUTTON_PLAY_PAUSE:
		if (CONFIG_AUDIO_DEV == HEADSET) {
			/* Starts/pauses the audio stream */
			switch (m_stream_state) {
			case STATE_DISCONNECTED:
				ret = ble_trans_iso_start();
				if (ret) {
					LOG_WRN("Unable to start iso, disconnected");
					break;
				}

				break;

			case STATE_LINK_READY:
				ret = ble_trans_iso_start();
				if (ret) {
					LOG_WRN("Unable to start iso, ready-state");
					break;
				}

				LOG_DBG("Starting headset device");
				stream_state_set(STATE_STREAMING);
				ret = led_blink(LED_APP_1_BLUE);
				ERR_CHK(ret);
				break;

			case STATE_PAUSED:
				LOG_INF("Playing stream");
				ret = ble_trans_iso_start();
				if (ret) {
					LOG_WRN("Got disconnected while trying to play again");
					break;
				}

#if (CONFIG_TRANSPORT_CIS)
				ret = ble_vcs_volume_unmute();
				if (ret) {
					LOG_WRN("Failed to unmute volume using VCS, ret = %d", ret);
				}
#endif /* (CONFIG_TRANSPORT_CIS) */
				stream_state_set(STATE_STREAMING);
				ret = led_blink(LED_APP_1_BLUE);
				ERR_CHK(ret);
				break;

			case STATE_STREAMING:
				LOG_INF("Pausing stream");
				stream_state_set(STATE_PAUSED);
				ret = led_on(LED_APP_1_BLUE);
				ERR_CHK(ret);
				ble_trans_iso_stop();
				break;

			default:
				/* We got the play/pause button in a state where we
				 * can not use it. This should be expected, buttons
				 * can be pressed at any time. For now, inform about it.
				 */
				LOG_DBG("Got play/pause in state %d", m_stream_state);
			}
		} else if (CONFIG_AUDIO_DEV == GATEWAY) {
#if (CONFIG_TRANSPORT_BIS)
			switch (m_stream_state) {
			case STATE_DISCONNECTED:
				audio_gateway_start();
				ret = ble_trans_iso_start();
				if (ret) {
					LOG_WRN("ble_trans_iso_start failed %d", ret);
				}
				stream_state_set(STATE_STREAMING);
				ret = led_blink(LED_APP_1_BLUE);
				ERR_CHK(ret);
				break;

			case STATE_STREAMING:
				audio_stop();
				ret = ble_trans_iso_stop();
				if (ret) {
					LOG_WRN("ble_trans_iso_stop failed, %d", ret);
				}
				stream_state_set(STATE_DISCONNECTED);
				ret = led_off(LED_APP_1_BLUE);
				ERR_CHK(ret);
				break;

			default:
				/* We got the play/pause button in a state where we
				 * can not use it. This should be expected, buttons
				 * can be pressed at any time. For now, inform about it.
				 */
				LOG_DBG("Gateway got play/pause in state %d", m_stream_state);
			}
#else
			LOG_WRN("Button is not used on CIS gateway");
#endif /* (CONFIG_TRANSPORT_BIS) */
		} else {
			LOG_ERR("Wrong device setting, should not reach here");
		}
		break;

	case BUTTON_VOLUME_UP:
		switch (m_stream_state) {
		case STATE_STREAMING:
#if (CONFIG_TRANSPORT_CIS)
			LOG_DBG("Button pressed for volume up");
			ret = ble_vcs_volume_up();
			if (ret) {
				LOG_WRN("Failed to increase volume, ret = %d", ret);
			}
#else
			if (CONFIG_AUDIO_DEV == HEADSET) {
				LOG_DBG("Button pressed for volume up");
				ret = hw_codec_volume_increase();
				ERR_CHK_MSG(ret, "Failed to increase volume");
			} else {
				LOG_WRN("Button is not used on gateway");
			}

#endif /* (CONFIG_TRANSPORT_CIS) */
			break;

		default:
			LOG_WRN("Can only change volume while streaming");
			break;
		}
		break;

	case BUTTON_VOLUME_DOWN:
		switch (m_stream_state) {
		case STATE_STREAMING:
#if (CONFIG_TRANSPORT_CIS)
			LOG_DBG("Button pressed for volume down");
			ret = ble_vcs_volume_down();
			if (ret) {
				LOG_WRN("Failed to decrease volume, ret = %d", ret);
			}
#else
			if (CONFIG_AUDIO_DEV == HEADSET) {
				LOG_DBG("Button pressed for volume down");
				ret = hw_codec_volume_decrease();
				ERR_CHK_MSG(ret, "Failed to decrease volume");
			} else {
				LOG_WRN("Button is not used on gateway");
			}

#endif /* (CONFIG_TRANSPORT_CIS) */
			break;
		default:
			LOG_WRN("Can only change volume while streaming");
			break;
		}
		break;

	case BUTTON_MUTE:
		switch (m_stream_state) {
		case STATE_STREAMING:
#if (CONFIG_TRANSPORT_CIS)
			LOG_DBG("Button pressed for mute");
			ret = ble_vcs_volume_mute();
			if (ret) {
				LOG_WRN("Failed to mute volume, ret = %d", ret);
			}
#else
			if (CONFIG_AUDIO_DEV == HEADSET) {
				LOG_DBG("Button pressed for mute");
				ret = hw_codec_volume_mute();
				ERR_CHK_MSG(ret, "Failed to mute volume");
			} else {
				LOG_WRN("Button is not used on gateway");
			}
#endif /* (CONFIG_TRANSPORT_CIS) */
			break;

		default:
			LOG_WRN("Can only mute volume while streaming");
			break;
		}
		break;

	case BUTTON_TEST_TONE:
		switch (m_stream_state) {
		case STATE_STREAMING:
			if (CONFIG_AUDIO_BIT_DEPTH_BITS != 16) {
				LOG_WRN("Tone gen only supports 16 bits");
				break;
			}

			if (test_tone_hz == 0) {
				test_tone_hz = TEST_TONE_BASE_FREQ_HZ;
			} else if (test_tone_hz >= TEST_TONE_BASE_FREQ_HZ * 4) {
				test_tone_hz = 0;
			} else {
				test_tone_hz = test_tone_hz * 2;
			}

			LOG_INF("Test tone set at %d Hz", test_tone_hz);

			ret = audio_encode_test_tone_set(test_tone_hz);
			ERR_CHK_MSG(ret, "Failed to generate test tone");
			break;

		default:
			LOG_WRN("Test tone can only be set in streaming mode");
			break;
		}
		break;

	default:
		LOG_WRN("Unexpected/unhandled button id: %d", event.button_pin);
	}
}

/* Handle data transport/stream events */
static void m_ble_transport_evt_handler(enum ble_evt_type event)
{
	int ret;

	LOG_DBG("Received event = %d, current state = %d", event, m_stream_state);
	switch (event) {
	case BLE_EVT_CONNECTED:
		LOG_INF("BLE evt connected");

		switch (m_stream_state) {
		case STATE_CONNECTING:
			/* Fall through */
		case STATE_DISCONNECTED:
			stream_state_set(STATE_CONNECTED);
			break;
		case STATE_STREAMING:
			break;
		case STATE_PAUSED:
			break;

		default:
			LOG_ERR("Got connected in state %d", m_stream_state);
		}
		break;

	case BLE_EVT_DISCONNECTED:
		LOG_INF("BLE evt disconnected");
		ret = led_off(LED_APP_1_BLUE);
		ERR_CHK(ret);
		/* Disconnect can happen in most states */
		switch (m_stream_state) {
		case STATE_CONNECTED:
			/* Fall through */
		case STATE_LINK_READY:
			stream_state_set(STATE_DISCONNECTED);
#if ((CONFIG_AUDIO_DEV == HEADSET) && CONFIG_TRANSPORT_BIS)
			ble_trans_iso_bis_rx_sync_get();
#endif /* ((CONFIG_AUDIO_DEV == HEADSET) && CONFIG_TRANSPORT_BIS) */
			break;

		case STATE_STREAMING:
#if (CONFIG_AUDIO_DEV == HEADSET)
			LOG_INF("Trying to reconnect");
			ret = ble_trans_iso_start();
			if (ret) {
				LOG_WRN("Unable to reconnect, please try again, ret = %d", ret);
#if (CONFIG_TRANSPORT_BIS)
				struct event_t event;

				event.event_source = EVT_SRC_PEER;
				event.link_activity = BLE_EVT_DISCONNECTED;
				ret = ctrl_events_put(&event);
				ERR_CHK_MSG(
					ret,
					"Unable to put event BLE_EVT_DISCONNECTED in event queue");
#endif /* (CONFIG_TRANSPORT_BIS) */
			}
#endif /* (CONFIG_AUDIO_DEV == HEADSET) */
#if ((CONFIG_AUDIO_DEV == GATEWAY) && CONFIG_TRANSPORT_CIS)
			stream_state_set(STATE_DISCONNECTED);
			audio_stop();
#endif /* ((CONFIG_AUDIO_DEV == GATEWAY) && CONFIG_TRANSPORT_CIS) */
			break;

		case STATE_PAUSED:
			stream_state_set(STATE_DISCONNECTED);
#if ((CONFIG_AUDIO_DEV == HEADSET) && CONFIG_TRANSPORT_BIS)
			ble_trans_iso_bis_rx_sync_get();
#endif /* ((CONFIG_AUDIO_DEV == HEADSET) && CONFIG_TRANSPORT_BIS) */
			break;

		/* Disconnect should not happen in these states: */
		case STATE_CONNECTING:
			/* Fall through */
		case STATE_DISCONNECTED:
			LOG_ERR("Got disconnected in state %d", m_stream_state);
			break;

		default:
			/* If we get here, we have missed out on a state above */
			LOG_ERR("Got disconnected in state: %d", m_stream_state);
		}

		break;

	case BLE_EVT_LINK_READY:
		LOG_INF("BLE evt link ready");

		switch (m_stream_state) {
		case STATE_PAUSED:
			ret = led_on(LED_APP_1_BLUE);
			ERR_CHK(ret);
			break;

		case STATE_STREAMING:
#if (CONFIG_AUDIO_DEV == HEADSET)
			ret = led_blink(LED_APP_1_BLUE);
			ERR_CHK(ret);
#endif /* (CONFIG_AUDIO_DEV == HEADSET) */
			break;

		case STATE_CONNECTED:
#if (CONFIG_AUDIO_DEV == HEADSET)
			stream_state_set(STATE_LINK_READY);
			ret = led_on(LED_APP_1_BLUE);
			ERR_CHK(ret);
#elif (CONFIG_AUDIO_DEV == GATEWAY)
			audio_gateway_start();
			stream_state_set(STATE_STREAMING);
			ret = led_blink(LED_APP_1_BLUE);
			ERR_CHK(ret);
#endif /* (CONFIG_AUDIO_DEV == HEADSET) */
			break;

		default:
			LOG_ERR("Got link_ready in state %d", m_stream_state);
		}
		break;

	case BLE_EVT_STREAMING:
		LOG_INF("BLE evt streaming");
		switch (m_stream_state) {
		case STATE_LINK_READY:
#if (CONFIG_AUDIO_DEV == HEADSET)
			stream_state_set(STATE_STREAMING);
			ret = led_blink(LED_APP_1_BLUE);
			ERR_CHK(ret);
#endif /* (CONFIG_AUDIO_DEV == HEADSET) */
			break;
		case STATE_STREAMING:
			/* Fall through */
		case STATE_PAUSED:
			break;
		default:
			LOG_ERR("Got streaming in state %d", m_stream_state);
			break;
		}
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
		m_button_evt_handler(my_event.button_activity);
		break;

	case EVT_SRC_PEER:
		m_ble_transport_evt_handler(my_event.link_activity);
		break;

	default:
		LOG_WRN("Unhandled event from queue -  source = %d", my_event.event_source);
	}
}

/**** The top level ****/
/* Initialize what must be initialized, to be ready to handle events */
int streamctrl_start(void)
{
	int ret;

	ret = data_fifo_init(&ble_fifo_rx);
	ERR_CHK_MSG(ret, "Failed to set up ble_rx FIFO");

#if (CONFIG_AUDIO_DEV == HEADSET)
	audio_headset_start();
#endif

	audio_datapath_thread_id =
		k_thread_create(&audio_datapath_thread_data, audio_datapath_thread_stack,
				CONFIG_AUDIO_DATAPATH_STACK_SIZE,
				(k_thread_entry_t)audio_datapath_thread, NULL, NULL, NULL,
				K_PRIO_PREEMPT(CONFIG_AUDIO_DATAPATH_THREAD_PRIO), 0, K_NO_WAIT);
	ret = k_thread_name_set(audio_datapath_thread_id, "AUDIO DATAPATH");
	ERR_CHK(ret);

	ret = m_ble_transport_init();
	ERR_CHK(ret);

	stream_state_set(STATE_CONNECTING);

	return 0;
}
