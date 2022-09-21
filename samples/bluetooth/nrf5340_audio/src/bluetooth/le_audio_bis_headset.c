/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "le_audio.h"

#include <bluetooth/bluetooth.h>
#include <bluetooth/audio/audio.h>
#include <bluetooth/audio/capabilities.h>

/* TODO: Remove when a get_info function is implemented in host */
#include <../subsys/bluetooth/audio/endpoint.h>

#include "macros_common.h"
#include "ctrl_events.h"
#include "hw_codec.h"
#include "channel_assignment.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(bis_headset, CONFIG_LOG_BLE_LEVEL);

BUILD_ASSERT(CONFIG_BT_AUDIO_BROADCAST_SNK_STREAM_COUNT <= 2,
	     "A maximum of two broadcast streams are currently supported");

static struct bt_audio_broadcast_sink *broadcast_sink;

static struct bt_audio_stream streams[CONFIG_LC3_DEC_CHAN_MAX];
static struct bt_audio_stream *streams_p[ARRAY_SIZE(streams)];

/* We need to set a location as a pre-compile, this changed in initialize */
static struct bt_codec codec =
	BT_CODEC_LC3_CONFIG_48_4(BT_AUDIO_LOCATION_FRONT_LEFT, BT_AUDIO_CONTEXT_TYPE_MEDIA);

/* Create a mask for the maximum BIS we can sync to using the number of streams
 * broadcasted. We add an additional 1 since the bis indexes start from 1 and not
 * 0.
 */
static const uint32_t bis_index_mask = BIT_MASK(CONFIG_BT_AUDIO_BROADCAST_SNK_STREAM_COUNT + 1U);
static uint32_t bis_index_bitfield;

static le_audio_receive_cb receive_cb;
static bool init_routine_completed;
static bool playing_state = true;

static int bis_headset_cleanup(bool from_sync_lost_cb);

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

		uint32_t octets_per_sdu = bt_codec_cfg_get_octets_per_frame(codec);
		uint32_t bitrate =
			octets_per_sdu * 8 * (1000000 / bt_codec_cfg_get_frame_duration_us(codec));

		LOG_INF("\tOctets per frame: %d (%d kbps)", octets_per_sdu, bitrate);
		LOG_INF("\tFrames per SDU: %d", bt_codec_cfg_get_frame_blocks_per_sdu(codec, true));
	} else {
		LOG_WRN("Codec is not LC3, codec_id: 0x%2x", codec->id);
	}
}

static bool bitrate_check(const struct bt_codec *codec)
{
	uint32_t octets_per_sdu = bt_codec_cfg_get_octets_per_frame(codec);

	if (octets_per_sdu < LE_AUDIO_SDU_SIZE_OCTETS(CONFIG_LC3_BITRATE_MIN)) {
		LOG_WRN("Bitrate too low");
		return false;
	} else if (octets_per_sdu > LE_AUDIO_SDU_SIZE_OCTETS(CONFIG_LC3_BITRATE_MAX)) {
		LOG_WRN("Bitrate too high");
		return false;
	}

	return true;
}

static bool adv_data_parse(struct bt_data *data, void *user_data)
{
	if (data->type == BT_DATA_NAME_COMPLETE && data->data_len == DEVICE_NAME_PEER_LEN) {
		memcpy((char *)user_data, data->data, data->data_len);
		return false;
	}

	return true;
}

static void stream_started_cb(struct bt_audio_stream *stream)
{
	int ret;

	ret = ctrl_events_le_audio_event_send(LE_AUDIO_EVT_STREAMING);
	ERR_CHK(ret);

	LOG_INF("Stream started");
}

static void stream_stopped_cb(struct bt_audio_stream *stream)
{
	int ret;

	ret = ctrl_events_le_audio_event_send(LE_AUDIO_EVT_NOT_STREAMING);
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
		LOG_DBG("Received %u total ISO packets", recv_cnt);
	}
}

static struct bt_audio_stream_ops stream_ops = { .started = stream_started_cb,
						 .stopped = stream_stopped_cb,
						 .recv = stream_recv_cb };

static bool scan_recv_cb(const struct bt_le_scan_recv_info *info, struct net_buf_simple *ad,
			 uint32_t broadcast_id)
{
	char name[DEVICE_NAME_PEER_LEN];

	bt_data_parse(ad, adv_data_parse, (void *)name);

	if (strcmp(name, DEVICE_NAME_PEER) == 0) {
		LOG_INF("Broadcast source %s found", name);
		return true;
	}

	return false;
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

	LOG_DBG("PA synced for broadcast sink with broadcast ID 0x%06X", broadcast_id);

	broadcast_sink = sink;

	LOG_DBG("Broadcast source PA synced, waiting for BASE");
}

static void pa_sync_lost_cb(struct bt_audio_broadcast_sink *sink)
{
	int ret;

	if (broadcast_sink == NULL) {
		LOG_ERR("Unexpected PA sync lost");
		return;
	}

	LOG_DBG("Sink disconnected");

	ret = bis_headset_cleanup(true);
	if (ret) {
		LOG_ERR("Error cleaning up");
		return;
	}

	LOG_INF("Restarting scanning for broadcast sources");

	ret = bt_audio_broadcast_sink_scan_start(BT_LE_SCAN_PASSIVE);
	if (ret) {
		LOG_ERR("Unable to start scanning for broadcast sources");
	}
}

static void base_recv_cb(struct bt_audio_broadcast_sink *sink, const struct bt_audio_base *base)
{
	int ret;
	uint32_t base_bis_index_bitfield = 0U;
	enum audio_channel channel;

	if (init_routine_completed) {
		return;
	}

	/* Test to ensure there are enough streams for each subgroup */
	if (base->subgroup_count > CONFIG_BT_AUDIO_BROADCAST_SNK_STREAM_COUNT) {
		LOG_WRN("Too many channels in subgroup");
		return;
	}

	ret = channel_assignment_get(&channel);
	if (ret) {
		/* Channel is not assigned yet: use default */
		channel = AUDIO_CHANNEL_DEFAULT;
	}
	channel = BIT(channel);

	LOG_DBG("Received BASE with %u subgroup(s) from broadcast sink", base->subgroup_count);

	/* Search each subgroup for the BIS of interest */
	for (size_t i = 0U; i < base->subgroup_count; i++) {
		for (size_t j = 0U; j < base->subgroups[i].bis_count; j++) {
			const uint8_t index = base->subgroups[i].bis_data[j].index;

			LOG_DBG("BIS %u   index = %u", j, index);

			/* If this is a BIS of interest then attach to and start a stream */
			if (index == channel) {
				if (bitrate_check((struct bt_codec *)&base->subgroups[i].codec)) {
					base_bis_index_bitfield |= BIT(index);

					streams[i].codec =
						(struct bt_codec *)&base->subgroups[i].codec;
					print_codec(streams[i].codec);

					LOG_DBG("Stream %u in subgroup %u from broadcast sink", i,
						j);
				}
			}
		}
	}

	if (base_bis_index_bitfield) {
		ret = ctrl_events_le_audio_event_send(LE_AUDIO_EVT_CONFIG_RECEIVED);
		ERR_CHK(ret);
		bis_index_bitfield = base_bis_index_bitfield & bis_index_mask;

		LOG_DBG("Waiting for syncable");
	} else {
		LOG_WRN("Found no suitable streams");
	}
}

static void syncable_cb(struct bt_audio_broadcast_sink *sink, bool encrypted)
{
	int ret;

	if (streams[0].ep->status.state == BT_AUDIO_EP_STATE_STREAMING || !playing_state) {
		return;
	}

	if (encrypted) {
		LOG_ERR("Cannot sync to encrypted broadcast source");
		return;
	}

	LOG_INF("Syncing to broadcast");

	ret = bt_audio_broadcast_sink_sync(broadcast_sink, bis_index_bitfield, streams_p, NULL);
	if (ret) {
		LOG_WRN("Unable to sync to broadcast source, ret: %d", ret);
		return;
	}

	init_routine_completed = true;
}

static struct bt_audio_broadcast_sink_cb broadcast_sink_cbs = { .scan_recv = scan_recv_cb,
								.scan_term = scan_term_cb,
								.pa_synced = pa_synced_cb,
								.pa_sync_lost = pa_sync_lost_cb,
								.base_recv = base_recv_cb,
								.syncable = syncable_cb };

static struct bt_audio_capability capabilities = {
	.dir = BT_AUDIO_DIR_SINK,
	.codec = &codec,
};

static void initialize(le_audio_receive_cb recv_cb)
{
	int ret;
	static bool initialized;
	enum audio_channel channel;

	if (!initialized) {
		receive_cb = recv_cb;

		ret = channel_assignment_get(&channel);
		if (ret) {
			/* Channel is not assigned yet: use default */
			channel = AUDIO_CHANNEL_DEFAULT;
		}
		if (channel == AUDIO_CH_L) {
			ret = bt_audio_capability_set_location(BT_AUDIO_DIR_SINK,
							       BT_AUDIO_LOCATION_FRONT_LEFT);
		} else {
			ret = bt_audio_capability_set_location(BT_AUDIO_DIR_SINK,
							       BT_AUDIO_LOCATION_FRONT_RIGHT);
		}
		if (ret) {
			LOG_ERR("Location set failed");
		}

		ret = bt_audio_capability_register(&capabilities);
		if (ret) {
			LOG_ERR("Capability register failed (ret %d)", ret);
			ERR_CHK(ret);
		}

		bt_audio_broadcast_sink_register_cb(&broadcast_sink_cbs);

		for (size_t i = 0U; i < ARRAY_SIZE(streams); i++) {
			streams_p[i] = &streams[i];
			streams[i].ops = &stream_ops;
		}

		initialized = true;
	}
}

static int bis_headset_cleanup(bool from_sync_lost_cb)
{
	int ret;

	ret = bt_audio_broadcast_sink_scan_stop();
	if (ret && ret != -EALREADY) {
		return ret;
	}

	if (broadcast_sink != NULL) {
		if (!from_sync_lost_cb) {
			ret = bt_audio_broadcast_sink_stop(broadcast_sink);
			if (ret && ret != -EALREADY) {
				return ret;
			}
		}

		ret = bt_audio_broadcast_sink_delete(broadcast_sink);
		if (ret && ret != -EALREADY) {
			return ret;
		}

		broadcast_sink = NULL;
	}

	bis_index_bitfield = 0U;
	init_routine_completed = false;

	return 0;
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
	if (streams[0].ep->status.state != BT_AUDIO_EP_STATE_STREAMING) {
		return -ECANCELED;
	}

	return hw_codec_volume_increase();
}

int le_audio_volume_down(void)
{
	if (streams[0].ep->status.state != BT_AUDIO_EP_STATE_STREAMING) {
		return -ECANCELED;
	}

	return hw_codec_volume_decrease();
}

int le_audio_volume_mute(void)
{
	if (streams[0].ep->status.state != BT_AUDIO_EP_STATE_STREAMING) {
		return -ECANCELED;
	}

	return hw_codec_volume_mute();
}

int le_audio_play(void)
{
	playing_state = true;
	return 0;
}

int le_audio_pause(void)
{
	playing_state = false;
	return bt_audio_broadcast_sink_stop(broadcast_sink);
}

int le_audio_send(uint8_t const *const data, size_t size)
{
	LOG_WRN("Not possible to send audio data from broadcast sink");
	return -ENXIO;
}

int le_audio_enable(le_audio_receive_cb recv_cb)
{
	int ret;

	initialize(recv_cb);

	ret = bis_headset_cleanup(false);
	if (ret) {
		LOG_ERR("Error cleaning up");
		return ret;
	}

	LOG_INF("Scanning for broadcast sources");

	ret = bt_audio_broadcast_sink_scan_start(BT_LE_SCAN_PASSIVE);
	if (ret) {
		return ret;
	}

	LOG_DBG("LE Audio enabled");

	return 0;
}

int le_audio_disable(void)
{
	int ret;

	ret = bis_headset_cleanup(false);
	if (ret) {
		LOG_ERR("Error cleaning up");
		return ret;
	}

	LOG_DBG("LE Audio disabled");

	return 0;
}
