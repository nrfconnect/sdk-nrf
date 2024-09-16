/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "broadcast_source.h"

#include <zephyr/zbus/zbus.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/pbp.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>

#include "bt_mgmt.h"
#include "macros_common.h"
#include "bt_le_audio_tx.h"
#include "le_audio.h"
#include "zbus_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(broadcast_source, CONFIG_BROADCAST_SOURCE_LOG_LEVEL);

/* Length-type-value size for channel allocation */
#define LTV_CHAN_ALLOC_SIZE 6

#if (CONFIG_AURACAST)
/* Index values into the PBA advertising data format.
 *
 * See Table 4.1 of the Public Broadcast Profile, Bluetooth® Profile Specification, v1.0.
 */
#define PBA_UUID_INDEX		 (0)
#define PBA_FEATURES_INDEX	 (2)
#define PBA_METADATA_SIZE_INDEX	 (3)
#define PBA_METADATA_START_INDEX (4)
#endif /* CONFIG_AURACAST */

ZBUS_CHAN_DEFINE(le_audio_chan, struct le_audio_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
		 ZBUS_MSG_INIT(0));

static struct bt_cap_broadcast_source *broadcast_sources[CONFIG_BT_ISO_MAX_BIG];
struct bt_cap_initiator_broadcast_create_param create_param[CONFIG_BT_ISO_MAX_BIG];
/* Make sure we have statically allocated streams for all potential BISes */
static struct bt_cap_stream cap_streams[CONFIG_BT_ISO_MAX_BIG]
				       [CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT]
				       [CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT];

static struct bt_bap_lc3_preset lc3_preset = BT_BAP_LC3_BROADCAST_PRESET_NRF5340_AUDIO;

static bool initialized;
static bool delete_broadcast_src[CONFIG_BT_ISO_MAX_BIG];

static int metadata_u8_add(uint8_t buffer[], uint8_t *index, uint8_t type, uint8_t value)
{
	if (buffer == NULL || index == NULL) {
		return -EINVAL;
	}

	/* Add length of type and value */
	buffer[(*index)++] = (sizeof(type) + sizeof(uint8_t));
	buffer[(*index)++] = type;
	buffer[(*index)++] = value;

	return 0;
}

static void le_audio_event_publish(enum le_audio_evt_type event, const struct stream_index *idx)
{
	int ret;
	struct le_audio_msg msg;

	msg.event = event;
	msg.idx = *idx;

	ret = zbus_chan_pub(&le_audio_chan, &msg, LE_AUDIO_ZBUS_EVENT_WAIT_TIME);
	ERR_CHK(ret);
}

static int stream_index_get(struct bt_bap_stream *stream, struct stream_index *idx)
{
	for (int i = 0; i < CONFIG_BT_ISO_MAX_BIG; i++) {
		for (int j = 0; j < CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT; j++) {
			for (int k = 0; k < ARRAY_SIZE(cap_streams[i][j]); k++) {
				if (&cap_streams[i][j][k].bap_stream == stream) {
					idx->lvl1 = i;
					idx->lvl2 = j;
					idx->lvl3 = k;
					return 0;
				}
			}
		}
	}

	LOG_WRN("Stream %p not found", (void *)stream);

	return -EINVAL;
}

static void stream_sent_cb(struct bt_bap_stream *stream)
{
	int ret;
	struct stream_index idx;

	ret = stream_index_get(stream, &idx);
	if (ret) {
		return;
	}

	if (IS_ENABLED(CONFIG_BT_AUDIO_BROADCAST_ZBUS_EVT_STREAM_SENT)) {
		le_audio_event_publish(LE_AUDIO_EVT_STREAM_SENT, &idx);
	}

	ERR_CHK(bt_le_audio_tx_stream_sent(idx));
}

static void stream_started_cb(struct bt_bap_stream *stream)
{
	int ret;
	struct stream_index idx;

	ret = stream_index_get(stream, &idx);
	if (ret) {
		return;
	}

	ERR_CHK(bt_le_audio_tx_stream_started(idx));

	le_audio_event_publish(LE_AUDIO_EVT_STREAMING, &idx);

	/* NOTE: The string below is used by the Nordic CI system */
	LOG_INF("Broadcast source %p started", (void *)stream);

	le_audio_print_codec(stream->codec_cfg, BT_AUDIO_DIR_SOURCE);
}

static void stream_stopped_cb(struct bt_bap_stream *stream, uint8_t reason)
{
	int ret;
	struct stream_index idx;

	ret = stream_index_get(stream, &idx);
	if (ret) {
		return;
	}

	le_audio_event_publish(LE_AUDIO_EVT_NOT_STREAMING, &idx);

	LOG_INF("Broadcast source %p stopped. Reason: %d", (void *)stream, reason);

	if (delete_broadcast_src[idx.lvl1] && broadcast_sources[idx.lvl1] != NULL) {
		ret = bt_cap_initiator_broadcast_audio_delete(broadcast_sources[idx.lvl1]);
		if (ret) {
			LOG_ERR("Unable to delete broadcast source %p", (void *)stream);
			delete_broadcast_src[idx.lvl1] = false;
			return;
		}

		broadcast_sources[idx.lvl1] = NULL;

		LOG_INF("Broadcast source %p deleted", (void *)stream);

		delete_broadcast_src[idx.lvl1] = false;
	}
}

static struct bt_bap_stream_ops stream_ops = {
	.sent = stream_sent_cb,
	.started = stream_started_cb,
	.stopped = stream_stopped_cb,
};

#if (CONFIG_AURACAST)
static void public_broadcast_features_set(uint8_t *features, uint8_t big_index)
{
	if (features == NULL) {
		LOG_ERR("No pointer to features");
		return;
	}

	if (big_index >= ARRAY_SIZE(create_param)) {
		LOG_ERR("BIG index %d out of range", big_index);
		return;
	}

	if (create_param[big_index].encryption) {
		*features |= BT_PBP_ANNOUNCEMENT_FEATURE_ENCRYPTION;
	}

	for (uint8_t i = 0; i < create_param->subgroup_count; i++) {
		int freq = bt_audio_codec_cfg_get_freq(
			create_param[big_index].subgroup_params[i].codec_cfg);

		if (freq < 0) {
			LOG_ERR("Unable to get frequency");
			continue;
		}

		if (freq == BT_AUDIO_CODEC_CFG_FREQ_16KHZ ||
		    freq == BT_AUDIO_CODEC_CFG_FREQ_24KHZ) {
			*features |= BT_PBP_ANNOUNCEMENT_FEATURE_STANDARD_QUALITY;
		} else if (freq == BT_AUDIO_CODEC_CFG_FREQ_48KHZ) {
			*features |= BT_PBP_ANNOUNCEMENT_FEATURE_HIGH_QUALITY;
		} else {
			LOG_WRN("%dkHz is not compatible with Auracast, choose 16kHz, 24kHz or "
				"48kHz",
				freq);
		}
	}
}
#endif /* (CONFIG_AURACAST) */

int broadcast_source_ext_adv_populate(uint8_t big_index, bool fixed_id, uint32_t broadcast_id,
				      struct broadcast_source_ext_adv_data *ext_adv_data,
				      struct bt_data *ext_adv_buf, size_t ext_adv_buf_vacant)
{
	int ret;
	uint32_t ext_adv_buf_cnt = 0;

	if (big_index >= CONFIG_BT_ISO_MAX_BIG) {
		LOG_ERR("Trying to populate ext adv for BIG %d out of %d", big_index,
			CONFIG_BT_ISO_MAX_BIG);
		return -EINVAL;
	}

	if (ext_adv_data == NULL || ext_adv_buf == NULL || ext_adv_buf_vacant == 0) {
		LOG_ERR("Advertising populate failed.");
		return -EINVAL;
	}

	size_t brdcast_name_size = strlen(ext_adv_data->brdcst_name_buf);

	ret = bt_mgmt_adv_buffer_put(ext_adv_buf, &ext_adv_buf_cnt, ext_adv_buf_vacant,
				     brdcast_name_size, BT_DATA_BROADCAST_NAME,
				     (void *)ext_adv_data->brdcst_name_buf);
	if (ret) {
		return ret;
	}

	if (!fixed_id) {
		/* Use a random broadcast ID */
		ret = bt_cap_initiator_broadcast_get_id(broadcast_sources[big_index],
							&broadcast_id);
		if (ret) {
			LOG_ERR("Unable to get broadcast ID: %d", ret);
			return ret;
		}
	}

	sys_put_le16(BT_UUID_BROADCAST_AUDIO_VAL, ext_adv_data->brdcst_id_buf);

	sys_put_le24(broadcast_id, &ext_adv_data->brdcst_id_buf[BROADCAST_SOURCE_ADV_ID_START]);

	ret = bt_mgmt_adv_buffer_put(ext_adv_buf, &ext_adv_buf_cnt, ext_adv_buf_vacant,
				     sizeof(ext_adv_data->brdcst_id_buf), BT_DATA_SVC_DATA16,
				     (void *)ext_adv_data->brdcst_id_buf);
	if (ret) {
		return ret;
	}

	sys_put_le16(CONFIG_BT_DEVICE_APPEARANCE, ext_adv_data->brdcst_appearance_buf);

	ret = bt_mgmt_adv_buffer_put(ext_adv_buf, &ext_adv_buf_cnt, ext_adv_buf_vacant,
				     sizeof(ext_adv_data->brdcst_appearance_buf),
				     BT_DATA_GAP_APPEARANCE,
				     (void *)ext_adv_data->brdcst_appearance_buf);
	if (ret) {
		return ret;
	}

#if (CONFIG_AURACAST)
	uint8_t meta_data_buf_size = 0;

	sys_put_le16(BT_UUID_PBA_VAL, &ext_adv_data->pba_buf[PBA_UUID_INDEX]);
	public_broadcast_features_set(&ext_adv_data->pba_buf[PBA_FEATURES_INDEX], big_index);

	/* Metadata */
	/* Parental rating */
	ret = metadata_u8_add(&ext_adv_data->pba_buf[PBA_METADATA_START_INDEX], &meta_data_buf_size,
			      BT_AUDIO_METADATA_TYPE_PARENTAL_RATING,
			      CONFIG_BT_AUDIO_BROADCAST_PARENTAL_RATING);
	if (ret) {
		return ret;
	}

	/* Active flag */
	ret = metadata_u8_add(&ext_adv_data->pba_buf[PBA_METADATA_START_INDEX], &meta_data_buf_size,
			      BT_AUDIO_METADATA_TYPE_AUDIO_STATE, BT_AUDIO_ACTIVE_STATE_ENABLED);
	if (ret) {
		return ret;
	}

	/* Metadata size */
	ext_adv_data->pba_buf[PBA_METADATA_SIZE_INDEX] = meta_data_buf_size;

	/* Add PBA buffer to extended advertising data */
	ret = bt_mgmt_adv_buffer_put(ext_adv_buf, &ext_adv_buf_cnt, ext_adv_buf_vacant,
				     BROADCAST_SOURCE_PBA_HEADER_SIZE +
					     ext_adv_data->pba_buf[PBA_METADATA_SIZE_INDEX],
				     BT_DATA_SVC_DATA16, (void *)ext_adv_data->pba_buf);
	if (ret) {
		return ret;
	}

#endif /* (CONFIG_AURACAST) */

	return ext_adv_buf_cnt;
}

int broadcast_source_per_adv_populate(uint8_t big_index,
				      struct broadcast_source_per_adv_data *per_adv_data,
				      struct bt_data *per_adv_buf, size_t per_adv_buf_vacant)
{
	int ret;
	size_t per_adv_buf_cnt = 0;

	if (big_index >= CONFIG_BT_ISO_MAX_BIG) {
		LOG_ERR("Trying to populate per adv for BIG %d out of %d", big_index,
			CONFIG_BT_ISO_MAX_BIG);
		return -EINVAL;
	}

	if (per_adv_data == NULL || per_adv_buf == NULL || per_adv_buf_vacant == 0) {
		LOG_ERR("Periodic advertising populate failed.");
		return -EINVAL;
	}

	/* Setup periodic advertising data */
	ret = bt_cap_initiator_broadcast_get_base(broadcast_sources[big_index],
						  per_adv_data->base_buf);
	if (ret) {
		LOG_ERR("Failed to get encoded BASE: %d", ret);
		return ret;
	}

	ret = bt_mgmt_adv_buffer_put(per_adv_buf, &per_adv_buf_cnt, per_adv_buf_vacant,
				     per_adv_data->base_buf->len, BT_DATA_SVC_DATA16,
				     (void *)per_adv_data->base_buf->data);
	if (ret) {
		return ret;
	}

	return per_adv_buf_cnt;
}

/**
 * @brief Set the channel allocation to a preset codec configuration.
 *
 * @param	data		The preset codec configuration.
 * @param	data_len	Length of @p data
 * @param	loc		Location bitmask setting.
 */
static void bt_audio_codec_allocation_set(uint8_t *data, uint8_t data_len,
					  enum bt_audio_location loc)
{
	data[0] = data_len - 1;
	data[1] = BT_AUDIO_CODEC_CFG_CHAN_ALLOC;
	sys_put_le32((const uint32_t)loc, &data[2]);
}

static int create_param_produce(uint8_t big_index,
				struct broadcast_source_big const *const ext_create_param,
				struct bt_cap_initiator_broadcast_create_param *create_param)
{
	int ret;

	if (big_index >= CONFIG_BT_ISO_MAX_BIG) {
		LOG_ERR("Trying to create param for BIG %d out of %d", big_index,
			CONFIG_BT_ISO_MAX_BIG);
		return -EINVAL;
	}

	if (ext_create_param->num_subgroups > CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT) {
		LOG_ERR("Trying to create %d subgroups, but only allocated memory for %d",
			ext_create_param->num_subgroups,
			CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT);
		return -EINVAL;
	}

	uint8_t total_num_bis = 0;

	for (size_t i = 0U; i < ext_create_param->num_subgroups; i++) {
		for (size_t j = 0; j < ext_create_param->subgroups[i].num_bises; j++) {
			total_num_bis++;
		}
	}

	if (total_num_bis > CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT) {
		LOG_ERR("Trying to set up %d BISes in total, but only allocated memory for %d",
			total_num_bis, CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT);
		return -EINVAL;
	}

	static struct bt_cap_initiator_broadcast_stream_param
		stream_params[CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT][2];
	static uint8_t bis_codec_data[CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT][2]
				     [LTV_CHAN_ALLOC_SIZE];
	static struct bt_cap_initiator_broadcast_subgroup_param
		subgroup_params[CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT];

	(void)memset(cap_streams[big_index], 0, sizeof(cap_streams[big_index]));

	for (size_t i = 0U; i < ext_create_param->num_subgroups; i++) {
		enum bt_audio_location subgroup_loc = 0;

		for (size_t j = 0; j < ext_create_param->subgroups[i].num_bises; j++) {
			stream_params[i][j].stream = &cap_streams[big_index][i][j];

			stream_params[i][j].data_len = ARRAY_SIZE(bis_codec_data[i][j]);
			stream_params[i][j].data = bis_codec_data[i][j];

			enum bt_audio_location loc = ext_create_param->subgroups[i].location[j];

			subgroup_loc |= loc;
			bt_audio_codec_allocation_set(stream_params[i][j].data,
						      stream_params[i][j].data_len, loc);
		}

		subgroup_params[i].stream_count = ext_create_param->subgroups[i].num_bises;
		subgroup_params[i].stream_params = stream_params[i];
		subgroup_params[i].codec_cfg =
			&ext_create_param->subgroups[i].group_lc3_preset.codec_cfg;
		ret = bt_audio_codec_cfg_set_chan_allocation(subgroup_params[i].codec_cfg,
							     subgroup_loc);
		if (ret < 0) {
			LOG_WRN("Failed to set location: %d", ret);
			return -EINVAL;
		}

		ret = bt_audio_codec_cfg_meta_set_stream_context(
			subgroup_params[i].codec_cfg, ext_create_param->subgroups[i].context);
		if (ret < 0) {
			LOG_WRN("Failed to set context: %d", ret);
			return -EINVAL;
		}
	}

	/* Create broadcast_source */
	create_param->subgroup_count = ext_create_param->num_subgroups;
	create_param->subgroup_params = subgroup_params;
	/* All QoS within the BIG will be the same, so we get the one from the first subgroup */
	create_param->qos = &ext_create_param->subgroups[0].group_lc3_preset.qos;

	create_param->packing = ext_create_param->packing;

	create_param->encryption = ext_create_param->encryption;
	if (ext_create_param->encryption) {
		memset(create_param->broadcast_code, 0, sizeof(create_param->broadcast_code));
		memcpy(create_param->broadcast_code, ext_create_param->broadcast_code,
		       sizeof(ext_create_param->broadcast_code));
	}

	return 0;
}

bool broadcast_source_is_streaming(uint8_t big_index)
{
	if (big_index >= CONFIG_BT_ISO_MAX_BIG) {
		LOG_ERR("Trying to check BIG %d out of %d", big_index, CONFIG_BT_ISO_MAX_BIG);
		return false;
	}

	if (broadcast_sources[big_index] == NULL) {
		return false;
	}

	/* All streams in a broadcast source is in the same state,
	 * so we can just check the first stream
	 */
	return le_audio_ep_state_check(cap_streams[big_index][0][0].bap_stream.ep,
				       BT_BAP_EP_STATE_STREAMING);
}

int broadcast_source_start(uint8_t big_index, struct bt_le_ext_adv *ext_adv)
{
	int ret;

	if (ext_adv == NULL) {
		LOG_ERR("No advertising set available");
		return -EINVAL;
	}

	if (big_index >= CONFIG_BT_ISO_MAX_BIG) {
		LOG_ERR("Trying to start BIG %d out of %d", big_index, CONFIG_BT_ISO_MAX_BIG);
		return -EINVAL;
	}

	LOG_DBG("Starting broadcast source");

	/* All streams in a broadcast source is in the same state,
	 * so we can just check the first stream
	 */
	if (cap_streams[big_index][0][0].bap_stream.ep == NULL) {
		LOG_ERR("stream->ep is NULL");
		return -ECANCELED;
	}

	if (le_audio_ep_state_check(cap_streams[big_index][0][0].bap_stream.ep,
				    BT_BAP_EP_STATE_STREAMING)) {
		LOG_WRN("Already streaming");
		return -EALREADY;
	}

	ret = bt_cap_initiator_broadcast_audio_start(broadcast_sources[big_index], ext_adv);
	if (ret) {
		LOG_WRN("Failed to start broadcast, ret: %d", ret);
		return ret;
	}

	return 0;
}

int broadcast_source_stop(uint8_t big_index)
{
	int ret;

	if (big_index >= CONFIG_BT_ISO_MAX_BIG) {
		LOG_ERR("Trying to stop BIG %d out of %d", big_index, CONFIG_BT_ISO_MAX_BIG);
		return -EINVAL;
	}

	/* All streams in a broadcast source is in the same state,
	 * so we can just check the first stream
	 */
	if (cap_streams[big_index][0][0].bap_stream.ep == NULL) {
		LOG_ERR("stream->ep is NULL");
		return -ECANCELED;
	}

	if (le_audio_ep_state_check(cap_streams[big_index][0][0].bap_stream.ep,
				    BT_BAP_EP_STATE_STREAMING)) {
		ret = bt_cap_initiator_broadcast_audio_stop(broadcast_sources[big_index]);
		if (ret) {
			LOG_WRN("Failed to stop broadcast, ret: %d", ret);
			return ret;
		}
	} else {
		LOG_WRN("Not in a streaming state");
		return -EINVAL;
	}

	return 0;
}

/* TODO: Use the function below once
 * https://github.com/zephyrproject-rtos/zephyr/pull/72908 is merged
 */
#if CONFIG_CUSTOM_BROADCASTER
static uint8_t audio_map_location_get(struct bt_bap_stream *bap_stream)
{
	int ret;
	enum bt_audio_location loc;

	ret = bt_audio_codec_cfg_get_chan_allocation(bap_stream->codec_cfg, &loc, false);
	if (ret) {
		LOG_WRN("Unable to find location, defaulting to left");
		return AUDIO_CH_L;
	}

	/* For now, only front_left and front_right are supported,
	 * left is default for everything else.
	 */

	if (loc == BT_AUDIO_LOCATION_FRONT_RIGHT) {
		LOG_WRN("Setting right");
		return AUDIO_CH_R;
	}

	return AUDIO_CH_L;
}
#endif

int broadcast_source_id_get(uint8_t big_index, uint32_t *broadcast_id)
{
	int ret;

	if (big_index >= CONFIG_BT_ISO_MAX_BIG) {
		LOG_ERR("Failed to get broadcast ID for BIG %d out of %d", big_index,
			CONFIG_BT_ISO_MAX_BIG);
		return -EINVAL;
	}

	if (broadcast_sources[big_index] == NULL) {
		LOG_ERR("No broadcast source");
		return -EINVAL;
	}

	if (broadcast_id == NULL) {
		LOG_ERR("NULL pointer given for broadcast_id");
		return -EINVAL;
	}

	ret = bt_cap_initiator_broadcast_get_id(broadcast_sources[big_index], broadcast_id);
	if (ret) {
		LOG_ERR("Unable to get broadcast ID: %d", ret);
		return ret;
	}

	return 0;
}

int broadcast_source_send(uint8_t big_index, uint8_t subgroup_index,
			  struct le_audio_encoded_audio enc_audio)
{
	int ret;
	uint8_t num_active_streams = 0;

	if (big_index >= CONFIG_BT_ISO_MAX_BIG) {
		LOG_ERR("Trying to send to BIG %d out of %d", big_index, CONFIG_BT_ISO_MAX_BIG);
		return -EINVAL;
	}

	struct le_audio_tx_info
		tx[CONFIG_BT_ISO_MAX_CHAN * CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT];

	for (int i = 0; i < ARRAY_SIZE(cap_streams[big_index][subgroup_index]); i++) {
		if (!le_audio_ep_state_check(
			    cap_streams[big_index][subgroup_index][i].bap_stream.ep,
			    BT_BAP_EP_STATE_STREAMING)) {
			/* Skip streams not in a streaming state */
			continue;
		}

		/* Set cap stream pointer */
		tx[num_active_streams].cap_stream = &cap_streams[big_index][subgroup_index][i];

		/* Set index */
		tx[num_active_streams].idx.lvl1 = big_index;
		tx[num_active_streams].idx.lvl2 = subgroup_index;
		tx[num_active_streams].idx.lvl3 = i;

		/* Set channel location */
		/* TODO: Use the function below once
		 * https://github.com/zephyrproject-rtos/zephyr/pull/72908 is merged
		 */
		/* tx[num_active_streams].audio_channel = audio_map_location_get(stream);*/
		tx[num_active_streams].audio_channel = i;

		num_active_streams++;
	}

	if (num_active_streams == 0) {
		LOG_WRN("No active streams");
		return -ECANCELED;
	}

	ret = bt_le_audio_tx_send(tx, num_active_streams, enc_audio);
	if (ret) {
		return ret;
	}

	return 0;
}

int broadcast_source_disable(uint8_t big_index)
{
	int ret;

	if (big_index >= CONFIG_BT_ISO_MAX_BIG) {
		LOG_ERR("Trying to disable BIG %d out of %d", big_index, CONFIG_BT_ISO_MAX_BIG);
		return -EINVAL;
	}

	/* All streams in a broadcast source is in the same state,
	 * so we can just check the first stream
	 */
	if (le_audio_ep_state_check(cap_streams[big_index][0][0].bap_stream.ep,
				    BT_BAP_EP_STATE_STREAMING)) {
		/* Deleting broadcast source in stream_stopped_cb() */
		delete_broadcast_src[big_index] = true;

		ret = bt_cap_initiator_broadcast_audio_stop(broadcast_sources[big_index]);
		if (ret) {
			LOG_WRN("Failed to stop broadcast source");
			return ret;
		}
	} else if (broadcast_sources[big_index] != NULL) {
		ret = bt_cap_initiator_broadcast_audio_delete(broadcast_sources[big_index]);
		if (ret) {
			LOG_WRN("Failed to delete broadcast source");
			return ret;
		}

		broadcast_sources[big_index] = NULL;
	}

	initialized = false;

	LOG_DBG("Broadcast source disabled");

	return 0;
}

/* Will set up one BIG, one subgroup and two BISes */
void broadcast_source_default_create(struct broadcast_source_big *broadcast_param)
{
	static enum bt_audio_location location[2] = {BT_AUDIO_LOCATION_FRONT_LEFT,
						     BT_AUDIO_LOCATION_FRONT_RIGHT};
	static struct subgroup_config subgroups;

	subgroups.group_lc3_preset = lc3_preset;

	subgroups.num_bises = 2;
	subgroups.context = BT_AUDIO_CONTEXT_TYPE_MEDIA;

	subgroups.location = location;

	broadcast_param->subgroups = &subgroups;
	broadcast_param->num_subgroups = 1;

	if (IS_ENABLED(CONFIG_BT_AUDIO_PACKING_INTERLEAVED)) {
		broadcast_param->packing = BT_ISO_PACKING_INTERLEAVED;
	} else {
		broadcast_param->packing = BT_ISO_PACKING_SEQUENTIAL;
	}

	if (IS_ENABLED(CONFIG_BT_AUDIO_BROADCAST_ENCRYPTED)) {
		broadcast_param->encryption = true;
		memset(broadcast_param->broadcast_code, 0, sizeof(broadcast_param->broadcast_code));
		memcpy(broadcast_param->broadcast_code, CONFIG_BT_AUDIO_BROADCAST_ENCRYPTION_KEY,
		       MIN(sizeof(CONFIG_BT_AUDIO_BROADCAST_ENCRYPTION_KEY),
			   sizeof(broadcast_param->broadcast_code)));
	} else {
		broadcast_param->encryption = false;
	}

	if (IS_ENABLED(CONFIG_BT_AUDIO_BROADCAST_IMMEDIATE_FLAG)) {
		bt_audio_codec_cfg_meta_set_bcast_audio_immediate_rend_flag(
			&subgroups.group_lc3_preset.codec_cfg);
	}

	bt_audio_codec_cfg_meta_set_lang(&subgroups.group_lc3_preset.codec_cfg, "eng");
}

int broadcast_source_enable(struct broadcast_source_big const *const broadcast_param,
			    uint8_t big_index)
{
	int ret;

	if (big_index >= CONFIG_BT_ISO_MAX_BIG) {
		LOG_ERR("Trying to set up %d BIGS, but only allocated memory for %d", big_index,
			CONFIG_BT_ISO_MAX_BIG);
		return -EINVAL;
	}

	if (!initialized) {
		bt_le_audio_tx_init();
	}

	LOG_INF("Enabling broadcast_source %d", big_index);

	ret = create_param_produce(big_index, broadcast_param, &create_param[big_index]);
	if (ret) {
		LOG_ERR("Failed to create the create_param: %d", ret);
		return ret;
	}

	/* Register callbacks per stream */
	for (size_t j = 0U; j < create_param[big_index].subgroup_count; j++) {
		for (size_t k = 0; k < create_param[big_index].subgroup_params[j].stream_count;
		     k++) {
			bt_cap_stream_ops_register(
				create_param[big_index].subgroup_params[j].stream_params[k].stream,
				&stream_ops);
		}
	}

	ret = bt_cap_initiator_broadcast_audio_create(&create_param[big_index],
						      &broadcast_sources[big_index]);
	if (ret) {
		LOG_ERR("Failed to create broadcast source, ret: %d", ret);
		return ret;
	}

	initialized = true;

	LOG_DBG("Broadcast source enabled");

	return 0;
}
