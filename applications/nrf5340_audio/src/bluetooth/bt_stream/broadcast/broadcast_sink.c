/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "broadcast_sink.h"

#include <zephyr/zbus/zbus.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/pacs.h>
#include <zephyr/sys/byteorder.h>

/* TODO: Remove when a get_info function is implemented in host */
#include <../subsys/bluetooth/audio/bap_endpoint.h>

#include "macros_common.h"
#include "le_audio.h"
#include "nrf5340_audio_common.h"
#include "channel_assignment.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(broadcast_sink, CONFIG_BROADCAST_SINK_LOG_LEVEL);

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
	uint32_t pd;
};

static struct bt_bap_broadcast_sink *broadcast_sink;
static struct bt_bap_stream audio_streams[CONFIG_BT_BAP_BROADCAST_SNK_STREAM_COUNT];
static struct audio_codec_info audio_codec_info[CONFIG_BT_BAP_BROADCAST_SNK_STREAM_COUNT];
static uint32_t bis_index_bitfields[CONFIG_BT_BAP_BROADCAST_SNK_STREAM_COUNT];

static struct bt_le_per_adv_sync *pa_sync_stored;

static struct active_audio_stream active_stream;

/* The values of sync_stream_cnt and active_stream_index must never become larger
 * than the sizes of the arrays above (audio_streams etc.)
 */
static uint8_t sync_stream_cnt;
static uint8_t active_stream_index;

static struct bt_audio_codec_cap codec_cap = BT_AUDIO_CODEC_CAP_LC3(
	BT_AUDIO_CODEC_CAPABILIY_FREQ,
	(BT_AUDIO_CODEC_LC3_DURATION_10 | BT_AUDIO_CODEC_LC3_DURATION_PREFER_10),
	BT_AUDIO_CODEC_LC3_CHAN_COUNT_SUPPORT(1), LE_AUDIO_SDU_SIZE_OCTETS(CONFIG_LC3_BITRATE_MIN),
	LE_AUDIO_SDU_SIZE_OCTETS(CONFIG_LC3_BITRATE_MAX), 1u, BT_AUDIO_CONTEXT_TYPE_MEDIA);

static struct bt_pacs_cap capabilities = {
	.codec_cap = &codec_cap,
};

static le_audio_receive_cb receive_cb;

static bool init_routine_completed;
static bool paused;

static int broadcast_sink_cleanup(void)
{
	int ret;

	init_routine_completed = false;

	active_stream.pd = 0;
	active_stream.stream = NULL;
	active_stream.codec = NULL;

	if (broadcast_sink != NULL) {
		ret = bt_bap_broadcast_sink_delete(broadcast_sink);
		if (ret && ret != -EALREADY) {
			return ret;
		}

		broadcast_sink = NULL;
	}

	return 0;
}

static void bis_cleanup_worker(struct k_work *work)
{
	int ret;

	ret = broadcast_sink_cleanup();
	if (ret) {
		LOG_WRN("Failed to clean up BISes: %d", ret);
	}
}

K_WORK_DEFINE(bis_cleanup_work, bis_cleanup_worker);

static void le_audio_event_publish(enum le_audio_evt_type event)
{
	int ret;
	struct le_audio_msg msg;

	if (event == LE_AUDIO_EVT_SYNC_LOST) {
		msg.pa_sync = pa_sync_stored;
		pa_sync_stored = NULL;
	}

	msg.event = event;

	ret = zbus_chan_pub(&le_audio_chan, &msg, LE_AUDIO_ZBUS_EVENT_WAIT_TIME);
	ERR_CHK(ret);
}

static void print_codec(const struct audio_codec_info *codec)
{
	if (codec->id == BT_HCI_CODING_FORMAT_LC3) {
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

static void get_codec_info(const struct bt_audio_codec_cfg *codec,
			   struct audio_codec_info *codec_info)
{
	if (codec->id == BT_HCI_CODING_FORMAT_LC3) {
		/* LC3 uses the generic LTV format - other codecs might do as well */
		LOG_DBG("Retrieve the codec configuration for LC3");
		codec_info->id = codec->id;
		codec_info->cid = codec->cid;
		codec_info->vid = codec->vid;
		codec_info->frequency = bt_audio_codec_cfg_get_freq(codec);
		codec_info->frame_duration_us = bt_audio_codec_cfg_get_frame_duration_us(codec);
		bt_audio_codec_cfg_get_chan_allocation_val(codec, &codec_info->chan_allocation);
		codec_info->octets_per_sdu = bt_audio_codec_cfg_get_octets_per_frame(codec);
		codec_info->bitrate =
			(codec_info->octets_per_sdu * 8 * 1000000) / codec_info->frame_duration_us;
		codec_info->blocks_per_sdu =
			bt_audio_codec_cfg_get_frame_blocks_per_sdu(codec, true);
	} else {
		LOG_WRN("Codec is not LC3, codec_id: 0x%2hhx", codec->id);
	}
}

static bool bitrate_check(const struct bt_audio_codec_cfg *codec)
{
	uint32_t octets_per_sdu = bt_audio_codec_cfg_get_octets_per_frame(codec);

	if (octets_per_sdu < LE_AUDIO_SDU_SIZE_OCTETS(CONFIG_LC3_BITRATE_MIN)) {
		LOG_WRN("Bitrate too low");
		return false;
	} else if (octets_per_sdu > LE_AUDIO_SDU_SIZE_OCTETS(CONFIG_LC3_BITRATE_MAX)) {
		LOG_WRN("Bitrate too high");
		return false;
	}

	return true;
}

static void stream_started_cb(struct bt_bap_stream *stream)
{
	le_audio_event_publish(LE_AUDIO_EVT_STREAMING);

	/* NOTE: The string below is used by the Nordic CI system */
	LOG_INF("Stream index %d started", active_stream_index);
	print_codec(&audio_codec_info[active_stream_index]);
}

static void stream_stopped_cb(struct bt_bap_stream *stream, uint8_t reason)
{

	switch (reason) {
	case BT_HCI_ERR_LOCALHOST_TERM_CONN:
		LOG_INF("Stream stopped by user");
		le_audio_event_publish(LE_AUDIO_EVT_NOT_STREAMING);

		break;

	case BT_HCI_ERR_CONN_FAIL_TO_ESTAB:
		/* Fall-through */
	case BT_HCI_ERR_CONN_TIMEOUT:
		LOG_INF("Stream sync lost");
		k_work_submit(&bis_cleanup_work);

		le_audio_event_publish(LE_AUDIO_EVT_SYNC_LOST);

		break;

	case BT_HCI_ERR_REMOTE_USER_TERM_CONN:
		LOG_INF("Broadcast source stopped streaming");
		le_audio_event_publish(LE_AUDIO_EVT_NOT_STREAMING);

		break;

	default:
		LOG_WRN("Unhandled reason: %d", reason);

		break;
	}

	/* NOTE: The string below is used by the Nordic CI system */
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
	.recv = stream_recv_cb,
};

static bool parse_cb(struct bt_data *data, void *codec)
{
	if (data->type == BT_AUDIO_CODEC_CONFIG_LC3_CHAN_ALLOC) {
		((struct audio_codec_info *)codec)->chan_allocation = sys_get_le32(data->data);
		return false;
	}

	return true;
}

/**
 * @brief	Function which overwrites BIS specific codec information.
 *		I.e. level 3 specific information overwrites general level 2 information.
 *
 * @note	This will change when new host APIs are available.
 *
 * @return	0 for success, error otherwise.
 */
static int bis_specific_codec_config(struct bt_bap_base_bis_data bis_data,
				     struct audio_codec_info *codec)
{
	int ret;

	ret = bt_audio_data_parse(bis_data.data, bis_data.data_len, parse_cb, codec);
	if (ret && ret != -ECANCELED) {
		LOG_WRN("Could not overwrite BIS specific codec info: %d", ret);
		return -ENXIO;
	}

	return 0;
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

			if (bitrate_check(&base->subgroups[i].codec_cfg)) {
				suitable_stream_found = true;

				bis_index_bitfields[sync_stream_cnt] = BIT(index);

				/* Get general (level 2) codec config from the subgroup */
				audio_streams[sync_stream_cnt].codec_cfg =
					(struct bt_audio_codec_cfg *)&base->subgroups[i].codec_cfg;
				get_codec_info(audio_streams[sync_stream_cnt].codec_cfg,
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
		active_stream.pd = base->pd;

		le_audio_event_publish(LE_AUDIO_EVT_CONFIG_RECEIVED);

		LOG_DBG("Channel %s active",
			((active_stream_index == AUDIO_CH_L) ? CH_L_TAG : CH_R_TAG));
		LOG_DBG("Waiting for syncable");
	} else {
		LOG_DBG("Found no suitable stream");
		le_audio_event_publish(LE_AUDIO_EVT_NO_VALID_CFG);
	}
}

static void syncable_cb(struct bt_bap_broadcast_sink *sink, bool encrypted)
{
	int ret;
	static uint8_t bis_encryption_key[BT_ISO_BROADCAST_CODE_SIZE] = {0};
	struct bt_bap_stream *audio_streams_p[] = {&audio_streams[active_stream_index]};

	LOG_DBG("Broadcast sink is syncable");

	if (IS_ENABLED(CONFIG_BT_AUDIO_BROADCAST_ENCRYPTED)) {
		memcpy(bis_encryption_key, CONFIG_BT_AUDIO_BROADCAST_ENCRYPTION_KEY,
		       MIN(strlen(CONFIG_BT_AUDIO_BROADCAST_ENCRYPTION_KEY),
			   ARRAY_SIZE(bis_encryption_key)));
	}

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

	/* NOTE: The string below is used by the Nordic CI system */
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

static struct bt_bap_broadcast_sink_cb broadcast_sink_cbs = {
	.base_recv = base_recv_cb,
	.syncable = syncable_cb,
};

int broadcast_sink_change_active_audio_stream(void)
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

int broadcast_sink_config_get(uint32_t *bitrate, uint32_t *sampling_rate, uint32_t *pres_delay)
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

		*pres_delay = active_stream.pd;
	}

	return 0;
}

int broadcast_sink_pa_sync_set(struct bt_le_per_adv_sync *pa_sync, uint32_t broadcast_id)
{
	int ret;

	if (pa_sync == NULL) {
		LOG_ERR("Invalid PA sync received");
		return -EINVAL;
	}

	LOG_DBG("Trying to set PA sync with ID: %d", broadcast_id);

	if (active_stream.stream != NULL && active_stream.stream->ep != NULL) {
		if (active_stream.stream->ep->status.state == BT_BAP_EP_STATE_STREAMING) {
			ret = bt_bap_broadcast_sink_stop(broadcast_sink);
			if (ret) {
				LOG_ERR("Failed to stop broadcast sink: %d", ret);
				return ret;
			}

			broadcast_sink_cleanup();
		}
	}

	/* If broadcast_sink was not in an active stream we still need to clean it up */
	if (broadcast_sink != NULL) {
		broadcast_sink_cleanup();
	}

	ret = bt_bap_broadcast_sink_create(pa_sync, broadcast_id, &broadcast_sink);
	if (ret) {
		LOG_WRN("Failed to create sink: %d", ret);
		return ret;
	}

	pa_sync_stored = pa_sync;

	return 0;
}

int broadcast_sink_start(void)
{
	if (!paused) {
		LOG_WRN("Already playing");
		return -EALREADY;
	}

	paused = false;
	return 0;
}

int broadcast_sink_stop(void)
{
	int ret;

	if (paused) {
		LOG_WRN("Already paused");
		return -EALREADY;
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
		return -EALREADY;
	}

	return 0;
}

int broadcast_sink_disable(void)
{
	int ret;

	if (active_stream.stream != NULL && active_stream.stream->ep != NULL) {
		if (active_stream.stream->ep->status.state == BT_BAP_EP_STATE_STREAMING) {
			ret = bt_bap_broadcast_sink_stop(broadcast_sink);
			if (ret) {
				LOG_ERR("Failed to stop sink");
			}
		}
	}

	if (pa_sync_stored != NULL) {
		ret = bt_le_per_adv_sync_delete(pa_sync_stored);
		if (ret) {
			LOG_ERR("Failed to delete pa_sync");
			return ret;
		}
	}

	ret = broadcast_sink_cleanup();
	if (ret) {
		LOG_ERR("Error cleaning up");
		return ret;
	}

	LOG_DBG("Broadcast sink disabled");

	return 0;
}

int broadcast_sink_enable(le_audio_receive_cb recv_cb)
{
	int ret;
	static bool initialized;
	enum audio_channel channel;

	if (initialized) {
		LOG_WRN("Already initialized");
		return -EALREADY;
	}

	if (recv_cb == NULL) {
		LOG_ERR("Receive callback is NULL");
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

	LOG_DBG("Broadcast sink enabled");

	return 0;
}
