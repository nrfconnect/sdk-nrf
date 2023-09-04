/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "streamctrl.h"

#include <stddef.h>
#include <string.h>
#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <zephyr/debug/stack.h>
#include <zephyr/zbus/zbus.h>

#include "nrf5340_audio_common.h"
#include "led.h"
#include "button_assignments.h"
#include "macros_common.h"
#include "audio_system.h"
#include "button_handler.h"
#include "data_fifo.h"
#include "le_audio.h"
#include "bt_mgmt.h"
#include "bt_rend.h"
#include "audio_datapath.h"
#include "bt_content_ctrl.h"

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

ZBUS_SUBSCRIBER_DEFINE(button_evt_sub, CONFIG_BUTTON_MSG_SUB_QUEUE_SIZE);
ZBUS_SUBSCRIBER_DEFINE(le_audio_evt_sub, CONFIG_LE_AUDIO_MSG_SUB_QUEUE_SIZE);
ZBUS_SUBSCRIBER_DEFINE(content_control_evt_sub, CONFIG_CONTENT_CONTROL_MSG_SUB_QUEUE_SIZE);

static struct k_thread audio_datapath_thread_data;
static struct k_thread button_msg_sub_thread_data;
static struct k_thread le_audio_msg_sub_thread_data;
static struct k_thread content_control_msg_sub_thread_data;

static k_tid_t audio_datapath_thread_id;
static k_tid_t button_msg_sub_thread_id;
static k_tid_t le_audio_msg_sub_thread_id;
static k_tid_t content_control_msg_sub_thread_id;

K_THREAD_STACK_DEFINE(audio_datapath_thread_stack, CONFIG_AUDIO_DATAPATH_STACK_SIZE);
K_THREAD_STACK_DEFINE(button_msg_sub_thread_stack, CONFIG_BUTTON_MSG_SUB_STACK_SIZE);
K_THREAD_STACK_DEFINE(le_audio_msg_sub_thread_stack, CONFIG_LE_AUDIO_MSG_SUB_STACK_SIZE);
K_THREAD_STACK_DEFINE(content_control_msg_sub_thread_stack,
		      CONFIG_CONTENT_CONTROL_MSG_SUB_STACK_SIZE);

static enum stream_state strm_state = STATE_PAUSED;

struct rx_stats {
	uint32_t recv_cnt;
	uint32_t bad_frame_cnt;
	uint32_t data_size_mismatch_cnt;
};

/* Callback for handling BLE RX */
static void le_audio_rx_data_handler(uint8_t const *const p_data, size_t data_size, bool bad_frame,
				     uint32_t sdu_ref, enum audio_channel channel_index,
				     size_t desired_data_size)
{
	/* Capture timestamp of when audio frame is received */
	uint32_t recv_frame_ts = nrfx_timer_capture(&audio_sync_timer_instance,
						    AUDIO_SYNC_TIMER_CURR_TIME_CAPTURE_CHANNEL);

	/* Since the audio datapath thread is preemptive, no actions on the
	 * FIFO can happen whilst in this handler.
	 */

	bool data_size_mismatch = false;
	static struct rx_stats rx_stats[AUDIO_CH_NUM];

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
		return;
	}

	if (strm_state != STATE_STREAMING) {
		/* Throw away data */
		LOG_DBG("Not in streaming state, throwing data: %d", strm_state);
		return;
	}

	int ret;
	struct ble_iso_data *iso_received = NULL;

#if (CONFIG_AUDIO_DEV == GATEWAY)
	if (channel_index != AUDIO_CH_L) {
		/* Only left channel RX data in use on gateway */
		return;
	}
#endif /* (CONFIG_AUDIO_DEV == GATEWAY) */

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
#endif /* ((CONFIG_AUDIO_DEV == GATEWAY) && (CONFIG_AUDIO_SOURCE_USB)) */
		data_fifo_block_free(&ble_fifo_rx, (void *)iso_received);

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

	struct encoded_audio enc_audio = {.data = data, .size = size, .num_ch = num_ch};

	if (strm_state == STATE_STREAMING) {
		ret = le_audio_send(enc_audio);

		if (ret != 0 && ret != prev_ret) {
			if (ret == -ECANCELED) {
				LOG_WRN("Sending operation cancelled");
			} else {
				LOG_WRN("Problem with sending LE audio data, ret: %d", ret);
			}
		}

		prev_ret = ret;
	}
}

#define TEST_TONE_BASE_FREQ_HZ 1000

static int test_tone_start(void)
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

/* Handle button activity */
static void button_msg_sub_thread(void)
{
	int ret;
	const struct zbus_channel *chan;
	bool broadcast_alt = true;

	while (1) {
		ret = zbus_sub_wait(&button_evt_sub, &chan, K_FOREVER);
		ERR_CHK(ret);

		struct button_msg msg;

		ret = zbus_chan_read(chan, &msg, ZBUS_READ_TIMEOUT_MS);
		ERR_CHK(ret);

		LOG_DBG("Got btn evt from queue - id = %d, action = %d", msg.button_pin,
			msg.button_action);

		if (msg.button_action != BUTTON_PRESS) {
			LOG_WRN("Unhandled button action");
			return;
		}

		switch (msg.button_pin) {
		case BUTTON_PLAY_PAUSE:
			if (IS_ENABLED(CONFIG_WALKIE_TALKIE_DEMO)) {
				LOG_DBG("Play/pause not supported in walkie-talkie mode");
				return;
			}

			if (strm_state == STATE_STREAMING) {
				ret = bt_content_ctrl_stop(NULL);
				if (ret) {
					LOG_WRN("Could not stop: %d", ret);
				}
			} else if (strm_state == STATE_PAUSED) {
				ret = bt_content_ctrl_start(NULL);
				if (ret) {
					LOG_WRN("Could not start: %d", ret);
				}
			} else {
				LOG_WRN("In invalid state: %d", strm_state);
			}

			break;

		case BUTTON_VOLUME_UP:
			ret = bt_rend_volume_up();
			if (ret) {
				LOG_WRN("Failed to increase volume: %d", ret);
			}

			break;

		case BUTTON_VOLUME_DOWN:
			ret = bt_rend_volume_down();
			if (ret) {
				LOG_WRN("Failed to decrease volume: %d", ret);
			}

			break;

		case BUTTON_4:
			if (IS_ENABLED(CONFIG_AUDIO_TEST_TONE)) {
				if (IS_ENABLED(CONFIG_WALKIE_TALKIE_DEMO)) {
					LOG_DBG("Test tone is disabled in walkie-talkie mode");
					break;
				}

				ret = test_tone_start();
				if (ret) {
					LOG_WRN("Failed to play test tone, ret: %d", ret);
				}

				break;
			}

			ret = le_audio_user_defined_button_press(LE_AUDIO_USER_DEFINED_ACTION_1);
			if (ret) {
				LOG_WRN("Failed button 4 press, ret: %d", ret);
			}

			break;

		case BUTTON_5:
			if (IS_ENABLED(CONFIG_AUDIO_MUTE)) {
				ret = bt_rend_volume_mute(false);
				if (ret) {
					LOG_WRN("Failed to mute volume");
				}

				break;
			} else if (IS_ENABLED(CONFIG_BT_BAP_BROADCAST_SINK)) {
				/* Will eventually be handled by different applications */
				le_audio_disable();
				if (broadcast_alt) {
					ret = bt_mgmt_scan_start(
						0, 0, BT_MGMT_SCAN_TYPE_BROADCAST,
						CONFIG_BT_AUDIO_BROADCAST_NAME_ALT);
					broadcast_alt = false;
				} else {
					ret = bt_mgmt_scan_start(0, 0, BT_MGMT_SCAN_TYPE_BROADCAST,
								 CONFIG_BT_AUDIO_BROADCAST_NAME);
					broadcast_alt = true;
				}

				if (ret) {
					LOG_WRN("Failed to start scanning for broadcaster: %d",
						ret);
				}

				break;
			}

			break;

		default:
			LOG_WRN("Unexpected/unhandled button id: %d", msg.button_pin);
		}

		STACK_USAGE_PRINT("button_msg_thread", &button_msg_sub_thread_data);
	}
}

/* Handle Bluetooth LE audio events */
static void le_audio_msg_sub_thread(void)
{
	int ret;
	uint32_t pres_delay_us;
	uint32_t bitrate_bps;
	uint32_t sampling_rate_hz;
	const struct zbus_channel *chan;

	while (1) {
		ret = zbus_sub_wait(&le_audio_evt_sub, &chan, K_FOREVER);
		ERR_CHK(ret);

		struct le_audio_msg msg;

		ret = zbus_chan_read(chan, &msg, ZBUS_READ_TIMEOUT_MS);
		ERR_CHK(ret);

		uint8_t event = msg.event;

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

			ret = le_audio_config_get(&bitrate_bps, &sampling_rate_hz, NULL);
			if (ret) {
				LOG_WRN("Failed to get config: %d", ret);
				break;
			}

			LOG_DBG("Sampling rate: %d Hz", sampling_rate_hz);
			LOG_DBG("Bitrate: %d bps", bitrate_bps);

			break;

		case LE_AUDIO_EVT_PRES_DELAY_SET:
			LOG_DBG("Set presentation delay");

			ret = le_audio_config_get(NULL, NULL, &pres_delay_us);
			if (ret) {
				LOG_ERR("Failed to get config: %d", ret);
				break;
			}

			ret = audio_datapath_pres_delay_us_set(pres_delay_us);
			if (ret) {
				LOG_ERR("Failed to set presentation delay to %d", pres_delay_us);
				break;
			}

			LOG_INF("Presentation delay %d us is set by initiator", pres_delay_us);

			break;

		case LE_AUDIO_EVT_SYNC_LOST:
			LOG_INF("Sync lost");

			if (strm_state == STATE_STREAMING) {
				stream_state_set(STATE_PAUSED);
				audio_system_stop();
				ret = led_on(LED_APP_1_BLUE);
				ERR_CHK(ret);
			}

			if (IS_ENABLED(CONFIG_BT_OBSERVER)) {
				ret = bt_mgmt_scan_start(0, 0, BT_MGMT_SCAN_TYPE_BROADCAST, NULL);
				if (ret) {
					if (ret != -EALREADY) {
						LOG_ERR("Failed to restart scanning: %d", ret);
					}

					break;
				}

				/* NOTE: The string below is used by the Nordic CI system */
				LOG_INF("Restarted scanning for broadcaster");
			}

			break;

		case LE_AUDIO_EVT_NO_VALID_CFG:
			LOG_WRN("No valid configurations found, will disconnect");

			ret = bt_mgmt_conn_disconnect(msg.conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
			if (ret) {
				LOG_ERR("Failed to disconnect: %d", ret);
			}

			break;

		default:
			LOG_WRN("Unexpected/unhandled le_audio event: %d", event);

			break;
		}

		STACK_USAGE_PRINT("le_audio_msg_thread", &le_audio_msg_sub_thread_data);
	}
}

/* Handle content control events */
static void content_control_msg_sub_thread(void)
{
	int ret;

	const struct zbus_channel *chan;

	while (1) {
		ret = zbus_sub_wait(&content_control_evt_sub, &chan, K_FOREVER);
		ERR_CHK(ret);

		struct content_control_msg msg;

		ret = zbus_chan_read(chan, &msg, ZBUS_READ_TIMEOUT_MS);
		ERR_CHK(ret);

		uint8_t event = msg.event;

		LOG_DBG("Received event = %d, current state = %d", event, strm_state);

		switch (event) {
		case MEDIA_PLAY:
			ret = le_audio_play();
			if (ret && (ret != -EALREADY)) {
				LOG_ERR("Failed to play: %d", ret);
			}

			break;

		case MEDIA_STOP:
			ret = le_audio_pause();
			if (ret && (ret != -EALREADY)) {
				LOG_ERR("Failed to pause: %d", ret);
			}

			break;

		default:
			LOG_WRN("Unexpected/unhandled content control event: %d", event);

			break;
		}

		STACK_USAGE_PRINT("content_control_msg_thread",
				  &content_control_msg_sub_thread_data);
	}
}

/**
 * @brief	Zbus listener to receive events from bt_mgmt
 *
 * @note	Will in most cases be called from BT_RX context,
 *		so there should not be too much processing done here
 */
static void bt_mgmt_evt_handler(const struct zbus_channel *chan)
{
	int ret;
	const struct bt_mgmt_msg *msg;

	msg = zbus_chan_const_msg(chan);
	uint8_t event = msg->event;

	switch (event) {
	case BT_MGMT_CONNECTED:
		LOG_INF("Connected");

		break;

	case BT_MGMT_DISCONNECTED:
		LOG_INF("Disconnected");
		le_audio_conn_disconnected(msg->conn);

		ret = bt_content_ctrl_conn_disconnected(msg->conn);
		if (ret) {
			LOG_ERR("bt_content_ctrl_disconnected failed with %d", ret);
		}

		break;

	case BT_MGMT_EXT_ADV_READY:
		LOG_INF("Ext adv ready");
		if (IS_ENABLED(CONFIG_TRANSPORT_BIS)) {
			ret = le_audio_ext_adv_set(msg->ext_adv);
			if (ret) {
				LOG_WRN("Failed to set extended advertisement data");
			}
		}

		break;

	case BT_MGMT_SECURITY_CHANGED:
		LOG_INF("Security changed");

		if (CONFIG_AUDIO_DEV == GATEWAY) {
			le_audio_conn_set(msg->conn);
		}

		ret = bt_rend_discover(msg->conn);
		if (ret) {
			LOG_WRN("Failed to discover rendering services");
		}

		ret = bt_content_ctrl_discover(msg->conn);
		if (ret == -EALREADY) {
			LOG_DBG("Discovery in progress or already done");
		} else if (ret) {
			LOG_ERR("Failed to start discovery of content control: %d", ret);
		}

		break;

	case BT_MGMT_PA_SYNCED:
		LOG_INF("PA synced");

		ret = le_audio_pa_sync_set(msg->pa_sync, msg->broadcast_id);
		if (ret) {
			LOG_WRN("Failed to set PA sync");
		}

		break;

	case BT_MGMT_PA_SYNC_LOST:
		LOG_INF("PA sync lost");

		if (IS_ENABLED(CONFIG_BT_OBSERVER)) {
			ret = bt_mgmt_scan_start(0, 0, BT_MGMT_SCAN_TYPE_BROADCAST, NULL);
			if (ret) {
				if (ret == -EALREADY) {
					return;
				}

				LOG_ERR("Failed to restart scanning: %d", ret);
				break;
			}

			/* NOTE: The string below is used by the Nordic CI system */
			LOG_INF("Restarted scanning for broadcaster");
		}

		break;

	default:
		LOG_WRN("Unexpected/unhandled bt_mgmt event: %d", event);

		break;
	}
}

ZBUS_LISTENER_DEFINE(bt_mgmt_evt_sub, bt_mgmt_evt_handler);

int streamctrl_start(void)
{
	int ret;

	static bool started;

	if (started) {
		LOG_WRN("Streamctrl already started");
		return -EALREADY;
	}

	audio_system_init();

	ret = data_fifo_init(&ble_fifo_rx);
	ERR_CHK_MSG(ret, "Failed to set up ble_rx FIFO");

	button_msg_sub_thread_id = k_thread_create(
		&button_msg_sub_thread_data, button_msg_sub_thread_stack,
		CONFIG_BUTTON_MSG_SUB_STACK_SIZE, (k_thread_entry_t)button_msg_sub_thread, NULL,
		NULL, NULL, K_PRIO_PREEMPT(CONFIG_BUTTON_MSG_SUB_THREAD_PRIO), 0, K_NO_WAIT);
	ret = k_thread_name_set(button_msg_sub_thread_id, "BUTTON_MSG_SUB");
	ERR_CHK(ret);

	le_audio_msg_sub_thread_id = k_thread_create(
		&le_audio_msg_sub_thread_data, le_audio_msg_sub_thread_stack,
		CONFIG_LE_AUDIO_MSG_SUB_STACK_SIZE, (k_thread_entry_t)le_audio_msg_sub_thread, NULL,
		NULL, NULL, K_PRIO_PREEMPT(CONFIG_LE_AUDIO_MSG_SUB_THREAD_PRIO), 0, K_NO_WAIT);
	ret = k_thread_name_set(le_audio_msg_sub_thread_id, "LE_AUDIO_MSG_SUB");
	ERR_CHK(ret);

	content_control_msg_sub_thread_id = k_thread_create(
		&content_control_msg_sub_thread_data, content_control_msg_sub_thread_stack,
		CONFIG_CONTENT_CONTROL_MSG_SUB_STACK_SIZE,
		(k_thread_entry_t)content_control_msg_sub_thread, NULL, NULL, NULL,
		K_PRIO_PREEMPT(CONFIG_CONTENT_CONTROL_MSG_SUB_THREAD_PRIO), 0, K_NO_WAIT);
	ret = k_thread_name_set(content_control_msg_sub_thread_id, "CONTENT_CONTROL_MSG_SUB");
	ERR_CHK(ret);

	audio_datapath_thread_id = k_thread_create(
		&audio_datapath_thread_data, audio_datapath_thread_stack,
		CONFIG_AUDIO_DATAPATH_STACK_SIZE, (k_thread_entry_t)audio_datapath_thread, NULL,
		NULL, NULL, K_PRIO_PREEMPT(CONFIG_AUDIO_DATAPATH_THREAD_PRIO), 0, K_NO_WAIT);
	ret = k_thread_name_set(audio_datapath_thread_id, "AUDIO DATAPATH");
	ERR_CHK(ret);

	ret = le_audio_enable(le_audio_rx_data_handler, audio_datapath_sdu_ref_update);
	ERR_CHK_MSG(ret, "Failed to enable LE Audio");

	ret = bt_rend_init();
	ERR_CHK(ret);

	ret = bt_content_ctrl_init();
	ERR_CHK(ret);

	if ((CONFIG_AUDIO_DEV == HEADSET) && IS_ENABLED(CONFIG_TRANSPORT_CIS)) {
		size_t ext_adv_size = 0;
		const struct bt_data *ext_adv = NULL;

		le_audio_adv_get(&ext_adv, &ext_adv_size, false);

		ret = bt_mgmt_adv_start(ext_adv, ext_adv_size, NULL, 0, true);
		ERR_CHK(ret);
	} else if ((CONFIG_AUDIO_DEV == GATEWAY) && IS_ENABLED(CONFIG_TRANSPORT_BIS)) {
		size_t ext_adv_size = 0;
		size_t per_adv_size = 0;
		const struct bt_data *ext_adv = NULL;
		const struct bt_data *per_adv = NULL;

		le_audio_adv_get(&ext_adv, &ext_adv_size, false);
		le_audio_adv_get(&per_adv, &per_adv_size, true);

		ret = bt_mgmt_adv_start(ext_adv, ext_adv_size, per_adv, per_adv_size, false);
		ERR_CHK(ret);
	} else if ((CONFIG_AUDIO_DEV == GATEWAY) && IS_ENABLED(CONFIG_TRANSPORT_CIS)) {
		ret = bt_mgmt_scan_start(0, 0, BT_MGMT_SCAN_TYPE_CONN, CONFIG_BT_DEVICE_NAME);
		ERR_CHK_MSG(ret, "Failed to start scanning");
	} else if ((CONFIG_AUDIO_DEV == HEADSET) && IS_ENABLED(CONFIG_TRANSPORT_BIS)) {
		ret = bt_mgmt_scan_start(0, 0, BT_MGMT_SCAN_TYPE_BROADCAST,
					 CONFIG_BT_AUDIO_BROADCAST_NAME);
		ERR_CHK_MSG(ret, "Failed to start scanning");
	}

	started = true;

	return 0;
}
