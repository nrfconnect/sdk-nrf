/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "le_audio.h"

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/capabilities.h>

/* TODO: Remove when a get_info function is implemented in host */
#include <../subsys/bluetooth/audio/endpoint.h>

#include "macros_common.h"
#include "ctrl_events.h"
#include "hw_codec.h"
#include "channel_assignment.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bis_headset, 4);

BUILD_ASSERT(CONFIG_BT_AUDIO_BROADCAST_SNK_STREAM_COUNT <= 2,
	     "A maximum of two broadcast streams are currently supported");

static struct bt_audio_broadcast_sink *broadcast_sink;

static struct bt_audio_stream audio_streams[CONFIG_BT_AUDIO_BROADCAST_SNK_STREAM_COUNT];
static struct bt_audio_stream *audio_streams_p[ARRAY_SIZE(audio_streams)];
static struct bt_audio_stream *active_audio_stream_p = NULL;
static unsigned int sync_stream_cnt = 0;
static unsigned int active_stream_index = ARRAY_SIZE(audio_streams);

/* We need to set a location as a pre-compile, this changed in initialize */
static struct bt_codec codec =
	BT_CODEC_LC3_CONFIG_48_4(BT_AUDIO_LOCATION_FRONT_LEFT, BT_AUDIO_CONTEXT_TYPE_MEDIA);

/* Create a mask for the maximum BIS we can sync to using the number of streams
 * broadcasted. We add an additional 1 since the bis indexes start from 1 and not
 * 0.
 */
static const uint32_t bis_index_mask = BIT_MASK(ARRAY_SIZE(audio_streams) + 1U);
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
		uint32_t bitrate = octets_per_sdu * 8 * (1000000 / bt_codec_cfg_get_frame_duration_us(codec));

		LOG_INF("\tOctets per frame: %u (%u kbps)", octets_per_sdu, bitrate);
		LOG_INF("\tFrames per SDU: %d", bt_codec_cfg_get_frame_blocks_per_sdu(codec, true));
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

	LOG_INF("Stream %u started", active_stream_index);
}

static void stream_stopped_cb(struct bt_audio_stream *stream)
{
	int ret;

	ret = ctrl_events_le_audio_event_send(LE_AUDIO_EVT_NOT_STREAMING);
	ERR_CHK(ret);

	LOG_INF("Stream %u stopped", active_stream_index);
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
				base_bis_index_bitfield |= BIT(index);

				audio_streams[sync_stream_cnt].codec =
					(struct bt_codec *)&base->subgroups[i].codec;
				print_codec(audio_streams[sync_stream_cnt].codec);

				LOG_DBG("Stream %u in subgroup %d from broadcast sink",
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

	channel_assignment_get((enum audio_channel *)&active_stream_index);
	active_audio_stream_p = audio_streams_p[active_stream_index];

	if (base_bis_index_bitfield) {
		bis_index_bitfield = base_bis_index_bitfield & bis_index_mask;

		ret = ctrl_events_le_audio_event_send(LE_AUDIO_EVT_CONFIG_RECEIVED);
		ERR_CHK(ret);

		LOG_DBG("Channel %s active from bit mask %u",
			((active_stream_index == AUDIO_CH_L) ? CH_L_TAG : CH_R_TAG), bis_index_bitfield);
		LOG_DBG("Waiting for syncable");
	} else {
		LOG_WRN("Found no suitable stream");
	}
}

static void syncable_cb(struct bt_audio_broadcast_sink *sink, bool encrypted)
{
	int ret;

	if (active_audio_stream_p->ep->status.state == BT_AUDIO_EP_STATE_STREAMING ||
	    !playing_state) {
		LOG_DBG("EP streaming or in playing_state or switching stream");
		return;
	}

	if (encrypted) {
		LOG_ERR("Cannot sync to encrypted broadcast source");
		return;
	}

	LOG_INF("Syncing to broadcast stream %u", active_stream_index);
	bis_index_bitfield = BIT(active_stream_index + 1);

	ret = bt_audio_broadcast_sink_sync(broadcast_sink, bis_index_bitfield, audio_streams_p, NULL);
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

		channel_assignment_get(&channel);

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

	bis_index_bitfield = 0U;
	init_routine_completed = false;

	return 0;
}

int le_audio_switch_sync_stream_cb(void)
{
	int ret = 0;
	unsigned int stream_index;

	playing_state = false;

	ret = ctrl_events_le_audio_event_send(LE_AUDIO_EVT_NOT_STREAMING);
	ERR_CHK(ret);

	ret = bt_audio_broadcast_sink_stop(broadcast_sink);
	if (ret) {
		LOG_WRN("Failed to pause playing");
	}

	/* NOTE: Assume in same subgroup and thus same audio decoder settings */
	ret = le_audio_get_active_stream(&stream_index);
	ERR_CHK_MSG(ret, "Failed to find an active audio stream");

	LOG_DBG("Active stream %u", stream_index);

	/* Wrap streams */
	stream_index += 1;
	if (stream_index >= le_audio_get_number_streams()) {
		stream_index = 0;
	}

	ret = le_audio_set_active_stream(stream_index);
	ERR_CHK_MSG(ret, "Failed to change to active stream");

	ret = le_audio_play();
	if (ret) {
		LOG_WRN("Failed to start playing");
	}
	
	LOG_DBG("Changed to stream %u", stream_index);

	return ret;
}

int le_audio_user_defined_button_press(le_audio_button_pressed_cb user_defined)
{
	int ret = 0;

	if (user_defined != NULL) {
		ret = user_defined();
	}

	return ret;
}

int le_audio_config_get(uint32_t *bitrate, uint32_t *sampling_rate)
{
	int frames_per_sec;
	int bits_per_frame;

	if (active_audio_stream_p == NULL) {
		return -ECANCELED;
	}

	frames_per_sec =
		1000000 / bt_codec_cfg_get_frame_duration_us(active_audio_stream_p->codec);
	bits_per_frame = bt_codec_cfg_get_octets_per_frame(active_audio_stream_p->codec) * 8;

	*sampling_rate = bt_codec_cfg_get_freq(active_audio_stream_p->codec);
	*bitrate = frames_per_sec * bits_per_frame;

	return 0;
}

int le_audio_set_active_stream(unsigned int new_active_stream_index)
{
	/* Wrap index */
	if (new_active_stream_index >= sync_stream_cnt)
		return -ECANCELED;

	LOG_DBG("New: %u    Active: %u", new_active_stream_index, active_stream_index);

	active_stream_index = new_active_stream_index;
	active_audio_stream_p = &audio_streams[active_stream_index];

	return 0;
}

int le_audio_get_active_stream(unsigned int *new_active_stream_index)
{
	*new_active_stream_index = active_stream_index;
	return 0;
}

unsigned int le_audio_get_number_streams(void)
{
	return sync_stream_cnt;
}

int le_audio_volume_up(void)
{
	if (active_audio_stream_p->ep->status.state != BT_AUDIO_EP_STATE_STREAMING) {
		return -ECANCELED;
	}

	return hw_codec_volume_increase();
}

int le_audio_volume_down(void)
{
	if (active_audio_stream_p->ep->status.state != BT_AUDIO_EP_STATE_STREAMING) {
		return -ECANCELED;
	}

	return hw_codec_volume_decrease();
}

int le_audio_volume_mute(void)
{
	if (active_audio_stream_p->ep->status.state != BT_AUDIO_EP_STATE_STREAMING) {
		return -ECANCELED;
	}

	return hw_codec_volume_mute();
}

int le_audio_play(void)
{
	playing_state = true;
	LOG_DBG("Request to play audio");
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
