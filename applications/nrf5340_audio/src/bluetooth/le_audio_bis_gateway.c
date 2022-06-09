/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#if (CONFIG_AUDIO_DEV == GATEWAY)

#include "le_audio.h"

#include <bluetooth/bluetooth.h>
#include <bluetooth/audio/audio.h>
/* TODO: Remove when a get_info function is implemented in host */
#include <../subsys/bluetooth/audio/endpoint.h>

#include "macros_common.h"
#include "ctrl_events.h"
#include "audio_datapath.h"

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
static bool delete_broadcast_src;

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

	if (delete_broadcast_src && broadcast_source != NULL) {
		ret = bt_audio_broadcast_source_delete(broadcast_source);
		if (ret) {
			LOG_ERR("Unable to delete broadcast source");
			delete_broadcast_src = false;
			return;
		}

		broadcast_source = NULL;

		LOG_INF("Broadcast source deleted");

		delete_broadcast_src = false;
	}
}

static struct bt_audio_stream_ops stream_ops = { .sent = stream_sent_cb,
						 .started = stream_started_cb,
						 .stopped = stream_stopped_cb };

static void initialize(void)
{
	static bool initialized;

	if (!initialized) {
		for (size_t i = 0U; i < ARRAY_SIZE(streams); i++) {
			streams[i].ops = &stream_ops;
		}

		initialized = true;
	}
}

int le_audio_config_get(uint32_t *bitrate, uint32_t *sampling_rate)
{
	LOG_WRN("Not possible to get config on broadcast source");
	return -ENXIO;
}

int le_audio_volume_up(void)
{
	LOG_WRN("Not possible to increase volume on/from broadcast source");
	return -ENXIO;
}

int le_audio_volume_down(void)
{
	LOG_WRN("Not possible to decrease volume on/from broadcast source");
	return -ENXIO;
}

int le_audio_volume_mute(void)
{
	LOG_WRN("Not possible to mute volume on/from broadcast source");
	return -ENXIO;
}

int le_audio_play(void)
{
	int ret;
	struct event_t event;

	event.event_source = EVT_SRC_LE_AUDIO;
	event.le_audio_activity.le_audio_evt_type = LE_AUDIO_EVT_STREAMING;

	ret = ctrl_events_put(&event);

	return ret;
}

int le_audio_pause(void)
{
	int ret;
	struct event_t event;

	event.event_source = EVT_SRC_LE_AUDIO;
	event.le_audio_activity.le_audio_evt_type = LE_AUDIO_EVT_NOT_STREAMING;

	ret = ctrl_events_put(&event);

	return ret;
}

int le_audio_send(uint8_t const *const data, size_t size)
{
	int ret;
	struct net_buf *buf;

	if (streams[0].ep->status.state != BT_AUDIO_EP_STATE_STREAMING) {
		return -ECANCELED;
	}

	buf = net_buf_alloc(&tx_pool, K_FOREVER);
	if (buf == NULL) {
		LOG_WRN("Could not allocate buffer when sending");
		return -ENOMEM;
	}

	net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);
	net_buf_add_mem(buf, data, size);

#if (CONFIG_AUDIO_SOURCE_I2S)
	struct bt_iso_tx_info tx_info = { 0 };

	ret = bt_iso_chan_get_tx_sync(streams[0].iso, &tx_info);

	if (ret) {
		LOG_WRN("Error getting ISO TX anchor point: %d", ret);
	} else {
		audio_datapath_sdu_ref_update(tx_info.ts);
	}
#endif

	ret = bt_audio_stream_send(streams, buf);
	if (ret < 0) {
		net_buf_unref(buf);
		return ret;
	}

	send_req_cnt++;

	return 0;
}

int le_audio_enable(le_audio_receive_cb recv_cb)
{
	int ret;

	initialize();

	LOG_INF("Creating broadcast source");

	ret = bt_audio_broadcast_source_create(streams, ARRAY_SIZE(streams), &lc3_preset.codec,
					       &lc3_preset.qos, &broadcast_source);
	if (ret) {
		return ret;
	}

	LOG_INF("Starting broadcast source");

	ret = bt_audio_broadcast_source_start(broadcast_source);
	if (ret) {
		return ret;
	}

	LOG_INF("LE Audio enabled");

	return 0;
}

int le_audio_disable(void)
{
	int ret;

	if (streams[0].ep->status.state == BT_AUDIO_EP_STATE_STREAMING) {
		/* Deleting broadcast source in stream_stopped_cb() */
		delete_broadcast_src = true;

		ret = bt_audio_broadcast_source_stop(broadcast_source);
		if (ret) {
			return ret;
		}
	} else if (broadcast_source != NULL) {
		ret = bt_audio_broadcast_source_delete(broadcast_source);
		if (ret) {
			return ret;
		}

		broadcast_source = NULL;
	}

	LOG_INF("LE Audio disabled");

	return 0;
}

#endif /* (CONFIG_AUDIO_DEV == GATEWAY) */
