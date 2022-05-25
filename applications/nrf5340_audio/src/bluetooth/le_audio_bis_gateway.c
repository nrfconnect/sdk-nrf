/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#if (CONFIG_AUDIO_DEV == GATEWAY)

#include <bluetooth/bluetooth.h>
#include <bluetooth/audio/audio.h>

#include "le_audio.h"
#include "macros_common.h"
#include "ctrl_events.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(bis_gateway, CONFIG_LOG_BLE_LEVEL);

static struct bt_audio_lc3_preset lc3_preset = BT_AUDIO_LC3_BROADCAST_PRESET_48_4_1;

static struct bt_audio_broadcast_source *broadcast_source;

BUILD_ASSERT(CONFIG_BT_AUDIO_BROADCAST_SRC_STREAM_COUNT <= 1,
	     "Only one stream is currently supported");
static struct bt_audio_stream streams[CONFIG_BT_AUDIO_BROADCAST_SRC_STREAM_COUNT];

NET_BUF_POOL_FIXED_DEFINE(tx_pool, CONFIG_BT_AUDIO_BROADCAST_SRC_STREAM_COUNT,
			  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU), 8, NULL);

static uint32_t send_req_cnt;
static uint32_t sent_cnt;
static bool stopping;

static void stream_sent_cb(struct bt_audio_stream *stream)
{
	int32_t diff;

	sent_cnt++;

	diff = send_req_cnt - sent_cnt;

	// TODO: Use the atomic functionality from ble_trans
	// if (diff < 0) {
	// 	/* Should not happen */
	// 	LOG_WRN("Sending underrun");
	// } else if (diff > 0) {
	// 	LOG_WRN("Sending overrun");
	// }

	if ((sent_cnt % 1000U) == 0U) {
		LOG_INF("Sent %u total ISO packets", sent_cnt);
	}
}

static void stream_started_cb(struct bt_audio_stream *stream)
{
	int ret;
	struct event_t event;

	event.event_source = EVT_SRC_LE_AUDIO;
	event.le_audio_activity.le_audio_evt_type = LE_AUDIO_EVT_STREAMING;

	ret = ctrl_events_put(&event);
	ERR_CHK(ret);

	LOG_INF("Broadcast source started");
}

static void stream_stopped_cb(struct bt_audio_stream *stream)
{
	int ret;
	struct event_t event;

	event.event_source = EVT_SRC_LE_AUDIO;
	event.le_audio_activity.le_audio_evt_type = LE_AUDIO_EVT_NOT_STREAMING;

	ret = ctrl_events_put(&event);
	ERR_CHK(ret);

	LOG_INF("Broadcast source stopped");
}

static struct bt_audio_stream_ops stream_ops = { .sent = stream_sent_cb,
						 .started = stream_started_cb,
						 .stopped = stream_stopped_cb };

int le_audio_config_get(void)
{
	return 0;
}

int le_audio_volume_up(void)
{
	return 0;
}

int le_audio_volume_down(void)
{
	return 0;
}

int le_audio_volume_mute(void)
{
	return 0;
}

int le_audio_play(void)
{
	return 0;
}

int le_audio_pause(void)
{
	return 0;
}

int le_audio_send(uint8_t const *const data, size_t size)
{
	// TODO: Implement the synchronization workaround created by Audun here? (OR ONLY FOR CIS?)
	// It this upstreamed now?

	int ret;
	struct net_buf *buf;

	if (stopping) {
		return -ECANCELED;
	}

	buf = net_buf_alloc(&tx_pool, K_FOREVER);
	if (buf == NULL) {
		LOG_WRN("Could not allocate buffer when sending");
		return -ENOMEM;
	}

	net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);
	net_buf_add_mem(buf, data, size);

	ret = bt_audio_stream_send(streams, buf);
	if (ret < 0) {
		LOG_WRN("Unable to broadcast data: %d", ret);
		net_buf_unref(buf);
		return ret;
	}

	send_req_cnt++;

	return 0;
}

// TODO: Implement return code
void le_audio_enable(le_audio_receive_cb recv_cb)
{
	int ret;

	// TODO: Create le_audio_initialize()
	for (size_t i = 0U; i < ARRAY_SIZE(streams); i++) {
		streams[i].ops = &stream_ops;
	}

	LOG_INF("Creating broadcast source");

	ret = bt_audio_broadcast_source_create(streams, ARRAY_SIZE(streams), &lc3_preset.codec,
					       &lc3_preset.qos, &broadcast_source);
	ERR_CHK_MSG(ret, "Unable to create broadcast source");

	LOG_INF("Starting broadcast source");

	stopping = false;

	ret = bt_audio_broadcast_source_start(broadcast_source);
	ERR_CHK_MSG(ret, "Unable to start broadcast source");
}

// TODO: Implement return code
void le_audio_disable(void)
{
	// TODO: Stopping functionality is in broadcast_audio_source sample
}

#endif /* (CONFIG_AUDIO_DEV == GATEWAY) */
