/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

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

BUILD_ASSERT(CONFIG_BT_AUDIO_BROADCAST_SRC_STREAM_COUNT <= 2,
	     "A maximum of two streams are currently supported");

#define HCI_ISO_BUF_ALLOC_PER_CHAN 2

#define BT_AUDIO_LC3_BROADCAST_PRESET_NRF5340_AUDIO                                                \
	BT_AUDIO_LC3_PRESET(                                                                       \
		BT_CODEC_LC3_CONFIG(BT_CODEC_CONFIG_LC3_FREQ_48KHZ,                                \
				    BT_CODEC_CONFIG_LC3_DURATION_10, BT_AUDIO_LOCATION_FRONT_LEFT, \
				    LE_AUDIO_SDU_SIZE_OCTETS(CONFIG_LC3_BITRATE), 1,               \
				    BT_AUDIO_CONTEXT_TYPE_MEDIA),                                  \
		BT_CODEC_LC3_QOS_10_UNFRAMED(LE_AUDIO_SDU_SIZE_OCTETS(CONFIG_LC3_BITRATE), 4u,     \
					     20u, LE_AUDIO_PRES_DELAY_US))

/* For being able to dynamically define iso_tx_pools */
#define NET_BUF_POOL_ITERATE(i, _)                                                                 \
	NET_BUF_POOL_FIXED_DEFINE(iso_tx_pool_##i, HCI_ISO_BUF_ALLOC_PER_CHAN,                     \
				  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU), 8, NULL);
#define NET_BUF_POOL_PTR_ITERATE(i, ...) IDENTITY(&iso_tx_pool_##i)
LISTIFY(CONFIG_BT_ISO_MAX_CHAN, NET_BUF_POOL_ITERATE, (;))

/* clang-format off */
static struct net_buf_pool *iso_tx_pools[] = { LISTIFY(CONFIG_BT_ISO_MAX_CHAN,
						       NET_BUF_POOL_PTR_ITERATE, (,)) };
/* clang-format on */

static struct bt_audio_broadcast_source *broadcast_source;

static struct bt_audio_stream streams[CONFIG_BT_AUDIO_BROADCAST_SRC_STREAM_COUNT];
static struct bt_audio_stream *streams_p[ARRAY_SIZE(streams)];

static struct bt_audio_lc3_preset lc3_preset = BT_AUDIO_LC3_BROADCAST_PRESET_NRF5340_AUDIO;

static atomic_t iso_tx_pool_alloc[CONFIG_BT_ISO_MAX_CHAN];
static bool delete_broadcast_src;
static uint32_t seq_num[CONFIG_BT_ISO_MAX_CHAN];

static bool is_iso_buffer_full(uint8_t idx)
{
	/* net_buf_alloc allocates buffers for APP->NET transfer over HCI RPMsg,
	 * but when these buffers are released it is not guaranteed that the
	 * data has actually been sent. The data might be qued on the NET core,
	 * and this can cause delays in the audio.
	 * When stream_sent_cb() is called the data has been sent.
	 * Data will be discarded if allocation becomes too high, to avoid audio delays.
	 * If the NET and APP core operates in clock sync, discarding should not occur.
	 */

	if (atomic_get(&iso_tx_pool_alloc[idx]) >= HCI_ISO_BUF_ALLOC_PER_CHAN) {
		return true;
	}

	return false;
}

static int get_stream_index(struct bt_audio_stream *stream, uint8_t *index)
{
	for (size_t i = 0U; i < ARRAY_SIZE(streams); i++) {
		if (&streams[i] == stream) {
			*index = i;
			return 0;
		}
	}

	LOG_WRN("Stream %p not found", (void *)stream);

	return -EINVAL;
}

static void stream_sent_cb(struct bt_audio_stream *stream)
{
	static uint32_t sent_cnt[ARRAY_SIZE(streams)];
	uint8_t index;

	get_stream_index(stream, &index);

	if (atomic_get(&iso_tx_pool_alloc[index])) {
		atomic_dec(&iso_tx_pool_alloc[index]);
	} else {
		LOG_WRN("Decreasing atomic variable for stream %u failed", index);
	}

	sent_cnt[index]++;

	if ((sent_cnt[index] % 1000U) == 0U) {
		LOG_DBG("Sent %u total ISO packets on stream %u", sent_cnt[index], index);
	}
}

static void stream_started_cb(struct bt_audio_stream *stream)
{
	int ret;
	uint8_t index;

	get_stream_index(stream, &index);
	seq_num[index] = 0;

	ret = ctrl_events_le_audio_event_send(LE_AUDIO_EVT_STREAMING);
	ERR_CHK(ret);

	LOG_INF("Broadcast source %p started", (void *)stream);
}

static void stream_stopped_cb(struct bt_audio_stream *stream)
{
	int ret;

	ret = ctrl_events_le_audio_event_send(LE_AUDIO_EVT_NOT_STREAMING);
	ERR_CHK(ret);

	LOG_INF("Broadcast source %p stopped", (void *)stream);

	if (delete_broadcast_src && broadcast_source != NULL) {
		ret = bt_audio_broadcast_source_delete(broadcast_source);
		if (ret) {
			LOG_ERR("Unable to delete broadcast source %p", (void *)stream);
			delete_broadcast_src = false;
			return;
		}

		broadcast_source = NULL;

		LOG_INF("Broadcast source %p deleted", (void *)stream);

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
			streams_p[i] = &streams[i];
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

	ret = bt_audio_broadcast_source_start(broadcast_source);
	if (ret) {
		LOG_WRN("Failed to start broadcast, ret: %d", ret);
	}

	return ret;
}

int le_audio_pause(void)
{
	int ret;

	ret = bt_audio_broadcast_source_stop(broadcast_source);
	if (ret) {
		LOG_WRN("Failed to stop broadcast, ret: %d", ret);
	}

	return ret;
}

int le_audio_send(uint8_t const *const data, size_t size)
{
	int ret;
	static bool wrn_printed[CONFIG_BT_ISO_MAX_CHAN];
	struct net_buf *buf;
	size_t num_streams = ARRAY_SIZE(streams);
	size_t data_size = size / num_streams;

	for (size_t i = 0U; i < num_streams; i++) {
		if (streams[i].ep->status.state != BT_AUDIO_EP_STATE_STREAMING) {
			LOG_DBG("Stream %d not in streaming state", i);
			continue;
		}

		if (is_iso_buffer_full(i)) {
			if (!wrn_printed[i]) {
				LOG_WRN("HCI ISO TX overrun on ch %d - Single print", i);
				wrn_printed[i] = true;
			}

			return -ENOMEM;
		}

		wrn_printed[i] = false;

		buf = net_buf_alloc(iso_tx_pools[i], K_NO_WAIT);
		if (buf == NULL) {
			/* This should never occur because of the is_iso_buffer_full() check */
			LOG_WRN("Out of TX buffers");
			return -ENOMEM;
		}

		net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);
		net_buf_add_mem(buf, &data[i * data_size], data_size);

		atomic_inc(&iso_tx_pool_alloc[i]);

		ret = bt_audio_stream_send(&streams[i], buf, seq_num[i]++, BT_ISO_TIMESTAMP_NONE);
		if (ret < 0) {
			LOG_WRN("Failed to send audio data: %d", ret);
			net_buf_unref(buf);
			atomic_dec(&iso_tx_pool_alloc[i]);
			return ret;
		}
	}

#if (CONFIG_AUDIO_SOURCE_I2S)
	struct bt_iso_tx_info tx_info = { 0 };

	ret = bt_iso_chan_get_tx_sync(streams[0].iso, &tx_info);

	if (ret) {
		LOG_DBG("Error getting ISO TX anchor point: %d", ret);
	} else {
		audio_datapath_sdu_ref_update(tx_info.ts);
	}
#endif

	return 0;
}

int le_audio_enable(le_audio_receive_cb recv_cb)
{
	int ret;

	initialize();

	LOG_DBG("Creating broadcast source");

	ret = bt_audio_broadcast_source_create(streams_p, ARRAY_SIZE(streams_p), &lc3_preset.codec,
					       &lc3_preset.qos, &broadcast_source);
	if (ret) {
		return ret;
	}

	LOG_DBG("Starting broadcast source");

	ret = bt_audio_broadcast_source_start(broadcast_source);
	if (ret) {
		return ret;
	}

	LOG_DBG("LE Audio enabled");

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

	LOG_DBG("LE Audio disabled");

	return 0;
}
