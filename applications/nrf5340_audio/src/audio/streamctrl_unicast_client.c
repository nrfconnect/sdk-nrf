/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "streamctrl.h"

#include <zephyr/zbus/zbus.h>

#include "nrf5340_audio_common.h"
#include "led.h"
#include "button_assignments.h"
#include "macros_common.h"
#include "audio_system.h"
#include "button_handler.h"
#include "le_audio.h"
#include "bt_mgmt.h"
#include "bt_rend.h"
#include "bt_content_ctrl.h"
#include "unicast_client.h"
#include "le_audio_rx.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(streamctrl_unicast_client, CONFIG_STREAMCTRL_LOG_LEVEL);

static enum stream_state strm_state = STATE_PAUSED;

ZBUS_SUBSCRIBER_DEFINE(button_evt_sub, CONFIG_BUTTON_MSG_SUB_QUEUE_SIZE);
ZBUS_SUBSCRIBER_DEFINE(content_control_evt_sub, CONFIG_CONTENT_CONTROL_MSG_SUB_QUEUE_SIZE);

ZBUS_MSG_SUBSCRIBER_DEFINE(le_audio_evt_sub);

ZBUS_CHAN_DECLARE(button_chan);
ZBUS_CHAN_DECLARE(le_audio_chan);
ZBUS_CHAN_DECLARE(bt_mgmt_chan);
ZBUS_CHAN_DECLARE(cont_media_chan);
ZBUS_CHAN_DECLARE(sdu_ref_chan);

ZBUS_OBS_DECLARE(sdu_ref_msg_listen);

static struct k_thread button_msg_sub_thread_data;
static struct k_thread le_audio_msg_sub_thread_data;
static struct k_thread content_control_msg_sub_thread_data;

static k_tid_t button_msg_sub_thread_id;
static k_tid_t le_audio_msg_sub_thread_id;
static k_tid_t content_control_thread_id;

K_THREAD_STACK_DEFINE(button_msg_sub_thread_stack, CONFIG_BUTTON_MSG_SUB_STACK_SIZE);
K_THREAD_STACK_DEFINE(le_audio_msg_sub_thread_stack, CONFIG_LE_AUDIO_MSG_SUB_STACK_SIZE);
K_THREAD_STACK_DEFINE(content_control_msg_sub_thread_stack,
		      CONFIG_CONTENT_CONTROL_MSG_SUB_STACK_SIZE);

/* Function for handling all stream state changes */
static void stream_state_set(enum stream_state stream_state_new)
{
	strm_state = stream_state_new;
}

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

		switch (msg.event) {
		case MEDIA_START:
			unicast_client_start();
			break;

		case MEDIA_STOP:
			unicast_client_stop();
			break;

		default:
			LOG_WRN("Unhandled event from content ctrl: %d", msg.event);
			break;
		}

		STACK_USAGE_PRINT("content_ctrl_msg_thread", &content_control_msg_sub_thread);
	}
}

/**
 * @brief	Handle button activity.
 */
static void button_msg_sub_thread(void)
{
	int ret;
	const struct zbus_channel *chan;

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
				break;
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

				if (strm_state != STATE_STREAMING) {
					LOG_WRN("Not in streaming state");
					break;
				}

				ret = audio_system_encode_test_tone_step();
				if (ret) {
					LOG_WRN("Failed to play test tone, ret: %d", ret);
				}

				break;
			}

			break;

		case BUTTON_5:
			if (IS_ENABLED(CONFIG_AUDIO_MUTE)) {
				ret = bt_rend_volume_mute(false);
				if (ret) {
					LOG_WRN("Failed to mute, ret: %d", ret);
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

/**
 * @brief	Handle Bluetooth LE audio events.
 */
static void le_audio_msg_sub_thread(void)
{
	int ret;
	const struct zbus_channel *chan;

	while (1) {
		struct le_audio_msg msg;

		ret = zbus_sub_wait_msg(&le_audio_evt_sub, &chan, &msg, K_FOREVER);
		ERR_CHK(ret);

		LOG_DBG("Received event = %d, current state = %d", msg.event, strm_state);

		switch (msg.event) {
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

		default:
			LOG_WRN("Unexpected/unhandled le_audio event: %d", msg.event);
			break;
		}

		STACK_USAGE_PRINT("le_audio_msg_thread", &le_audio_msg_sub_thread_data);
	}
}

/**
 * @brief	Zbus listener to receive events from bt_mgmt.
 *
 * @param[in]	chan	Zbus channel.
 *
 * @note	Will in most cases be called from BT_RX context,
 *		so there should not be too much processing done here.
 */
static void bt_mgmt_evt_handler(const struct zbus_channel *chan)
{
	int ret;
	const struct bt_mgmt_msg *msg;

	msg = zbus_chan_const_msg(chan);

	switch (msg->event) {
	case BT_MGMT_CONNECTED:
		LOG_INF("Device connected");
		break;

	case BT_MGMT_SECURITY_CHANGED:
		LOG_INF("Security changed");

		ret = bt_rend_discover(msg->conn);
		if (ret) {
			LOG_WRN("Failed to discover rendering services");
		}

		if (IS_ENABLED(CONFIG_STREAM_BIDIRECTIONAL)) {
			ret = unicast_client_discover(msg->conn, UNICAST_SERVER_BIDIR);
		} else {
			ret = unicast_client_discover(msg->conn, UNICAST_SERVER_SINK);
		}

		if (ret) {
			LOG_ERR("Failed to handle unicast client discover: %d", ret);
		}

		break;

	case BT_MGMT_DISCONNECTED:
		LOG_INF("Device disconnected");

		unicast_client_conn_disconnected(msg->conn);
		break;

	default:
		LOG_WRN("Unexpected/unhandled bt_mgmt event: %d", msg->event);
		break;
	}
}

ZBUS_LISTENER_DEFINE(bt_mgmt_evt_listen, bt_mgmt_evt_handler);

/**
 * @brief	Create zbus subscriber threads.
 *
 * @return	0 for success, error otherwise.
 */
static int zbus_subscribers_create(void)
{
	int ret;

	button_msg_sub_thread_id = k_thread_create(
		&button_msg_sub_thread_data, button_msg_sub_thread_stack,
		CONFIG_BUTTON_MSG_SUB_STACK_SIZE, (k_thread_entry_t)button_msg_sub_thread, NULL,
		NULL, NULL, K_PRIO_PREEMPT(CONFIG_BUTTON_MSG_SUB_THREAD_PRIO), 0, K_NO_WAIT);
	ret = k_thread_name_set(button_msg_sub_thread_id, "BUTTON_MSG_SUB");
	if (ret) {
		LOG_ERR("Failed to create button_msg thread");
		return ret;
	}

	le_audio_msg_sub_thread_id = k_thread_create(
		&le_audio_msg_sub_thread_data, le_audio_msg_sub_thread_stack,
		CONFIG_LE_AUDIO_MSG_SUB_STACK_SIZE, (k_thread_entry_t)le_audio_msg_sub_thread, NULL,
		NULL, NULL, K_PRIO_PREEMPT(CONFIG_LE_AUDIO_MSG_SUB_THREAD_PRIO), 0, K_NO_WAIT);
	ret = k_thread_name_set(le_audio_msg_sub_thread_id, "LE_AUDIO_MSG_SUB");
	if (ret) {
		LOG_ERR("Failed to create le_audio_msg thread");
		return ret;
	}

	content_control_thread_id = k_thread_create(
		&content_control_msg_sub_thread_data, content_control_msg_sub_thread_stack,
		CONFIG_CONTENT_CONTROL_MSG_SUB_STACK_SIZE,
		(k_thread_entry_t)content_control_msg_sub_thread, NULL, NULL, NULL,
		K_PRIO_PREEMPT(CONFIG_CONTENT_CONTROL_MSG_SUB_THREAD_PRIO), 0, K_NO_WAIT);
	ret = k_thread_name_set(content_control_thread_id, "CONTENT_CONTROL_MSG_SUB");
	if (ret) {
		return ret;
	}

	ret = zbus_chan_add_obs(&sdu_ref_chan, &sdu_ref_msg_listen, ZBUS_ADD_OBS_TIMEOUT_MS);
	if (ret) {
		LOG_ERR("Failed to add timestamp listener");
		return ret;
	}

	return 0;
}

/**
 * @brief	Link zbus producers and observers.
 *
 * @return	0 for success, error otherwise.
 */
static int zbus_link_producers_observers(void)
{
	int ret;

	if (!IS_ENABLED(CONFIG_ZBUS)) {
		return -ENOTSUP;
	}

	ret = zbus_chan_add_obs(&button_chan, &button_evt_sub, ZBUS_ADD_OBS_TIMEOUT_MS);
	if (ret) {
		LOG_ERR("Failed to add button sub");
		return ret;
	}

	ret = zbus_chan_add_obs(&le_audio_chan, &le_audio_evt_sub, ZBUS_ADD_OBS_TIMEOUT_MS);
	if (ret) {
		LOG_ERR("Failed to add le_audio sub");
		return ret;
	}

	ret = zbus_chan_add_obs(&bt_mgmt_chan, &bt_mgmt_evt_listen, ZBUS_ADD_OBS_TIMEOUT_MS);
	if (ret) {
		LOG_ERR("Failed to add bt_mgmt listener");
		return ret;
	}

	ret = zbus_chan_add_obs(&cont_media_chan, &content_control_evt_sub,
				ZBUS_ADD_OBS_TIMEOUT_MS);

	return 0;
}

uint8_t stream_state_get(void)
{
	return strm_state;
}

void streamctrl_send(void const *const data, size_t size, uint8_t num_ch)
{
	int ret;
	static int prev_ret;

	struct le_audio_encoded_audio enc_audio = {.data = data, .size = size, .num_ch = num_ch};

	if (strm_state == STATE_STREAMING) {
		ret = unicast_client_send(enc_audio);

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

int streamctrl_start(void)
{
	int ret;
	static bool started;

	if (started) {
		LOG_WRN("Streamctrl already started");
		return -EALREADY;
	}

	ret = audio_system_init();
	ERR_CHK_MSG(ret, "Failed to initialize the audio system");

	ret = zbus_subscribers_create();
	ERR_CHK_MSG(ret, "Failed to create zbus subscriber threads");

	ret = zbus_link_producers_observers();
	ERR_CHK_MSG(ret, "Failed to link zbus producers and observers");

	ret = le_audio_rx_init();
	ERR_CHK(ret);

	ret = bt_rend_init();
	ERR_CHK(ret);

	ret = bt_content_ctrl_init();
	ERR_CHK(ret);

	ret = unicast_client_enable(le_audio_rx_data_handler);
	ERR_CHK(ret);

	started = true;

	ret = bt_mgmt_scan_start(0, 0, BT_MGMT_SCAN_TYPE_CONN, CONFIG_BT_DEVICE_NAME);
	if (ret) {
		LOG_ERR("Failed to start scanning");
		return ret;
	}

	return 0;
}
