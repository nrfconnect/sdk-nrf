/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "le_audio.h"

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/pacs.h>

/* TODO: Remove when a get_info function is implemented in host */
#include <../subsys/bluetooth/audio/endpoint.h>

#include "macros_common.h"
#include "ctrl_events.h"
#include "hw_codec.h"
#include "channel_assignment.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bis_headset, CONFIG_BLE_LOG_LEVEL);

BUILD_ASSERT(CONFIG_BT_AUDIO_BROADCAST_SNK_STREAM_COUNT <= 2,
	     "A maximum of two broadcast streams are currently supported");

struct audio_codec_info {
	uint8_t id;
	uint16_t cid;
	uint16_t vid;
	int frequency;
	int frame_duration_us;
	int chan_allocation;
	int octets_per_sdu;
	int bitrate;
	int blocks_per_sdu;
};

struct active_audio_stream {
	struct bt_audio_stream *stream;
	struct audio_codec_info *codec;
	uint8_t brdcast_src_name_idx;
};

struct bt_name {
	char *name;
	size_t size;
};

static const char *const brdcast_src_names[] = { CONFIG_BT_AUDIO_BROADCAST_NAME,
						 CONFIG_BT_AUDIO_BROADCAST_NAME_ALT };

static struct bt_audio_broadcast_sink *broadcast_sink;

static struct bt_audio_stream audio_streams[CONFIG_BT_AUDIO_BROADCAST_SNK_STREAM_COUNT];
static struct bt_audio_stream *audio_streams_p[ARRAY_SIZE(audio_streams)];
static struct audio_codec_info audio_codec_info[CONFIG_BT_AUDIO_BROADCAST_SNK_STREAM_COUNT];
static uint32_t bis_index_bitfields[CONFIG_BT_AUDIO_BROADCAST_SNK_STREAM_COUNT];

static struct active_audio_stream active_stream;

/* The values of sync_stream_cnt and active_stream_index must never become larger
 * than the sizes of the arrays above (audio_streams etc.)
 */
static uint8_t sync_stream_cnt;
static uint8_t active_stream_index;

/* We need to set a location as a pre-compile, this changed in initialize */
static struct bt_codec codec_capabilities =
	BT_CODEC_LC3_CONFIG_48_4(BT_AUDIO_LOCATION_FRONT_LEFT, BT_AUDIO_CONTEXT_TYPE_MEDIA);

static le_audio_receive_cb receive_cb;
static bool init_routine_completed;
static bool playing_state = true;

static int bis_headset_cleanup(bool from_sync_lost_cb);

static void print_codec(const struct audio_codec_info *codec)
{
	if (codec->id == BT_CODEC_LC3_ID) {
		LOG_INF("Codec config for LC3:");
		LOG_INF("\tFrequency: %d Hz", codec->frequency);
		LOG_INF("\tFrame Duration: %d us", codec->frame_duration_us);
		LOG_INF("\tOctets per frame: %d (%d kbps)", codec->octets_per_sdu, codec->bitrate);
		LOG_INF("\tFrames per SDU: %d", codec->blocks_per_sdu);
		if (codec->chan_allocation >= 0) {
			LOG_INF("\tChannel allocation: 0x%x", codec->chan_allocation);
		}
	} else {
		LOG_WRN("Codec is not LC3, codec_id: 0x%2hhx", codec->id);
	}
}

static void get_codec_info(const struct bt_codec *codec, struct audio_codec_info *codec_info)
{
	if (codec->id == BT_CODEC_LC3_ID) {
		/* LC3 uses the generic LTV format - other codecs might do as well */
		LOG_DBG("Retrieve the codec configuration for LC3");
		codec_info->id = codec->id;
		codec_info->cid = codec->cid;
		codec_info->vid = codec->vid;
		codec_info->frequency = bt_codec_cfg_get_freq(codec);
		codec_info->frame_duration_us = bt_codec_cfg_get_frame_duration_us(codec);
		bt_codec_cfg_get_chan_allocation_val(codec, &codec_info->chan_allocation);
		codec_info->octets_per_sdu = bt_codec_cfg_get_octets_per_frame(codec);
		codec_info->bitrate =
			(codec_info->octets_per_sdu * 8 * 1000000) / codec_info->frame_duration_us;
		codec_info->blocks_per_sdu = bt_codec_cfg_get_frame_blocks_per_sdu(codec, true);
	} else {
		LOG_WRN("Codec is not LC3, codec_id: 0x%2hhx", codec->id);
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
	struct bt_name *bis_name = (struct bt_name *)user_data;

	if (data->type == BT_DATA_BROADCAST_NAME && data->data_len) {
		if (data->data_len <= bis_name->size) {
			memcpy(bis_name->name, data->data, data->data_len);
			return false;
		}
	}

	return true;
}

static void stream_started_cb(struct bt_audio_stream *stream)
{
	int ret;

	ret = ctrl_events_le_audio_event_send(LE_AUDIO_EVT_STREAMING);
	ERR_CHK(ret);

	LOG_INF("Stream index %d started", active_stream_index);
}

static void stream_stopped_cb(struct bt_audio_stream *stream)
{
	int ret;

	ret = ctrl_events_le_audio_event_send(LE_AUDIO_EVT_NOT_STREAMING);
	ERR_CHK(ret);

	LOG_INF("Stream index %d stopped", active_stream_index);
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

	receive_cb(buf->data, buf->len, bad_frame, info->ts, active_stream_index);

	recv_cnt++;
	if ((recv_cnt % 1000U) == 0U) {
		LOG_DBG("Received %d total ISO packets for stream %d", recv_cnt,
			active_stream_index);
	}
}

static struct bt_audio_stream_ops stream_ops = { .started = stream_started_cb,
						 .stopped = stream_stopped_cb,
						 .recv = stream_recv_cb };

static bool scan_recv_cb(const struct bt_le_scan_recv_info *info, struct net_buf_simple *ad,
			 uint32_t broadcast_id)
{
	char name[MAX(sizeof(CONFIG_BT_AUDIO_BROADCAST_NAME),
		      sizeof(CONFIG_BT_AUDIO_BROADCAST_NAME_ALT))] = { '\0' };
	struct bt_name bis_name = { &name[0], ARRAY_SIZE(name) };

	bt_data_parse(ad, adv_data_parse, (void *)&bis_name);

	if (strlen(bis_name.name) ==
	    strlen(brdcast_src_names[active_stream.brdcast_src_name_idx])) {
		if (strncmp(bis_name.name, brdcast_src_names[active_stream.brdcast_src_name_idx],
			    strlen(brdcast_src_names[active_stream.brdcast_src_name_idx])) == 0) {
			LOG_INF("Broadcast source %s found", bis_name.name);
			return true;
		}
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

	LOG_DBG("Periodic advertising sync lost");

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

	LOG_INF("Restarting scanning for broadcast sources after sync lost");

	ret = bt_audio_broadcast_sink_scan_start(BT_LE_SCAN_PASSIVE);
	if (ret) {
		LOG_ERR("Unable to start scanning for broadcast sources");
	}
}

static void base_recv_cb(struct bt_audio_broadcast_sink *sink, const struct bt_audio_base *base)
{
	int ret;
	bool suitable_stream_found = false;

	if (init_routine_completed) {
		return;
	}

	LOG_DBG("Received BASE with %zu subgroup(s) from broadcast sink", base->subgroup_count);

	sync_stream_cnt = 0;

	/* Search each subgroup for the BIS of interest */
	for (int i = 0; i < base->subgroup_count; i++) {
		for (int j = 0; j < base->subgroups[i].bis_count; j++) {
			const uint8_t index = base->subgroups[i].bis_data[j].index;

			LOG_DBG("BIS %d   index = %hhu", j, index);

			if (bitrate_check((struct bt_codec *)&base->subgroups[i].codec)) {
				suitable_stream_found = true;

				bis_index_bitfields[sync_stream_cnt] = BIT(index);

				audio_streams[sync_stream_cnt].codec =
					(struct bt_codec *)&base->subgroups[i].codec;
				get_codec_info(audio_streams[sync_stream_cnt].codec,
					       &audio_codec_info[sync_stream_cnt]);
				print_codec(&audio_codec_info[sync_stream_cnt]);

				LOG_DBG("Stream %d in subgroup %d from broadcast sink",
					sync_stream_cnt, i);

				sync_stream_cnt += 1;
				if (sync_stream_cnt >= ARRAY_SIZE(audio_streams)) {
					break;
				}
			}
		}

		if (sync_stream_cnt >= ARRAY_SIZE(audio_streams)) {
			break;
		}
	}

	/* Set the initial active stream based on the defined channel of the device */
	channel_assignment_get((enum audio_channel *)&active_stream_index);
	active_stream.stream = &audio_streams[active_stream_index];
	active_stream.codec = &audio_codec_info[active_stream_index];

	if (suitable_stream_found) {
		ret = ctrl_events_le_audio_event_send(LE_AUDIO_EVT_CONFIG_RECEIVED);
		ERR_CHK(ret);

		LOG_DBG("Channel %s active",
			((active_stream_index == AUDIO_CH_L) ? CH_L_TAG : CH_R_TAG));
		LOG_DBG("Waiting for syncable");
	} else {
		LOG_WRN("Found no suitable stream");
	}
}

static void syncable_cb(struct bt_audio_broadcast_sink *sink, bool encrypted)
{
	int ret;
	static uint8_t bis_encryption_key[16];

	LOG_DBG("Broadcast sink is syncable");

#if (CONFIG_BT_AUDIO_BROADCAST_ENCRYPTED)
	strncpy(bis_encryption_key, CONFIG_BT_AUDIO_BROADCAST_ENCRYPTION_KEY, 16);
#endif /* (CONFIG_BT_AUDIO_BROADCAST_ENCRYPTED) */

	if (active_stream.stream->ep->status.state == BT_AUDIO_EP_STATE_STREAMING ||
	    !playing_state) {
		LOG_DBG("Syncable received, but either in paused_state or already in a stream");
		return;
	}

	LOG_INF("Syncing to broadcast stream index %d", active_stream_index);

	ret = bt_audio_broadcast_sink_sync(broadcast_sink, bis_index_bitfields[active_stream_index],
					   audio_streams_p, bis_encryption_key);
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

static struct bt_pacs_cap capabilities = {
	.codec = &codec_capabilities,
};

static void initialize(le_audio_receive_cb recv_cb)
{
	int ret;
	static bool initialized;
	enum audio_channel channel;

	if (!initialized) {
		receive_cb = recv_cb;

		channel_assignment_get(&channel);

		if (channel == AUDIO_CH_L) {
			ret = bt_pacs_set_location(BT_AUDIO_DIR_SINK, BT_AUDIO_LOCATION_FRONT_LEFT);
		} else {
			ret = bt_pacs_set_location(BT_AUDIO_DIR_SINK,
						   BT_AUDIO_LOCATION_FRONT_RIGHT);
		}
		if (ret) {
			LOG_ERR("Location set failed");
		}

		ret = bt_pacs_cap_register(BT_AUDIO_DIR_SINK, &capabilities);
		if (ret) {
			LOG_ERR("Capability register failed (ret %d)", ret);
			ERR_CHK(ret);
		}

		bt_audio_broadcast_sink_register_cb(&broadcast_sink_cbs);

		for (int i = 0; i < ARRAY_SIZE(audio_streams); i++) {
			audio_streams_p[i] = &audio_streams[i];
			audio_streams[i].ops = &stream_ops;
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

	init_routine_completed = false;

	return 0;
}

static int change_active_audio_stream(void)
{
	int ret;

	if (broadcast_sink != NULL) {
		if (playing_state) {
			ret = bt_audio_broadcast_sink_stop(broadcast_sink);
			if (ret) {
				LOG_WRN("Failed to stop sink");
			}
		}

		/* Wrap streams */
		if (++active_stream_index >= sync_stream_cnt) {
			active_stream_index = 0;
		}

		active_stream.stream = &audio_streams[active_stream_index];
		active_stream.codec = &audio_codec_info[active_stream_index];

		LOG_INF("Changed to stream %d", active_stream_index);
	} else {
		LOG_WRN("No streams");
		ret = -ECANCELED;
	}

	return 0;
}

static int change_active_brdcast_src(void)
{
	int ret;

	/* If the stream has been paused do not stop the sink again */
	ret = bis_headset_cleanup(!playing_state);
	if (ret) {
		LOG_ERR("Error cleaning up");
		return ret;
	}

	/* Wrap broadcaster names */
	if (++active_stream.brdcast_src_name_idx >= ARRAY_SIZE(brdcast_src_names)) {
		active_stream.brdcast_src_name_idx = 0;
	}
	LOG_INF("Switching to %s", brdcast_src_names[active_stream.brdcast_src_name_idx]);

	LOG_DBG("Restarting scanning for broadcast sources");
	ret = bt_audio_broadcast_sink_scan_start(BT_LE_SCAN_PASSIVE);
	if (ret && ret != -EALREADY) {
		LOG_ERR("Unable to start scanning for broadcast sources");
		return ret;
	}

	/* Always start playing new source */
	playing_state = true;

	return 0;
}

int le_audio_user_defined_button_press(enum le_audio_user_defined_action action)
{
	int ret;

	if (action == LE_AUDIO_USER_DEFINED_ACTION_1) {
		LOG_DBG("User defined action 1");
		ret = change_active_audio_stream();
	} else if (action == LE_AUDIO_USER_DEFINED_ACTION_2) {
		LOG_DBG("User defined action 2");
		ret = change_active_brdcast_src();
	} else {
		LOG_WRN("User defined action not recognised (%d)", action);
		ret = -ECANCELED;
	}

	return ret;
}

int le_audio_config_get(uint32_t *bitrate, uint32_t *sampling_rate)
{
	if (active_stream.codec == NULL) {
		return -ECANCELED;
	}

	*sampling_rate = active_stream.codec->frequency;
	*bitrate = active_stream.codec->bitrate;

	return 0;
}

int le_audio_volume_up(void)
{
	if (active_stream.stream->ep->status.state != BT_AUDIO_EP_STATE_STREAMING) {
		return -ECANCELED;
	}

	return hw_codec_volume_increase();
}

int le_audio_volume_down(void)
{
	if (active_stream.stream->ep->status.state != BT_AUDIO_EP_STATE_STREAMING) {
		return -ECANCELED;
	}

	return hw_codec_volume_decrease();
}

int le_audio_volume_mute(void)
{
	if (active_stream.stream->ep->status.state != BT_AUDIO_EP_STATE_STREAMING) {
		return -ECANCELED;
	}

	return hw_codec_volume_mute();
}

int le_audio_play_pause(void)
{
	int ret;

	if (playing_state) {
		playing_state = false;

		ret = bt_audio_broadcast_sink_stop(broadcast_sink);
		if (ret) {
			LOG_ERR("Failed to stop broadcast sink: %d", ret);
			return ret;
		}
	} else {
		playing_state = true;
	}

	return 0;
}

int le_audio_send(struct encoded_audio enc_audio)
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
