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
#include "zbus_common.h"
#include "channel_assignment.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(broadcast_sink, CONFIG_BROADCAST_SINK_LOG_LEVEL);

BUILD_ASSERT(CONFIG_BT_BAP_BROADCAST_SNK_STREAM_COUNT <= 2,
	     "A maximum of two broadcast streams are currently supported");

ZBUS_CHAN_DEFINE(le_audio_chan, struct le_audio_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
		 ZBUS_MSG_INIT(0));

static uint8_t bis_encryption_key[BT_ISO_BROADCAST_CODE_SIZE] = {0};
static bool broadcast_code_received;

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
	(BT_AUDIO_CODEC_CAP_DURATION_10 | BT_AUDIO_CODEC_CAP_DURATION_PREFER_10),
	BT_AUDIO_CODEC_CAP_CHAN_COUNT_SUPPORT(1), LE_AUDIO_SDU_SIZE_OCTETS(CONFIG_LC3_BITRATE_MIN),
	LE_AUDIO_SDU_SIZE_OCTETS(CONFIG_LC3_BITRATE_MAX), 1u, BT_AUDIO_CONTEXT_TYPE_ANY);

static struct bt_pacs_cap capabilities = {
	.codec_cap = &codec_cap,
};

#define AVAILABLE_SINK_CONTEXT (BT_AUDIO_CONTEXT_TYPE_ANY)

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
	LOG_INF("Codec config for LC3:");
	LOG_INF("\tFrequency: %d Hz", codec->frequency);
	LOG_INF("\tFrame Duration: %d us", codec->frame_duration_us);
	LOG_INF("\tOctets per frame: %d (%d kbps)", codec->octets_per_sdu, codec->bitrate);
	LOG_INF("\tFrames per SDU: %d", codec->blocks_per_sdu);
	if (codec->chan_allocation >= 0) {
		LOG_INF("\tChannel allocation: 0x%x", codec->chan_allocation);
	}
}

static void get_codec_info(const struct bt_audio_codec_cfg *codec,
			   struct audio_codec_info *codec_info)
{
	int ret;

	ret = le_audio_freq_hz_get(codec, &codec_info->frequency);
	if (ret) {
		LOG_DBG("Failed retrieving sampling frequency: %d", ret);
	}

	ret = le_audio_duration_us_get(codec, &codec_info->frame_duration_us);
	if (ret) {
		LOG_DBG("Failed retrieving frame duration: %d", ret);
	}

	ret = bt_audio_codec_cfg_get_chan_allocation(codec, &codec_info->chan_allocation, false);
	if (ret == -ENODATA) {
		/* Codec channel allocation not set, defaulting to 0 */
		codec_info->chan_allocation = 0;
	} else if (ret) {
		LOG_DBG("Failed retrieving channel allocation: %d", ret);
	}

	ret = le_audio_octets_per_frame_get(codec, &codec_info->octets_per_sdu);
	if (ret) {
		LOG_DBG("Failed retrieving octets per frame: %d", ret);
	}

	ret = le_audio_bitrate_get(codec, &codec_info->bitrate);
	if (ret) {
		LOG_DBG("Failed calculating bitrate: %d", ret);
	}

	ret = le_audio_frame_blocks_per_sdu_get(codec, &codec_info->blocks_per_sdu);
	if (codec_info->octets_per_sdu < 0) {
		LOG_DBG("Failed retrieving frame blocks per SDU: %d", codec_info->octets_per_sdu);
	}
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

	case BT_HCI_ERR_TERM_DUE_TO_MIC_FAIL:
		LOG_INF("MIC fail. The encryption key may be wrong");
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

static bool base_subgroup_bis_cb(const struct bt_bap_base_subgroup_bis *bis, void *user_data)
{
	int ret;
	struct bt_audio_codec_cfg codec_cfg = {0};

	LOG_DBG("BIS found, index %d", bis->index);

	ret = bt_bap_base_subgroup_bis_codec_to_codec_cfg(bis, &codec_cfg);
	if (ret != 0) {
		LOG_WRN("Could not find codec configuration for BIS index %d, ret "
			"= %d",
			bis->index, ret);
		return true;
	}

	get_codec_info(&codec_cfg, &audio_codec_info[bis->index - 1]);

	LOG_DBG("Channel allocation: 0x%x for BIS index %d",
		audio_codec_info[bis->index - 1].chan_allocation, bis->index);

	uint32_t chan_bitfield = audio_codec_info[bis->index - 1].chan_allocation;
	bool single_bit = (chan_bitfield & (chan_bitfield - 1)) == 0;

	if (single_bit) {
		bis_index_bitfields[bis->index - 1] = BIT(bis->index - 1);
	} else {
		LOG_WRN("More than one bit set in channel location, we only support 1 channel per "
			"BIS");
	}

	return true;
}

static bool base_subgroup_cb(const struct bt_bap_base_subgroup *subgroup, void *user_data)
{
	int ret;
	int bis_num;
	struct bt_audio_codec_cfg codec_cfg = {0};
	struct bt_bap_base_codec_id codec_id;
	bool *suitable_stream_found = user_data;

	ret = bt_bap_base_subgroup_codec_to_codec_cfg(subgroup, &codec_cfg);
	if (ret) {
		LOG_WRN("Failed to convert codec to codec_cfg: %d", ret);
		return true;
	}

	ret = bt_bap_base_get_subgroup_codec_id(subgroup, &codec_id);
	if (ret && codec_id.cid != BT_HCI_CODING_FORMAT_LC3) {
		LOG_WRN("Failed to get codec ID or codec ID is not supported: %d", ret);
		return true;
	}

	ret = le_audio_bitrate_check(&codec_cfg);
	if (!ret) {
		LOG_WRN("Bitrate check failed");
		return true;
	}

	ret = le_audio_freq_check(&codec_cfg);
	if (!ret) {
		LOG_WRN("Sample rate not supported");
		return true;
	}

	bis_num = bt_bap_base_get_subgroup_bis_count(subgroup);
	LOG_DBG("Subgroup %p has %d BISes", (void *)subgroup, bis_num);
	if (bis_num > 0) {
		*suitable_stream_found = true;
		sync_stream_cnt = bis_num;
		for (int i = 0; i < bis_num; i++) {
			get_codec_info(&codec_cfg, &audio_codec_info[i]);
		}

		ret = bt_bap_base_subgroup_foreach_bis(subgroup, base_subgroup_bis_cb, NULL);
		if (ret < 0) {
			LOG_WRN("Could not get BIS for subgroup %p: %d", (void *)subgroup, ret);
		}
		return false;
	}

	return true;
}

static void base_recv_cb(struct bt_bap_broadcast_sink *sink, const struct bt_bap_base *base,
			 size_t base_size)
{
	int ret;
	bool suitable_stream_found = false;

	if (init_routine_completed) {
		return;
	}

	sync_stream_cnt = 0;

	uint32_t subgroup_count = bt_bap_base_get_subgroup_count(base);

	LOG_DBG("Received BASE with %d subgroup(s) from broadcast sink", subgroup_count);

	ret = bt_bap_base_foreach_subgroup(base, base_subgroup_cb, &suitable_stream_found);
	if (ret != 0 && ret != -ECANCELED) {
		LOG_WRN("Failed to parse subgroups: %d", ret);
		return;
	}

	if (suitable_stream_found) {
		/* Set the initial active stream based on the defined channel of the device */
		channel_assignment_get((enum audio_channel *)&active_stream_index);

		/** If the stream matching channel is not present, revert back to first BIS, e.g.
		 *  mono stream but channel assignment is RIGHT
		 */
		if ((active_stream_index + 1) > sync_stream_cnt) {
			LOG_WRN("BIS index: %d not found, reverting to first BIS",
				(active_stream_index + 1));
			active_stream_index = 0;
		}

		active_stream.stream = &audio_streams[active_stream_index];
		active_stream.codec = &audio_codec_info[active_stream_index];
		ret = bt_bap_base_get_pres_delay(base);
		if (ret == -EINVAL) {
			LOG_WRN("Failed to get pres_delay: %d", ret);
			active_stream.pd = 0;
		} else {
			active_stream.pd = ret;
		}
		le_audio_event_publish(LE_AUDIO_EVT_CONFIG_RECEIVED);

		LOG_DBG("Channel %s active",
			((active_stream_index == AUDIO_CH_L) ? CH_L_TAG : CH_R_TAG));
		LOG_DBG("Waiting for syncable");
	} else {
		LOG_DBG("Found no suitable stream");
		le_audio_event_publish(LE_AUDIO_EVT_NO_VALID_CFG);
	}
}

static void syncable_cb(struct bt_bap_broadcast_sink *sink, const struct bt_iso_biginfo *biginfo)
{
	int ret;
	struct bt_bap_stream *audio_streams_p[] = {&audio_streams[active_stream_index]};
	static uint32_t prev_broadcast_id;

	LOG_DBG("Broadcast sink is syncable");

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

	if (bis_index_bitfields[active_stream_index] == 0) {
		LOG_ERR("No bits set in bitfield");
		return;
	} else if (!IS_POWER_OF_TWO(bis_index_bitfields[active_stream_index])) {
		/* Check that only one bit is set */
		LOG_ERR("Application syncs to only one stream");
		return;
	}

	/* NOTE: The string below is used by the Nordic CI system */
	LOG_INF("Syncing to broadcast stream index %d", active_stream_index);

	if (IS_ENABLED(CONFIG_BT_AUDIO_BROADCAST_ENCRYPTED)) {
		memcpy(bis_encryption_key, CONFIG_BT_AUDIO_BROADCAST_ENCRYPTION_KEY,
		       MIN(strlen(CONFIG_BT_AUDIO_BROADCAST_ENCRYPTION_KEY),
			   ARRAY_SIZE(bis_encryption_key)));
	} else {
		/* If the biginfo shows the stream is encrypted, then wait until broadcast code is
		 * received then start to sync. If headset is out of sync but still looking for same
		 * broadcaster, then the same broadcast code can be used.
		 */
		if (!broadcast_code_received && biginfo->encryption == true &&
		    sink->broadcast_id != prev_broadcast_id) {
			LOG_WRN("Stream is encrypted, but haven not received broadcast code");
			return;
		}

		broadcast_code_received = false;
	}

	ret = bt_bap_broadcast_sink_sync(broadcast_sink, bis_index_bitfields[active_stream_index],
					 audio_streams_p, bis_encryption_key);

	if (ret) {
		LOG_WRN("Unable to sync to broadcast source, ret: %d", ret);
		return;
	}

	prev_broadcast_id = sink->broadcast_id;

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

int broadcast_sink_broadcast_code_set(uint8_t *broadcast_code)
{
	if (broadcast_code == NULL) {
		LOG_ERR("Invalid broadcast code received");
		return -EINVAL;
	}

	memcpy(bis_encryption_key, broadcast_code, BT_ISO_BROADCAST_CODE_SIZE);
	broadcast_code_received = true;

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

	ret = bt_pacs_set_supported_contexts(BT_AUDIO_DIR_SINK, AVAILABLE_SINK_CONTEXT);
	if (ret) {
		LOG_ERR("Supported context set failed. Err: %d", ret);
		return ret;
	}

	ret = bt_pacs_set_available_contexts(BT_AUDIO_DIR_SINK, AVAILABLE_SINK_CONTEXT);
	if (ret) {
		LOG_ERR("Available context set failed. Err: %d", ret);
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
