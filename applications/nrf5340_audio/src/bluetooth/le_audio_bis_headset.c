/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#if (CONFIG_AUDIO_DEV == HEADSET)

#include <bluetooth/bluetooth.h>
#include <bluetooth/audio/audio.h>

#include "le_audio.h"
#include "macros_common.h"
#include "ctrl_events.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(bis_headset, CONFIG_LOG_BLE_LEVEL);

static le_audio_receive_cb receive_cb;
static bool synced_to_broadcast;

static struct bt_audio_broadcast_sink *broadcast_sink;

BUILD_ASSERT(CONFIG_BT_AUDIO_BROADCAST_SNK_STREAM_COUNT <= 1,
	     "Only one stream is currently supported");
static struct bt_audio_stream streams[CONFIG_BT_AUDIO_BROADCAST_SNK_STREAM_COUNT];

static struct bt_audio_lc3_preset lc3_preset = BT_AUDIO_LC3_BROADCAST_PRESET_48_4_1;

/* Create a mask for the maximum BIS we can sync to using the number of streams
 * we have. We add an additional 1 since the bis indexes start from 1 and not
 * 0.
 */
static const uint32_t bis_index_mask = BIT_MASK(ARRAY_SIZE(streams) + 1U);
static uint32_t bis_index_bitfield;

static void broadcast_sink_cleanup(void);

static void print_codec(const struct bt_codec *codec)
{
	if (codec->id == BT_CODEC_LC3_ID) {
		/* LC3 uses the generic LTV format - other codecs might do as well */
		uint32_t chan_allocation;

		LOG_INF("Codec config for LC3:");
		LOG_INF("\tFrequency: %d Hz", bt_codec_cfg_get_freq(codec));
		LOG_INF("\tFrame Duration: %d us", bt_codec_cfg_get_frame_duration_us(codec));
		if (bt_codec_cfg_get_chan_allocation_val(codec, &chan_allocation) == 0) {
			LOG_INF("\tChannel allocation: 0x%x", chan_allocation);
		}

		LOG_INF("\tOctets per frame: %d", bt_codec_cfg_get_octets_per_frame(codec));
		LOG_INF("\tFrames per SDU: %d", bt_codec_cfg_get_frame_blocks_per_sdu(codec, true));
	} else {
		LOG_INF("Codec is not LC3, codec_id: 0x%2x", codec->id);
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

	LOG_INF("Stream started");
}

static void stream_stopped_cb(struct bt_audio_stream *stream)
{
	int ret;
	struct event_t event;

	event.event_source = EVT_SRC_LE_AUDIO;
	event.le_audio_activity.le_audio_evt_type = LE_AUDIO_EVT_NOT_STREAMING;

	ret = ctrl_events_put(&event);
	ERR_CHK(ret);

	LOG_INF("Stream stopped");
}

static void stream_recv_cb(struct bt_audio_stream *stream, const struct bt_iso_recv_info *info,
			   struct net_buf *buf)
{
	static uint32_t recv_cnt;
	bool bad_frame = false;

	if (receive_cb == NULL) {
		LOG_ERR("The RX callback has not been set");
		return;
	}

	if (!(info->flags & BT_ISO_FLAGS_VALID)) {
		bad_frame = true;
	}

	receive_cb(buf->data, buf->len, bad_frame, info->ts);

	recv_cnt++;
	if ((recv_cnt % 1000U) == 0U) {
		LOG_INF("Received %u total ISO packets", recv_cnt);
	}
}

static struct bt_audio_stream_ops stream_ops = { .started = stream_started_cb,
						 .stopped = stream_stopped_cb,
						 .recv = stream_recv_cb };

static bool scan_recv_cb(const struct bt_le_scan_recv_info *info, uint32_t broadcast_id)
{
	LOG_INF("Broadcast source found, waiting for PA sync");

	return true;
}

static void scan_term_cb(int err)
{
	if (err) {
		LOG_ERR("Scan terminated with error: %d", err);
	}
}

static void pa_synced_cb(struct bt_audio_broadcast_sink *sink, struct bt_le_per_adv_sync *sync,
			 uint32_t broadcast_id)
{
	if (broadcast_sink != NULL) {
		LOG_ERR("Unexpected PA sync");
		return;
	}

	LOG_INF("PA synced for broadcast sink with broadcast ID 0x%06X", broadcast_id);

	broadcast_sink = sink;

	LOG_INF("Broadcast source PA synced, waiting for BASE");
}

static void pa_sync_lost_cb(struct bt_audio_broadcast_sink *sink)
{
	int ret;

	if (broadcast_sink == NULL) {
		LOG_ERR("Unexpected PA sync lost");
		return;
	}

	LOG_INF("Sink disconnected");

	/* At this point the broadcast sink is stopped and deleted */
	synced_to_broadcast = false;
	broadcast_sink = NULL;

	broadcast_sink_cleanup();

	LOG_INF("Restarting scanning for broadcast sources");

	ret = bt_audio_broadcast_sink_scan_start(BT_LE_SCAN_ACTIVE);
	ERR_CHK_MSG(ret, "Unable to start scanning for broadcast sources");
}

static void base_recv_cb(struct bt_audio_broadcast_sink *sink, const struct bt_audio_base *base)
{
	int ret;
	struct event_t event;
	uint32_t base_bis_index_bitfield = 0U;

	if (synced_to_broadcast) {
		return;
	}

	LOG_INF("Received BASE with %u subgroups from broadcast sink", base->subgroup_count);

	for (size_t i = 0U; i < base->subgroup_count; i++) {
		for (size_t j = 0U; j < base->subgroups[i].bis_count; j++) {
			const uint8_t index = base->subgroups[i].bis_data[j].index;

			base_bis_index_bitfield |= BIT(index);
			streams[i].codec = (struct bt_codec *)&base->subgroups[i].codec;
			print_codec(streams[i].codec);
			event.event_source = EVT_SRC_LE_AUDIO;
			event.le_audio_activity.le_audio_evt_type = LE_AUDIO_EVT_CONFIG_RECEIVED;

			ret = ctrl_events_put(&event);
			ERR_CHK(ret);
		}
	}

	bis_index_bitfield = base_bis_index_bitfield & bis_index_mask;

	LOG_INF("BASE received, waiting for syncable");
}

static void syncable_cb(struct bt_audio_broadcast_sink *sink, bool encrypted)
{
	int ret;

	if (synced_to_broadcast) {
		return;
	}

	if (encrypted) {
		LOG_ERR("Cannot sync to encrypted broadcast source");
		return;
	}

	LOG_INF("Syncing to broadcast");

	ret = bt_audio_broadcast_sink_sync(broadcast_sink, bis_index_bitfield, streams,
					   &lc3_preset.codec, NULL);
	ERR_CHK_MSG(ret, "Unable to sync to broadcast source");

	synced_to_broadcast = true;
}

static struct bt_audio_broadcast_sink_cb broadcast_sink_cbs = { .scan_recv = scan_recv_cb,
								.scan_term = scan_term_cb,
								.pa_synced = pa_synced_cb,
								.pa_sync_lost = pa_sync_lost_cb,
								.base_recv = base_recv_cb,
								.syncable = syncable_cb };

static void initialize(le_audio_receive_cb recv_cb)
{
	static bool initialized;

	if (!initialized) {
		receive_cb = recv_cb;

		bt_audio_broadcast_sink_register_cb(&broadcast_sink_cbs);

		for (size_t i = 0U; i < ARRAY_SIZE(streams); i++) {
			streams[i].ops = &stream_ops;
		}

		initialized = true;
	}
}

static void broadcast_sink_cleanup(void)
{
	int ret;

	if (synced_to_broadcast) {
		ret = bt_audio_broadcast_sink_stop(broadcast_sink);
		ERR_CHK_MSG(ret, "Unable to stop broadcast sink");
		synced_to_broadcast = false;
	}

	if (broadcast_sink != NULL) {
		ret = bt_audio_broadcast_sink_delete(broadcast_sink);
		ERR_CHK_MSG(ret, "Unable to delete broadcast sink");
		broadcast_sink = NULL;
	}

	bis_index_bitfield = 0U;
}

int le_audio_config_get(uint32_t *bitrate, uint32_t *sampling_rate)
{
	int frames_per_sec = 1000000 / bt_codec_cfg_get_frame_duration_us(streams[0].codec);
	int bits_per_frame = bt_codec_cfg_get_octets_per_frame(streams[0].codec) * 8;

	*sampling_rate = bt_codec_cfg_get_freq(streams[0].codec);
	*bitrate = frames_per_sec * bits_per_frame;

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
	return 0;
}

void le_audio_enable(le_audio_receive_cb recv_cb)
{
	int ret;

	initialize(recv_cb);

	broadcast_sink_cleanup();

	LOG_INF("Scanning for broadcast sources");

	ret = bt_audio_broadcast_sink_scan_start(BT_LE_SCAN_ACTIVE);
	ERR_CHK_MSG(ret, "Unable to start scanning for broadcast sources");
}

void le_audio_disable(void)
{
	broadcast_sink_cleanup();
}

#endif /* (CONFIG_AUDIO_DEV == HEADSET) */
