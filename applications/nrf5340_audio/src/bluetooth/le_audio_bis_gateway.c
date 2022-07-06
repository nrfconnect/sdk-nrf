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

BUILD_ASSERT(CONFIG_BT_AUDIO_BROADCAST_SRC_STREAM_COUNT <= 1,
	     "Only one stream is currently supported");

#define HCI_ISO_BUF_ALLOC_PER_CHAN 2

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

static struct bt_audio_lc3_preset lc3_preset = BT_AUDIO_LC3_BROADCAST_PRESET_48_4_1;

static atomic_t iso_tx_pool_alloc[CONFIG_BT_ISO_MAX_CHAN];
static bool delete_broadcast_src;

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

static void stream_sent_cb(struct bt_audio_stream *stream)
{
	static uint32_t sent_cnt;

	atomic_dec(&iso_tx_pool_alloc[0]);

	sent_cnt++;

	if ((sent_cnt % 1000U) == 0U) {
		LOG_INF("Sent %u total ISO packets", sent_cnt);
	}
}

static void stream_started_cb(struct bt_audio_stream *stream)
{
	int ret;

	ret = ctrl_events_le_audio_event_send(LE_AUDIO_EVT_STREAMING);
	ERR_CHK(ret);

	LOG_INF("Broadcast source started");
}

static void stream_stopped_cb(struct bt_audio_stream *stream)
{
	int ret;

	ret = ctrl_events_le_audio_event_send(LE_AUDIO_EVT_NOT_STREAMING);
	ERR_CHK(ret);

	atomic_clear(&iso_tx_pool_alloc[0]);

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
	stream_started_cb(&streams[0]);
	return 0;
}

int le_audio_pause(void)
{
	stream_stopped_cb(&streams[0]);
	return 0;
}

int le_audio_send(uint8_t const *const data, size_t size)
{
	int ret;
	static bool wrn_printed[CONFIG_BT_ISO_MAX_CHAN];
	struct net_buf *buf;

	if (streams[0].ep->status.state != BT_AUDIO_EP_STATE_STREAMING) {
		return -ECANCELED;
	}

	if (is_iso_buffer_full(0)) {
		if (!wrn_printed[0]) {
			LOG_WRN("HCI ISO TX overrun on ch %d - Single print", 0);
			wrn_printed[0] = true;
		}

		return -ENOMEM;
	}

	wrn_printed[0] = false;

	buf = net_buf_alloc(iso_tx_pools[0], K_NO_WAIT);
	if (buf == NULL) {
		/* This should never occur because of the is_iso_buffer_full() check */
		LOG_WRN("Out of TX buffers");
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

	atomic_inc(&iso_tx_pool_alloc[0]);

	ret = bt_audio_stream_send(&streams[0], buf);
	if (ret < 0) {
		LOG_WRN("Failed to send audio data: %d", ret);
		net_buf_unref(buf);
		atomic_dec(&iso_tx_pool_alloc[0]);
		return ret;
	}

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
