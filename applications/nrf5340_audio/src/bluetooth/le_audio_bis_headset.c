/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "le_audio.h"

#include <zephyr/zbus/zbus.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/pacs.h>
#include <zephyr/sys/byteorder.h>

/* TODO: Remove when a get_info function is implemented in host */
#include <../subsys/bluetooth/audio/bap_endpoint.h>

#include "macros_common.h"
#include "nrf5340_audio_common.h"
#include "hw_codec.h"
#include "channel_assignment.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bis_headset, CONFIG_BLE_LOG_LEVEL);

BUILD_ASSERT(CONFIG_BT_BAP_BROADCAST_SNK_STREAM_COUNT <= 2,
	     "A maximum of two broadcast streams are currently supported");

ZBUS_CHAN_DEFINE(le_audio_chan, struct le_audio_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
		 ZBUS_MSG_INIT(0));

struct audio_codec_info {
	uint8_t id;
	uint16_t cid;
	uint16_t vid;
	int frequency;
	int frame_duration_us;
	enum bt_audio_location chan_allocation;
	int octets_per_sdu;
	int bitrate;
	int blocks_per_sdu;
};

struct active_audio_stream {
	struct bt_bap_stream *stream;
	struct audio_codec_info *codec;
	uint8_t brdcast_src_name_idx;
};

struct bt_name {
	char *name;
	size_t size;
};

static const char *const brdcast_src_names[] = {CONFIG_BT_AUDIO_BROADCAST_NAME,
						CONFIG_BT_AUDIO_BROADCAST_NAME_ALT};

static struct bt_bap_broadcast_sink *broadcast_sink;
static struct bt_bap_stream audio_streams[CONFIG_BT_BAP_BROADCAST_SNK_STREAM_COUNT];
static struct audio_codec_info audio_codec_info[CONFIG_BT_BAP_BROADCAST_SNK_STREAM_COUNT];
static uint32_t bis_index_bitfields[CONFIG_BT_BAP_BROADCAST_SNK_STREAM_COUNT];

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
static bool paused;

static int bis_headset_cleanup(void);

static void le_audio_event_publish(enum le_audio_evt_type event)
{
	int ret;
	struct le_audio_msg msg;

	msg.event = event;

	ret = zbus_chan_pub(&le_audio_chan, &msg, K_NO_WAIT);
	ERR_CHK(ret);
}

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

static void stream_started_cb(struct bt_bap_stream *stream)
{
	le_audio_event_publish(LE_AUDIO_EVT_STREAMING);

	LOG_INF("Stream index %d started", active_stream_index);
	print_codec(&audio_codec_info[active_stream_index]);
}

static void stream_stopped_cb(struct bt_bap_stream *stream, uint8_t reason)
{
	le_audio_event_publish(LE_AUDIO_EVT_NOT_STREAMING);

	LOG_INF("Stream index %d stopped. Reason: %d", active_stream_index, reason);
}

static void stream_recv_cb(struct bt_bap_stream *stream, const struct bt_iso_recv_info *info,
			   struct net_buf *buf)
{
	bool bad_frame = false;

	if (receive_cb == NULL) {
		LOG_ERR("The RX callback has not been set");
		return;
	}

	if (!(info->flags & BT_ISO_FLAGS_VALID)) {
		bad_frame = true;
	}

	receive_cb(buf->data, buf->len, bad_frame, info->ts, active_stream_index,
		   active_stream.codec->octets_per_sdu);
}

static struct bt_bap_stream_ops stream_ops = {
	.started = stream_started_cb,
	.stopped = stream_stopped_cb,
#if (CONFIG_BT_AUDIO_RX)
	.recv = stream_recv_cb,
#endif /* (CONFIG_BT_AUDIO_RX) */
};

static bool scan_recv_cb(const struct bt_le_scan_recv_info *info, struct net_buf_simple *ad,
			 uint32_t broadcast_id)
{
	char name[MAX(sizeof(CONFIG_BT_AUDIO_BROADCAST_NAME),
		      sizeof(CONFIG_BT_AUDIO_BROADCAST_NAME_ALT))] = {'\0'};
	struct bt_name bis_name = {&name[0], ARRAY_SIZE(name)};

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

static void pa_synced_cb(struct bt_bap_broadcast_sink *sink, struct bt_le_per_adv_sync *sync,
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

static void pa_sync_lost_cb(struct bt_bap_broadcast_sink *sink)
{
	int ret;

	LOG_DBG("Periodic advertising sync lost");

	if (broadcast_sink == NULL) {
		LOG_ERR("Unexpected PA sync lost");
		return;
	}

	if (!init_routine_completed) {
		return;
	}

	LOG_DBG("Sink disconnected");

	ret = bis_headset_cleanup();
	if (ret) {
		LOG_ERR("Error cleaning up");
		return;
	}

	LOG_INF("Restarting scanning for broadcast sources after sync lost");

	ret = bt_bap_broadcast_sink_scan_start(BT_LE_SCAN_PASSIVE);
	if (ret) {
		LOG_ERR("Unable to start scanning for broadcast sources");
	}
}

/**
 * @brief	Function which overwrites BIS specific codec information.
 *			I.e. level 3 specific information overwrites general level 2 information.
 *
 * @note: This will change when new host APIs are available.
 */
static int bis_specific_codec_config(struct bt_bap_base_bis_data bis_data,
				     struct audio_codec_info *codec)
{
	for (int i = 0; i < bis_data.data_count; i++) {
		if (bis_data.data[i].data.type == BT_CODEC_CONFIG_LC3_CHAN_ALLOC) {
			codec->chan_allocation = sys_get_le32(bis_data.data[i].data.data);
			LOG_DBG("Location has been overwritten with %d", codec->chan_allocation);
			return 0;
		}
	}

	return -ENXIO;
}

static void base_recv_cb(struct bt_bap_broadcast_sink *sink, const struct bt_bap_base *base)
{
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

			LOG_DBG("Subgroup: %d BIS: %d index = %d", i, j, index);

			if (bitrate_check((struct bt_codec *)&base->subgroups[i].codec)) {
				suitable_stream_found = true;

				bis_index_bitfields[sync_stream_cnt] = BIT(index);

				/* Get general (level 2) codec config from the subgroup */
				audio_streams[sync_stream_cnt].codec =
					(struct bt_codec *)&base->subgroups[i].codec;
				get_codec_info(audio_streams[sync_stream_cnt].codec,
					       &audio_codec_info[sync_stream_cnt]);

				/* Overwrite codec config with level 3 BIS specific codec config.
				 * For now, this is only done for channel allocation
				 */
				(void)bis_specific_codec_config(base->subgroups[i].bis_data[j],
								&audio_codec_info[sync_stream_cnt]);

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

	if (suitable_stream_found) {
		/* Set the initial active stream based on the defined channel of the device */
		channel_assignment_get((enum audio_channel *)&active_stream_index);
		active_stream.stream = &audio_streams[active_stream_index];
		active_stream.codec = &audio_codec_info[active_stream_index];

		le_audio_event_publish(LE_AUDIO_EVT_CONFIG_RECEIVED);

		LOG_DBG("Channel %s active",
			((active_stream_index == AUDIO_CH_L) ? CH_L_TAG : CH_R_TAG));
		LOG_DBG("Waiting for syncable");
	} else {
		LOG_WRN("Found no suitable stream");
	}
}

static void syncable_cb(struct bt_bap_broadcast_sink *sink, bool encrypted)
{
	int ret;
	static uint8_t bis_encryption_key[16];
	struct bt_bap_stream *audio_streams_p[] = {&audio_streams[active_stream_index]};

	LOG_DBG("Broadcast sink is syncable");

#if (CONFIG_BT_AUDIO_BROADCAST_ENCRYPTED)
	strncpy(bis_encryption_key, CONFIG_BT_AUDIO_BROADCAST_ENCRYPTION_KEY, 16);
#endif /* (CONFIG_BT_AUDIO_BROADCAST_ENCRYPTED) */

	if (active_stream.stream != NULL && active_stream.stream->ep != NULL) {
		if (active_stream.stream->ep->status.state == BT_BAP_EP_STATE_STREAMING) {
			LOG_WRN("Syncable received, but already in a stream");
			return;
		}
	}

	if (paused) {
		LOG_DBG("Syncable received, but in paused state");
		return;
	}

	LOG_INF("Syncing to broadcast stream index %d", active_stream_index);

	if (bis_index_bitfields[active_stream_index] == 0) {
		LOG_ERR("No bits set in bitfield");
		return;
	} else if (!IS_POWER_OF_TWO(bis_index_bitfields[active_stream_index])) {
		/* Check that only one bit is set */
		LOG_ERR("Application syncs to only one stream");
		return;
	}

	ret = bt_bap_broadcast_sink_sync(broadcast_sink, bis_index_bitfields[active_stream_index],
					 audio_streams_p, bis_encryption_key);

	if (ret) {
		LOG_WRN("Unable to sync to broadcast source, ret: %d", ret);
		return;
	}

	/* Only a single stream used for now */
	active_stream.stream = &audio_streams[active_stream_index];

	init_routine_completed = true;
}

static struct bt_bap_broadcast_sink_cb broadcast_sink_cbs = {.scan_recv = scan_recv_cb,
							     .scan_term = scan_term_cb,
							     .pa_synced = pa_synced_cb,
							     .pa_sync_lost = pa_sync_lost_cb,
							     .base_recv = base_recv_cb,
							     .syncable = syncable_cb};

static struct bt_pacs_cap capabilities = {
	.codec = &codec_capabilities,
};

static int initialize(le_audio_receive_cb recv_cb)
{
	int ret;
	static bool initialized;
	enum audio_channel channel;

	if (initialized) {
		LOG_WRN("Already initialized");
		return -EALREADY;
	}

	if (recv_cb == NULL) {
		LOG_ERR("Receieve callback is NULL");
		return -EINVAL;
	}

	receive_cb = recv_cb;

	channel_assignment_get(&channel);

	if (channel == AUDIO_CH_L) {
		ret = bt_pacs_set_location(BT_AUDIO_DIR_SINK, BT_AUDIO_LOCATION_FRONT_LEFT);
	} else {
		ret = bt_pacs_set_location(BT_AUDIO_DIR_SINK, BT_AUDIO_LOCATION_FRONT_RIGHT);
	}

	if (ret) {
		LOG_ERR("Location set failed");
		return ret;
	}

	ret = bt_pacs_cap_register(BT_AUDIO_DIR_SINK, &capabilities);
	if (ret) {
		LOG_ERR("Capability register failed (ret %d)", ret);
		return ret;
	}

	bt_bap_broadcast_sink_register_cb(&broadcast_sink_cbs);

	for (int i = 0; i < ARRAY_SIZE(audio_streams); i++) {
		audio_streams[i].ops = &stream_ops;
	}

	initialized = true;

	return 0;
}

static int bis_headset_cleanup(void)
{
	int ret;

	init_routine_completed = false;

	ret = bt_bap_broadcast_sink_scan_stop();
	if (ret && ret != -EALREADY) {
		LOG_WRN("Failed to stop sink scan, ret: %d", ret);
		return ret;
	}

	if (broadcast_sink != NULL) {
		ret = bt_bap_broadcast_sink_delete(broadcast_sink);
		if (ret && ret != -EALREADY) {
			return ret;
		}

		broadcast_sink = NULL;
	}

	return 0;
}

static int change_active_audio_stream(void)
{
	int ret;

	if (broadcast_sink == NULL) {
		LOG_WRN("No broadcast sink");
		return -ECANCELED;
	}

	if (active_stream.stream != NULL && active_stream.stream->ep != NULL) {
		if (active_stream.stream->ep->status.state == BT_BAP_EP_STATE_STREAMING) {
			ret = bt_bap_broadcast_sink_stop(broadcast_sink);
			if (ret) {
				LOG_ERR("Failed to stop sink");
			}
		}
	}

	/* Wrap streams */
	if (++active_stream_index >= sync_stream_cnt) {
		active_stream_index = 0;
	}

	active_stream.stream = &audio_streams[active_stream_index];
	active_stream.codec = &audio_codec_info[active_stream_index];

	LOG_INF("Changed to stream %d", active_stream_index);

	return 0;
}

static int change_active_brdcast_src(void)
{
	int ret;

	if (active_stream.stream != NULL && active_stream.stream->ep != NULL) {
		if (active_stream.stream->ep->status.state == BT_BAP_EP_STATE_STREAMING) {
			ret = bt_bap_broadcast_sink_stop(broadcast_sink);
			if (ret) {
				LOG_ERR("Failed to stop broadcast sink: %d", ret);
				return ret;
			}
		}
	}

	ret = bis_headset_cleanup();
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
	ret = bt_bap_broadcast_sink_scan_start(NRF5340_AUDIO_GATEWAY_SCAN_PARAMS);
	if (ret && ret != -EALREADY) {
		LOG_ERR("Unable to start scanning for broadcast sources");
		return ret;
	}

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

int le_audio_config_get(uint32_t *bitrate, uint32_t *sampling_rate, uint32_t *pres_delay)
{
	if (active_stream.codec == NULL) {
		LOG_WRN("No active stream to get config from");
		return -ENXIO;
	}

	if (bitrate == NULL && sampling_rate == NULL && pres_delay == NULL) {
		LOG_ERR("No valid pointers received");
		return -ENXIO;
	}

	if (sampling_rate != NULL) {
		*sampling_rate = active_stream.codec->frequency;
	}

	if (bitrate != NULL) {
		*bitrate = active_stream.codec->bitrate;
	}

	if (pres_delay != NULL) {
		if (active_stream.stream == NULL) {
			LOG_WRN("No active stream");
			return -ENXIO;
		}
		*pres_delay = active_stream.stream->qos->pd;
	}

	return 0;
}

int le_audio_volume_up(void)
{
	if (active_stream.stream == NULL || active_stream.stream->ep == NULL) {
		return -EFAULT;
	}

	if (active_stream.stream->ep->status.state != BT_BAP_EP_STATE_STREAMING) {
		return -ECANCELED;
	}

	return hw_codec_volume_increase();
}

int le_audio_volume_down(void)
{
	if (active_stream.stream == NULL || active_stream.stream->ep == NULL) {
		return -EFAULT;
	}

	if (active_stream.stream->ep->status.state != BT_BAP_EP_STATE_STREAMING) {
		return -ECANCELED;
	}

	return hw_codec_volume_decrease();
}

int le_audio_volume_mute(void)
{
	if (active_stream.stream == NULL || active_stream.stream->ep == NULL) {
		return -EFAULT;
	}

	if (active_stream.stream->ep->status.state != BT_BAP_EP_STATE_STREAMING) {
		return -ECANCELED;
	}

	return hw_codec_volume_mute();
}

int le_audio_play_pause(void)
{
	int ret;

	if (paused) {
		paused = false;
		return 0;
	}

	if (active_stream.stream == NULL || active_stream.stream->ep == NULL) {
		LOG_WRN("Stream or endpoint not set");
		return -EPERM;
	}

	if (active_stream.stream->ep->status.state == BT_BAP_EP_STATE_STREAMING) {
		paused = true;
		ret = bt_bap_broadcast_sink_stop(broadcast_sink);
		if (ret) {
			LOG_ERR("Failed to stop broadcast sink: %d", ret);
			return ret;
		}
	} else {
		LOG_WRN("Current stream not in streaming state");
	}

	return 0;
}

int le_audio_send(struct encoded_audio enc_audio)
{
	LOG_WRN("Not possible to send audio data from broadcast sink");
	return -ENXIO;
}

int le_audio_enable(le_audio_receive_cb recv_cb, le_audio_timestamp_cb timestmp_cb)
{
	int ret;

	ARG_UNUSED(timestmp_cb);

	ret = initialize(recv_cb);
	if (ret) {
		LOG_ERR("Failed to initialize");
		return ret;
	}

	ret = bis_headset_cleanup();
	if (ret) {
		LOG_ERR("Error cleaning up");
		return ret;
	}

	LOG_INF("Scanning for broadcast sources");

	ret = bt_bap_broadcast_sink_scan_start(NRF5340_AUDIO_GATEWAY_SCAN_PARAMS);
	if (ret) {
		return ret;
	}

	LOG_DBG("LE Audio enabled");

	return 0;
}

int le_audio_disable(void)
{
	int ret;

	ret = bis_headset_cleanup();
	if (ret) {
		LOG_ERR("Error cleaning up");
		return ret;
	}

	LOG_DBG("LE Audio disabled");

	return 0;
}
