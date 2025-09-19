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
#include <zephyr/bluetooth/audio/csip.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/sys/byteorder.h>

/* TODO: Remove when a get_info function is implemented in host */
#include <../subsys/bluetooth/audio/bap_endpoint.h>

#include "bt_mgmt.h"
#include "macros_common.h"
#include "zbus_common.h"
#include "device_location.h"
#include "audio_defines.h"

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
	uint32_t pd;
};

static struct bt_bap_broadcast_sink *broadcast_sink;
static struct bt_bap_stream audio_streams[CONFIG_BT_BAP_BROADCAST_SNK_STREAM_COUNT];
static struct bt_bap_stream *audio_streams_p[ARRAY_SIZE(audio_streams)];
static struct audio_codec_info audio_codec_info[CONFIG_BT_BAP_BROADCAST_SNK_STREAM_COUNT];
static uint32_t bis_index_bitfield;
static struct bt_le_per_adv_sync *pa_sync_stored;

/* The values of sync_stream_cnt and active_stream_index must never become larger
 * than the sizes of the arrays above (audio_streams etc.)
 */
static uint8_t sync_stream_cnt;

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

static struct bt_csip_set_member_svc_inst *csip;

static uint8_t flags_adv_data;
static uint8_t bass_service_uuid[BT_UUID_SIZE_16];
static uint8_t gap_appear_adv_data[BT_UUID_SIZE_16];
static uint8_t csip_rsi_adv_data[BT_CSIP_RSI_SIZE];

#define CSIP_SET_SIZE 2
enum csip_set_rank {
	CSIP_HL_RANK = 1,
	CSIP_HR_RANK = 2
};

static int stream_index_get(const struct bt_bap_stream *stream)
{
	for (int i = 0; i < ARRAY_SIZE(audio_streams); i++) {
		if (stream == &audio_streams[i]) {
			return i;
		}
	}

	return -ENODEV;
}

/* Callback for locking state change from server side */
static void csip_lock_changed_cb(struct bt_conn *conn, struct bt_csip_set_member_svc_inst *csip,
				 bool locked)
{
	LOG_DBG("Client %p %s the lock", (void *)conn, locked ? "locked" : "released");
}

/* Callback for SIRK read request from peer side */
static uint8_t sirk_read_req_cb(struct bt_conn *conn, struct bt_csip_set_member_svc_inst *csip)
{
	/* Accept the request to read the SIRK, but return encrypted SIRK instead of plaintext */
	return BT_CSIP_READ_SIRK_REQ_RSP_ACCEPT_ENC;
}

static struct bt_csip_set_member_cb csip_callbacks = {
	.lock_changed = csip_lock_changed_cb,
	.sirk_read_req = sirk_read_req_cb,
};

struct bt_csip_set_member_register_param csip_param = {
	.set_size = CSIP_SET_SIZE,
	.lockable = true,
	.cb = &csip_callbacks,
};

int broadcast_sink_uuid_populate(struct net_buf_simple *uuid_buf)
{
	if (net_buf_simple_tailroom(uuid_buf) >= (BT_UUID_SIZE_16 * 3)) {
		net_buf_simple_add_le16(uuid_buf, BT_UUID_BASS_VAL);
		net_buf_simple_add_le16(uuid_buf, BT_UUID_PACS_VAL);
	} else {
		LOG_ERR("Not enough space for UUIDS");
		return -ENOMEM;
	}

	return 0;
}

int broadcast_sink_adv_populate(struct bt_data *adv_buf, uint8_t adv_buf_vacant)
{
	int ret;
	uint32_t adv_buf_cnt = 0;

	if (IS_ENABLED(CONFIG_BT_CSIP_SET_MEMBER)) {
		ret = bt_mgmt_adv_buffer_put(adv_buf, &adv_buf_cnt, adv_buf_vacant,
					     sizeof(csip_rsi_adv_data), BT_DATA_CSIS_RSI,
					     (void *)csip_rsi_adv_data);
		if (ret) {
			return ret;
		}
	}

	/*
	 * AD format required for broadcast sink with scan delegator.
	 * Details can be found in Basic Audio Profile Section 3.9.2.
	 */
	sys_put_le16(BT_UUID_BASS_VAL, &bass_service_uuid[0]);

	ret = bt_mgmt_adv_buffer_put(adv_buf, &adv_buf_cnt, adv_buf_vacant,
				     sizeof(bass_service_uuid), BT_DATA_SVC_DATA16,
				     (void *)bass_service_uuid);
	if (ret) {
		return ret;
	}

	sys_put_le16(CONFIG_BT_DEVICE_APPEARANCE, &gap_appear_adv_data[0]);

	ret = bt_mgmt_adv_buffer_put(adv_buf, &adv_buf_cnt, adv_buf_vacant,
				     sizeof(gap_appear_adv_data), BT_DATA_GAP_APPEARANCE,
				     (void *)gap_appear_adv_data);
	if (ret) {
		return ret;
	}

	flags_adv_data = BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR;

	ret = bt_mgmt_adv_buffer_put(adv_buf, &adv_buf_cnt, adv_buf_vacant, sizeof(uint8_t),
				     BT_DATA_FLAGS, (void *)&flags_adv_data);
	if (ret) {
		return ret;
	}

	return adv_buf_cnt;
}

static int broadcast_sink_cleanup(void)
{
	int ret;

	init_routine_completed = false;

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
	sync_stream_cnt++;

	/* NOTE: The string below is used by the Nordic CI system */
	LOG_INF("Stream index %d started", stream_index_get(stream));
	print_codec(&audio_codec_info[stream_index_get(stream)]);
}

static void stream_stopped_cb(struct bt_bap_stream *stream, uint8_t reason)
{
	if (sync_stream_cnt == 0) {
		LOG_WRN("Stream stopped, but no streams are currently synced");
		return;
	}
	sync_stream_cnt--;

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
	LOG_INF("Stream index %d stopped. Reason: %d", stream_index_get(stream), reason);
}

static void stream_recv_cb(struct bt_bap_stream *stream, const struct bt_iso_recv_info *info,
			   struct net_buf *audio_frame)
{
	int ret;
	struct audio_metadata meta;

	if (receive_cb == NULL) {
		LOG_ERR("The RX callback has not been set");
		return;
	}

	ret = le_audio_metadata_populate(&meta, stream, info, audio_frame);
	if (ret) {
		LOG_ERR("Failed to populate meta data: %d", ret);
		return;
	}

	receive_cb(audio_frame, &meta, stream_index_get(stream));
}

static struct bt_bap_stream_ops stream_ops = {
	.started = stream_started_cb,
	.stopped = stream_stopped_cb,
	.recv = stream_recv_cb,
};

/**
 * @brief	Parse a BIS in the context of a base subgroup.
 *
 * @param bis       Pointer to the BIS to parse.
 * @param user_data Pointer to user data (not used).
 *
 * @return true if parsing should continue, false otherwise.
 */
static bool bis_per_subgroup_parse(const struct bt_bap_base_subgroup_bis *bis, void *user_data)
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
	enum bt_audio_location device_location_temp;

	device_location_get(&device_location_temp);

	if (!(audio_codec_info[bis->index - 1].chan_allocation & device_location_temp)) {
		LOG_DBG("BIS idx %d channel alloc. 0x%x does not match this device' location 0x%x",
			bis->index, audio_codec_info[bis->index - 1].chan_allocation,
			device_location_temp);
		return true;
	}

	if (POPCOUNT(bis_index_bitfield) >= CONFIG_BT_AUDIO_CONCURRENT_RX_STREAMS_MAX) {
		LOG_DBG("Maximum number of BISes reached, ignoring BIS index %d", bis->index);
		return true;
	}

	bis_index_bitfield |= BIT(bis->index - 1);
	LOG_DBG("BIS index %d added to bitfield 0x%08x", bis->index, bis_index_bitfield);

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
		for (int i = 0; i < bis_num; i++) {
			get_codec_info(&codec_cfg, &audio_codec_info[i]);
			audio_codec_info[i].id = codec_id.id;
			audio_codec_info[i].cid = codec_id.cid;
			audio_codec_info[i].vid = codec_id.vid;
		}

		ret = bt_bap_base_subgroup_foreach_bis(subgroup, bis_per_subgroup_parse, NULL);
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
		ret = bt_bap_base_get_pres_delay(base);
		if (ret == -EINVAL) {
			LOG_WRN("Failed to get pres_delay: %d", ret);
			/* Since all BISes in a subgroup share the same codec info,
			 * we can use index 0.
			 */
			audio_codec_info[0].pd = 0;
		} else {
			audio_codec_info[0].pd = ret;
		}
		le_audio_event_publish(LE_AUDIO_EVT_CONFIG_RECEIVED);

		LOG_DBG("Waiting for syncable");
	} else {
		LOG_DBG("Found no suitable stream");
		le_audio_event_publish(LE_AUDIO_EVT_NO_VALID_CFG);
	}
}

static void syncable_cb(struct bt_bap_broadcast_sink *sink, const struct bt_iso_biginfo *biginfo)
{
	int ret;
	static uint32_t prev_broadcast_id;

	LOG_DBG("Broadcast sink is syncable");

	if (paused) {
		LOG_DBG("Syncable received, but in paused state");
		return;
	}

	if (bis_index_bitfield == 0) {
		LOG_ERR("No bits set in bitfield");
		return;
	}

	/* NOTE: The string below is used by the Nordic CI system */
	LOG_INF("Syncing to broadcast stream index 0x%04x (bitfield)", bis_index_bitfield);

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
			LOG_WRN("Stream is encrypted, but have not received broadcast code");
			return;
		}

		broadcast_code_received = false;
	}

	ret = bt_bap_broadcast_sink_sync(broadcast_sink, bis_index_bitfield, audio_streams_p,
					 bis_encryption_key);

	if (ret) {
		LOG_WRN("Unable to sync to broadcast source, ret: %d", ret);
		return;
	}

	prev_broadcast_id = sink->broadcast_id;

	init_routine_completed = true;
}

static bool is_any_active_streams(void)
{
	for (int i = 0; i < ARRAY_SIZE(audio_streams); i++) {
		if (audio_streams[i].ep != NULL &&
		    audio_streams[i].ep->state == BT_BAP_EP_STATE_STREAMING) {
			return true;
		}
	}

	return false;
}

static struct bt_bap_broadcast_sink_cb broadcast_sink_cbs = {
	.base_recv = base_recv_cb,
	.syncable = syncable_cb,
};

int le_audio_concurrent_sync_num_get(void)
{
	return sync_stream_cnt;
}

int broadcast_sink_config_get(uint32_t *bitrate, uint32_t *sampling_rate, uint32_t *pres_delay)
{
	if (bitrate == NULL && sampling_rate == NULL && pres_delay == NULL) {
		LOG_ERR("No valid pointers received");
		return -ENXIO;
	}

	/* Since all BISes in a subgroup share the same codec info, we can use index 0 */
	if (sampling_rate != NULL) {
		*sampling_rate = audio_codec_info[0].frequency;
	}

	if (bitrate != NULL) {
		*bitrate = audio_codec_info[0].bitrate;
	}

	if (pres_delay != NULL) {

		*pres_delay = audio_codec_info[0].pd;
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

	if (is_any_active_streams()) {
		LOG_DBG("There are active streams, stopping them before setting new PA sync");

		ret = bt_bap_broadcast_sink_stop(broadcast_sink);
		if (ret) {
			LOG_ERR("Failed to stop broadcast sink: %d", ret);
			return ret;
		}

		broadcast_sink_cleanup();
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

	if (is_any_active_streams() == false) {
		LOG_WRN("No active streams to stop");
		return -EALREADY;
	}

	paused = true;
	ret = bt_bap_broadcast_sink_stop(broadcast_sink);
	if (ret) {
		LOG_ERR("Failed to stop broadcast sink: %d", ret);
		return ret;
	}

	return 0;
}

int broadcast_sink_disable(void)
{
	int ret;

	if (is_any_active_streams()) {
		ret = bt_bap_broadcast_sink_stop(broadcast_sink);
		if (ret) {
			LOG_ERR("Failed to stop sink");
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
	enum bt_audio_location device_location;
	const struct bt_pacs_register_param pacs_param = {
		.snk_pac = true,
		.snk_loc = true,
	};

	if (initialized) {
		LOG_WRN("Already initialized");
		return -EALREADY;
	}

	if (recv_cb == NULL) {
		LOG_ERR("Receive callback is NULL");
		return -EINVAL;
	}

	receive_cb = recv_cb;

	device_location_get(&device_location);

	ret = bt_pacs_register(&pacs_param);
	if (ret) {
		LOG_ERR("Could not register PACS (err %d)\n", ret);
		return ret;
	}

	ret = bt_pacs_set_location(BT_AUDIO_DIR_SINK, device_location);
	if (ret) {
		LOG_ERR("Location set failed");
		return ret;
	}

	/*
	 * Set RANK. Use 1 for left, 2 for right, 1 otherwise,
	 * as rank will have to be considered depending on the application.
	 */
	if (device_location == BT_AUDIO_LOCATION_FRONT_LEFT) {
		csip_param.rank = CSIP_HL_RANK;
	} else if (device_location == BT_AUDIO_LOCATION_FRONT_RIGHT) {
		csip_param.rank = CSIP_HR_RANK;
	} else {
		csip_param.rank = CSIP_HL_RANK;
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

	if (IS_ENABLED(CONFIG_BT_AUDIO_SCAN_DELEGATOR)) {
		if (IS_ENABLED(CONFIG_BT_CSIP_SET_MEMBER_TEST_SAMPLE_DATA)) {
			LOG_WRN("CSIP test sample data is used, must be changed "
				"before production");
		} else {
			if (strcmp(CONFIG_BT_SET_IDENTITY_RESOLVING_KEY_DEFAULT,
				   CONFIG_BT_SET_IDENTITY_RESOLVING_KEY) == 0) {
				LOG_WRN("CSIP using the default SIRK, must be changed "
					"before production");
			}
			memcpy(csip_param.sirk, CONFIG_BT_SET_IDENTITY_RESOLVING_KEY,
			       BT_CSIP_SIRK_SIZE);
		}

		ret = bt_cap_acceptor_register(&csip_param, &csip);
		if (ret) {
			LOG_ERR("Failed to register CAP acceptor. Err: %d", ret);
			return ret;
		}

		ret = bt_csip_set_member_generate_rsi(csip, csip_rsi_adv_data);
		if (ret) {
			LOG_ERR("Failed to generate RSI. Err: %d", ret);
			return ret;
		}
	}

	bt_bap_broadcast_sink_register_cb(&broadcast_sink_cbs);

	for (int i = 0; i < ARRAY_SIZE(audio_streams); i++) {
		audio_streams[i].ops = &stream_ops;
		audio_streams_p[i] = &audio_streams[i];
	}

	initialized = true;

	LOG_DBG("Broadcast sink enabled");

	return 0;
}
