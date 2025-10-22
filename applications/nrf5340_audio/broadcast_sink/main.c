/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "streamctrl.h"

#include <zephyr/kernel.h>
#include <zephyr/zbus/zbus.h>

#include "broadcast_sink.h"
#include "zbus_common.h"
#include "peripherals.h"
#include "led_assignments.h"
#include "led.h"
#include "button_assignments.h"
#include "macros_common.h"
#include "audio_system.h"
#include "bt_mgmt.h"
#include "bt_rendering_and_capture.h"
#include "audio_datapath.h"
#include "le_audio_rx.h"
#include "fw_info_app.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, CONFIG_MAIN_LOG_LEVEL);

BUILD_ASSERT(CONFIG_BT_AUDIO_CONCURRENT_RX_STREAMS_MAX <= CONFIG_AUDIO_DECODE_CHANNELS_MAX);

struct ble_iso_data {
	uint8_t data[CONFIG_BT_ISO_RX_MTU];
	size_t data_size;
	bool bad_frame;
	uint32_t sdu_ref;
	uint32_t recv_frame_ts;
} __packed;

static uint32_t last_broadcast_id = BRDCAST_ID_NOT_USED;

ZBUS_SUBSCRIBER_DEFINE(button_evt_sub, CONFIG_BUTTON_MSG_SUB_QUEUE_SIZE);

ZBUS_MSG_SUBSCRIBER_DEFINE(le_audio_evt_sub);
ZBUS_MSG_SUBSCRIBER_DEFINE(bt_mgmt_evt_sub);

ZBUS_CHAN_DECLARE(button_chan);
ZBUS_CHAN_DECLARE(le_audio_chan);
ZBUS_CHAN_DECLARE(bt_mgmt_chan);
ZBUS_CHAN_DECLARE(volume_chan);

ZBUS_OBS_DECLARE(volume_evt_sub);

static struct k_thread button_msg_sub_thread_data;
static struct k_thread le_audio_msg_sub_thread_data;
static struct k_thread bt_mgmt_msg_sub_thread_data;

static k_tid_t button_msg_sub_thread_id;
static k_tid_t le_audio_msg_sub_thread_id;
static k_tid_t bt_mgmt_msg_sub_thread_id;

K_THREAD_STACK_DEFINE(button_msg_sub_thread_stack, CONFIG_BUTTON_MSG_SUB_STACK_SIZE);
K_THREAD_STACK_DEFINE(le_audio_msg_sub_thread_stack, CONFIG_LE_AUDIO_MSG_SUB_STACK_SIZE);
K_THREAD_STACK_DEFINE(bt_mgmt_msg_sub_thread_stack, CONFIG_BT_MGMT_MSG_SUB_STACK_SIZE);

static enum stream_state strm_state = STATE_PAUSED;

/* Function for handling all stream state changes */
static void stream_state_set(enum stream_state stream_state_new)
{
	strm_state = stream_state_new;
}

/**
 * @brief	Handle button activity.
 */
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
			continue;
		}

		switch (msg.button_pin) {
		case BUTTON_PLAY_PAUSE:
			if (strm_state == STATE_STREAMING) {
				ret = broadcast_sink_stop();
				if (ret) {
					LOG_WRN("Failed to stop broadcast sink: %d", ret);
				}
			} else if (strm_state == STATE_PAUSED) {
				ret = broadcast_sink_start();
				if (ret) {
					LOG_WRN("Failed to start broadcast sink: %d", ret);
				}
			} else {
				LOG_WRN("In invalid state: %d", strm_state);
			}

			break;

		case BUTTON_VOLUME_UP:
			ret = bt_r_and_c_volume_up();
			if (ret) {
				LOG_WRN("Failed to increase volume: %d", ret);
			}

			break;

		case BUTTON_VOLUME_DOWN:
			ret = bt_r_and_c_volume_down();
			if (ret) {
				LOG_WRN("Failed to decrease volume: %d", ret);
			}

			break;

		case BUTTON_4:
			/* Unused button */
			break;

		case BUTTON_5:
			if (IS_ENABLED(CONFIG_AUDIO_MUTE)) {
				ret = bt_r_and_c_volume_mute(false);
				if (ret) {
					LOG_WRN("Failed to mute, ret: %d", ret);
				}

				break;
			}

			ret = broadcast_sink_disable();
			if (ret) {
				LOG_ERR("Failed to disable the broadcast sink: %d", ret);
				break;
			}

			if (broadcast_alt) {
				ret = bt_mgmt_scan_start(0, 0, BT_MGMT_SCAN_TYPE_BROADCAST,
							 CONFIG_BT_AUDIO_BROADCAST_NAME_ALT,
							 BRDCAST_ID_NOT_USED);
				broadcast_alt = false;
			} else {
				ret = bt_mgmt_scan_start(0, 0, BT_MGMT_SCAN_TYPE_BROADCAST,
							 CONFIG_BT_AUDIO_BROADCAST_NAME,
							 BRDCAST_ID_NOT_USED);
				broadcast_alt = true;
			}

			if (ret) {
				LOG_WRN("Failed to start scanning for broadcaster: %d", ret);
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
	uint32_t pres_delay_us;
	uint32_t bitrate_bps;
	uint32_t sampling_rate_hz;

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
			ret = led_blink(LED_AUDIO_CONN_STATUS);
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
			ret = led_on(LED_AUDIO_CONN_STATUS);
			ERR_CHK(ret);

			break;

		case LE_AUDIO_EVT_CONFIG_RECEIVED:
			LOG_DBG("LE audio config received");

			ret = broadcast_sink_config_get(&bitrate_bps, &sampling_rate_hz,
							&pres_delay_us);
			if (ret) {
				LOG_WRN("Failed to get config: %d", ret);
				break;
			}

			LOG_DBG("\tSampling rate: %d Hz", sampling_rate_hz);
			LOG_DBG("\tBitrate (compressed): %d bps", bitrate_bps);

			ret = audio_system_config_set(VALUE_NOT_SET, VALUE_NOT_SET,
						      sampling_rate_hz);
			ERR_CHK(ret);

			ret = audio_datapath_pres_delay_us_set(pres_delay_us);
			if (ret) {
				break;
			}

			LOG_INF("Presentation delay %d us is set", pres_delay_us);

			break;

		case LE_AUDIO_EVT_SYNC_LOST:
			LOG_INF("Sync lost");

			ret = bt_mgmt_pa_sync_delete(msg.pa_sync);
			if (ret) {
				LOG_WRN("Failed to delete PA sync");
			}

			if (strm_state == STATE_STREAMING) {
				stream_state_set(STATE_PAUSED);
				audio_system_stop();
				ret = led_on(LED_AUDIO_CONN_STATUS);
				ERR_CHK(ret);
			}

			if (IS_ENABLED(CONFIG_BT_OBSERVER)) {
				ret = bt_mgmt_scan_start(0, 0, BT_MGMT_SCAN_TYPE_BROADCAST, NULL,
							 last_broadcast_id);
				if (ret) {
					if (ret == -EALREADY) {
						break;
					}

					LOG_ERR("Failed to restart scanning: %d", ret);
					break;
				}

				/* NOTE: The string below is used by the Nordic CI system */
				LOG_INF("Restarted scanning for broadcaster");
			}

			break;

		case LE_AUDIO_EVT_NO_VALID_CFG:
			LOG_WRN("No valid configurations found, disabling the broadcast sink");

			ret = broadcast_sink_disable();
			if (ret) {
				LOG_ERR("Failed to disable the broadcast sink: %d", ret);
				break;
			}

			break;

		case LE_AUDIO_EVT_STREAM_SENT:
			/* Nothing to do. */
			break;

		default:
			LOG_WRN("Unexpected/unhandled le_audio event: %d", msg.event);

			break;
		}

		STACK_USAGE_PRINT("le_audio_msg_thread", &le_audio_msg_sub_thread_data);
	}
}

/**
 * @brief	Handle bt_mgmt events.
 */
static void bt_mgmt_msg_sub_thread(void)
{
	int ret;
	static uint8_t *broadcast_code;
	const struct zbus_channel *chan;

	while (1) {
		struct bt_mgmt_msg msg;

		ret = zbus_sub_wait_msg(&bt_mgmt_evt_sub, &chan, &msg, K_FOREVER);
		ERR_CHK(ret);

		switch (msg.event) {
		case BT_MGMT_CONNECTED:
			LOG_DBG("Connected");
			break;

		case BT_MGMT_DISCONNECTED:
			LOG_DBG("Disconnected");
			break;

		case BT_MGMT_SECURITY_CHANGED:
			LOG_DBG("Security changed");
			break;

		case BT_MGMT_PA_SYNCED:
			LOG_DBG("PA synced");

			ret = broadcast_sink_pa_sync_set(msg.pa_sync, msg.broadcast_id);
			if (ret) {
				last_broadcast_id = BRDCAST_ID_NOT_USED;
				LOG_WRN("Failed to set PA sync");
			} else {
				last_broadcast_id = msg.broadcast_id;
			}

			break;

		case BT_MGMT_PA_SYNC_LOST:
			LOG_INF("PA sync lost, reason: %d", msg.pa_sync_term_reason);

			if (IS_ENABLED(CONFIG_BT_OBSERVER) &&
			    msg.pa_sync_term_reason != BT_HCI_ERR_LOCALHOST_TERM_CONN) {
				ret = bt_mgmt_scan_start(0, 0, BT_MGMT_SCAN_TYPE_BROADCAST, NULL,
							 BRDCAST_ID_NOT_USED);
				if (ret) {
					if (ret == -EALREADY) {
						LOG_ERR("Failed to restart scanning: %d", ret);
					}

					break;
				}

				/* NOTE: The string below is used by the Nordic CI system */
				LOG_INF("Restarted scanning for broadcaster");
			}

			break;

		case BT_MGMT_BROADCAST_SINK_DISABLE:
			LOG_DBG("Broadcast sink disabled");

			ret = broadcast_sink_disable();
			if (ret) {
				LOG_ERR("Failed to disable the broadcast sink: %d", ret);
			}

			break;

		case BT_MGMT_BROADCAST_CODE_RECEIVED:
			LOG_DBG("Broadcast code received");

			bt_mgmt_broadcast_code_ptr_get(&broadcast_code);

			ret = broadcast_sink_broadcast_code_set(broadcast_code);
			if (ret) {
				LOG_ERR("Failed to set broadcast code: %d", ret);
			}

			break;

		default:
			LOG_WRN("Unexpected/unhandled bt_mgmt event: %d", msg.event);

			break;
		}
	}
}

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

	bt_mgmt_msg_sub_thread_id = k_thread_create(
		&bt_mgmt_msg_sub_thread_data, bt_mgmt_msg_sub_thread_stack,
		CONFIG_BT_MGMT_MSG_SUB_STACK_SIZE, (k_thread_entry_t)bt_mgmt_msg_sub_thread, NULL,
		NULL, NULL, K_PRIO_PREEMPT(CONFIG_BT_MGMT_MSG_SUB_THREAD_PRIO), 0, K_NO_WAIT);
	ret = k_thread_name_set(bt_mgmt_msg_sub_thread_id, "BT_MGMT_MSG_SUB");
	if (ret) {
		LOG_ERR("Failed to create le_audio_msg thread");
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

	if (IS_ENABLED(CONFIG_BOARD_NRF5340_AUDIO_DK_NRF5340_CPUAPP)) {
		ret = zbus_chan_add_obs(&volume_chan, &volume_evt_sub, ZBUS_ADD_OBS_TIMEOUT_MS);
		if (ret) {
			LOG_ERR("Failed to add add volume sub");
			return ret;
		}
	}

	ret = zbus_chan_add_obs(&bt_mgmt_chan, &bt_mgmt_evt_sub, ZBUS_ADD_OBS_TIMEOUT_MS);
	if (ret) {
		LOG_ERR("Failed to add bt_mgmt sub");
		return ret;
	}

	return 0;
}

/*
 * @brief  The following configures the data for the extended advertising.
 *
 * @param  ext_adv_buf       Pointer to the bt_data used for extended advertising.
 * @param  ext_adv_buf_size  Size of @p ext_adv_buf.
 * @param  ext_adv_count     Pointer to the number of elements added to @p adv_buf.
 *
 * @return  0 for success, error otherwise.
 */
static int ext_adv_populate(struct bt_data *ext_adv_buf, size_t ext_adv_buf_size,
			    size_t *ext_adv_count)
{
	int ret;
	size_t ext_adv_buf_cnt = 0;

	NET_BUF_SIMPLE_DEFINE_STATIC(uuid_buf, CONFIG_EXT_ADV_UUID_BUF_MAX);

	ext_adv_buf[ext_adv_buf_cnt].type = BT_DATA_UUID16_ALL;
	ext_adv_buf[ext_adv_buf_cnt].data = uuid_buf.data;
	ext_adv_buf_cnt++;

	ret = bt_mgmt_manufacturer_uuid_populate(&uuid_buf, CONFIG_BT_DEVICE_MANUFACTURER_ID);
	if (ret) {
		LOG_ERR("Failed to add adv data with manufacturer ID: %d", ret);
		return ret;
	}

	ret = broadcast_sink_uuid_populate(&uuid_buf);
	if (ret < 0) {
		LOG_ERR("Failed to add UUID with broadcast sink: %d", ret);
		return ret;
	}

	ret = bt_r_and_c_uuid_populate(&uuid_buf);
	if (ret) {
		LOG_ERR("Failed to add adv data from renderer: %d", ret);
		return ret;
	}

	ext_adv_buf[ext_adv_buf_cnt].type = BT_DATA_NAME_COMPLETE;
	ext_adv_buf[ext_adv_buf_cnt].data = CONFIG_BT_DEVICE_NAME;
	ext_adv_buf[ext_adv_buf_cnt].data_len = sizeof(CONFIG_BT_DEVICE_NAME) - 1;
	ext_adv_buf_cnt++;

	ret = broadcast_sink_adv_populate(&ext_adv_buf[ext_adv_buf_cnt],
					  ext_adv_buf_size - ext_adv_buf_cnt);

	if (ret < 0) {
		LOG_ERR("Failed to add adv data from broadcast sink: %d", ret);
		return ret;
	}

	ext_adv_buf_cnt += ret;

	/* Add the number of UUIDs */
	ext_adv_buf[0].data_len = uuid_buf.len;

	LOG_DBG("Size of adv data: %d, num_elements: %d", sizeof(struct bt_data) * ext_adv_buf_cnt,
		ext_adv_buf_cnt);

	*ext_adv_count = ext_adv_buf_cnt;

	return 0;
}

uint8_t stream_state_get(void)
{
	return strm_state;
}

void streamctrl_send(struct net_buf const *const audio_frame)
{
	ARG_UNUSED(audio_frame);

	LOG_WRN("Sending is not possible for broadcast sink");
}

int main(void)
{
	int ret;

	LOG_DBG("Main started");

	ret = peripherals_init();
	ERR_CHK(ret);

	ret = fw_info_app_print();
	ERR_CHK(ret);

	ret = bt_mgmt_init();
	ERR_CHK(ret);

	ret = audio_system_init();
	ERR_CHK(ret);

	ret = zbus_subscribers_create();
	ERR_CHK_MSG(ret, "Failed to create zbus subscriber threads");

	ret = zbus_link_producers_observers();
	ERR_CHK_MSG(ret, "Failed to link zbus producers and observers");

	ret = le_audio_rx_init();
	ERR_CHK_MSG(ret, "Failed to initialize rx path");

	ret = broadcast_sink_enable(le_audio_rx_data_handler);
	ERR_CHK_MSG(ret, "Failed to enable broadcast sink");

	if (IS_ENABLED(CONFIG_BT_AUDIO_SCAN_DELEGATOR)) {
		static struct bt_data ext_adv_buf[CONFIG_EXT_ADV_BUF_MAX];
		size_t ext_adv_buf_cnt = 0;

		bt_mgmt_scan_delegator_init();

		ret = bt_r_and_c_init();
		ERR_CHK(ret);

		ret = ext_adv_populate(ext_adv_buf, ARRAY_SIZE(ext_adv_buf), &ext_adv_buf_cnt);
		ERR_CHK(ret);

		ret = bt_mgmt_adv_start(0, ext_adv_buf, ext_adv_buf_cnt, NULL, 0, true);
		ERR_CHK(ret);
	} else {
		ret = bt_mgmt_scan_start(0, 0, BT_MGMT_SCAN_TYPE_BROADCAST,
					 CONFIG_BT_AUDIO_BROADCAST_NAME, BRDCAST_ID_NOT_USED);
		ERR_CHK_MSG(ret, "Failed to start scanning");
	}

	return 0;
}
