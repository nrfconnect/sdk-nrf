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
#include <../subsys/bluetooth/audio/bap_iso.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>

/* TODO: Remove when a get_info function is implemented in host */
#include <../subsys/bluetooth/audio/bap_endpoint.h>

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

static struct bt_cap_broadcast_source *broadcast_source[CONFIG_BT_ISO_MAX_BIG];
/* Make sure we have statically allocated streams for all potential BISes */
static struct bt_cap_stream cap_streams[CONFIG_BT_ISO_MAX_BIG]
				       [CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT]
				       [CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT /
					CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT];

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

static void le_audio_event_publish(enum le_audio_evt_type event)
{
	int ret;
	struct le_audio_msg msg;

	msg.event = event;

	ret = zbus_chan_pub(&le_audio_chan, &msg, LE_AUDIO_ZBUS_EVENT_WAIT_TIME);
	ERR_CHK(ret);
}

static int get_stream_index(struct bt_bap_stream *stream, struct stream_index *index,
			    uint8_t *flattened_index)
{
	uint8_t flat_index = 0;

	for (int i = 0; i < CONFIG_BT_ISO_MAX_BIG; i++) {
		for (int j = 0; i < CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT; j++) {
			for (int k = 0; k < ARRAY_SIZE(cap_streams[i][j]); k++) {
				if (&cap_streams[i][j][k].bap_stream == stream) {
					index->level1_idx = i;
					index->level2_idx = j;
					index->level3_idx = k;
					*flattened_index = flat_index;
					return 0;
				}
				flat_index++;
			}
		}
	}

	LOG_WRN("Stream %p not found", (void *)stream);

	return -EINVAL;
}

static void stream_sent_cb(struct bt_bap_stream *stream)
{
	int ret;
	struct stream_index index;
	uint8_t flattened_index;

	ret = get_stream_index(stream, &index, &flattened_index);
	if (ret) {
		return;
	}

	ERR_CHK(bt_le_audio_tx_stream_sent(flattened_index));
}

static void stream_started_cb(struct bt_bap_stream *stream)
{
	int ret;
	struct stream_index index;
	uint8_t flattened_index;

	ret = get_stream_index(stream, &index, &flattened_index);
	if (ret) {
		return;
	}

	ERR_CHK(bt_le_audio_tx_stream_started(flattened_index));

	le_audio_event_publish(LE_AUDIO_EVT_STREAMING);

	/* NOTE: The string below is used by the Nordic CI system */
	LOG_INF("Broadcast source %p started", (void *)stream);

	le_audio_print_codec(stream->codec_cfg, BT_AUDIO_DIR_SOURCE);
}

static void stream_stopped_cb(struct bt_bap_stream *stream, uint8_t reason)
{
	int ret;
	struct stream_index index;
	uint8_t flattened_index;

	ret = get_stream_index(stream, &index, &flattened_index);
	if (ret) {
		return;
	}

	le_audio_event_publish(LE_AUDIO_EVT_NOT_STREAMING);

	ERR_CHK(bt_le_audio_tx_stream_stopped(flattened_index));

	LOG_INF("Broadcast source %p stopped. Reason: %d", (void *)stream, reason);

	if (delete_broadcast_src[index.level1_idx] && broadcast_source[index.level1_idx] != NULL) {
		ret = bt_cap_initiator_broadcast_audio_delete(broadcast_source[index.level1_idx]);
		if (ret) {
			LOG_ERR("Unable to delete broadcast source %p", (void *)stream);
			delete_broadcast_src[index.level1_idx] = false;
			return;
		}

		broadcast_source[index.level1_idx] = NULL;

		LOG_INF("Broadcast source %p deleted", (void *)stream);

		delete_broadcast_src[index.level1_idx] = false;
	}
}

static struct bt_bap_stream_ops stream_ops = {
	.sent = stream_sent_cb,
	.started = stream_started_cb,
	.stopped = stream_stopped_cb,
};

#if (CONFIG_AURACAST)
static void public_broadcast_features_set(uint8_t *features)
{
	int freq = bt_audio_codec_cfg_get_freq(&lc3_preset.codec_cfg);

	if (features == NULL) {
		LOG_ERR("No pointer to features");
		return;
	}

	if (IS_ENABLED(CONFIG_BT_AUDIO_BROADCAST_ENCRYPTED)) {
		*features |= 0x01;
	}

	if (freq == BT_AUDIO_CODEC_CFG_FREQ_16KHZ || freq == BT_AUDIO_CODEC_CFG_FREQ_24KHZ) {
		*features |= 0x02;
	} else if (freq == BT_AUDIO_CODEC_CFG_FREQ_48KHZ) {
		*features |= 0x04;
	} else {
		LOG_WRN("%dkHz is not compatible with Auracast, choose 16kHz, 24kHz or 48kHz",
			freq);
	}
}
#endif /* (CONFIG_AURACAST) */

int broadcast_source_ext_adv_populate(uint8_t big_index,
				      struct broadcast_source_ext_adv_data *ext_adv_data,
				      struct bt_data *ext_adv_buf, size_t ext_adv_buf_vacant)
{
	int ret;
	uint32_t broadcast_id = 0;
	uint32_t ext_adv_buf_cnt = 0;
	size_t brdcst_name_size;

	if (ext_adv_data == NULL || ext_adv_buf == NULL || ext_adv_buf_vacant == 0) {
		LOG_ERR("Advertising populate failed.");
		return -EINVAL;
	}

	sys_put_le16(BT_UUID_BROADCAST_AUDIO_VAL, ext_adv_data->brdcst_id_buf);

	if (IS_ENABLED(CONFIG_BT_AUDIO_USE_BROADCAST_NAME_ALT)) {
		brdcst_name_size = sizeof(CONFIG_BT_AUDIO_BROADCAST_NAME_ALT) - 1;
		memcpy(ext_adv_data->brdcst_name_buf, CONFIG_BT_AUDIO_BROADCAST_NAME_ALT,
		       brdcst_name_size);
	} else {
		brdcst_name_size = sizeof(CONFIG_BT_AUDIO_BROADCAST_NAME) - 1;
		memcpy(ext_adv_data->brdcst_name_buf, CONFIG_BT_AUDIO_BROADCAST_NAME,
		       brdcst_name_size);
	}

	ret = bt_mgmt_adv_buffer_put(ext_adv_buf, &ext_adv_buf_cnt, ext_adv_buf_vacant,
				     brdcst_name_size, BT_DATA_BROADCAST_NAME,
				     (void *)ext_adv_data->brdcst_name_buf);
	if (ret) {
		return ret;
	}

	/* Setup extended advertising data */
	if (IS_ENABLED(CONFIG_BT_AUDIO_USE_BROADCAST_ID_RANDOM)) {
		ret = bt_cap_initiator_broadcast_get_id(broadcast_source[big_index], &broadcast_id);
		if (ret) {
			LOG_ERR("Unable to get broadcast ID: %d", ret);
			return ret;
		}
	} else {
		broadcast_id = CONFIG_BT_AUDIO_BROADCAST_ID_FIXED;
	}

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
	public_broadcast_features_set(&ext_adv_data->pba_buf[PBA_FEATURES_INDEX]);

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

	if (per_adv_data == NULL || per_adv_buf == NULL || per_adv_buf_vacant == 0) {
		LOG_ERR("Periodic advertising populate failed.");
		return -EINVAL;
	}

	/* Setup periodic advertising data */
	ret = bt_cap_initiator_broadcast_get_base(broadcast_source[big_index],
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

		subgroup_params[i].stream_count = ARRAY_SIZE(stream_params[i]);
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

int broadcast_source_start(uint8_t big_index, struct bt_le_ext_adv *ext_adv)
{
	int ret;

	if (ext_adv == NULL) {
		LOG_ERR("No advertising set available");
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

	if (cap_streams[big_index][0][0].bap_stream.ep->status.state == BT_BAP_EP_STATE_STREAMING) {
		LOG_WRN("Already streaming");
		return -EALREADY;
	}

	ret = bt_cap_initiator_broadcast_audio_start(broadcast_source[big_index], ext_adv);
	if (ret) {
		LOG_WRN("Failed to start broadcast, ret: %d", ret);
		return ret;
	}

	return 0;
}

int broadcast_source_stop(uint8_t big_index)
{
	int ret;

	/* All streams in a broadcast source is in the same state,
	 * so we can just check the first stream
	 */
	if (cap_streams[big_index][0][0].bap_stream.ep == NULL) {
		LOG_ERR("stream->ep is NULL");
		return -ECANCELED;
	}

	if (cap_streams[big_index][0][0].bap_stream.ep->status.state == BT_BAP_EP_STATE_STREAMING) {
		ret = bt_cap_initiator_broadcast_audio_stop(broadcast_source[big_index]);
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

	ret = bt_audio_codec_cfg_get_chan_allocation(bap_stream->codec_cfg, &loc);
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

int broadcast_source_send(uint8_t big_index, struct le_audio_encoded_audio enc_audio)
{
	int ret;
	struct bt_bap_stream *bap_tx_streams[CONFIG_BT_ISO_MAX_CHAN];
	uint8_t audio_mapping_mask[CONFIG_BT_ISO_MAX_CHAN] = {UINT8_MAX};
	uint8_t flat_index = 0;

	for (int i = 0; i < CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT; i++) {
		for (int j = 0; j < ARRAY_SIZE(cap_streams[big_index][i]); j++) {
			struct bt_bap_stream *stream = &cap_streams[big_index][i][j].bap_stream;

			/* TODO: Use the function below once
			 * https://github.com/zephyrproject-rtos/zephyr/pull/72908 is merged
			 */
			/* audio_mapping_mask[flat_index] = audio_map_location_get(stream); */

			audio_mapping_mask[flat_index] = j;
			bap_tx_streams[flat_index] = stream;
			flat_index++;
		}
	}

	if (!flat_index) {
		LOG_WRN("No active streams");
		return -ECANCELED;
	}

	ret = bt_le_audio_tx_send(bap_tx_streams, audio_mapping_mask, enc_audio, flat_index);
	if (ret) {
		return ret;
	}

	return 0;
}

int broadcast_source_disable(uint8_t big_index)
{
	int ret;

	if (cap_streams[big_index][0][0].bap_stream.ep->status.state == BT_BAP_EP_STATE_STREAMING) {
		/* Deleting broadcast source in stream_stopped_cb() */
		delete_broadcast_src[big_index] = true;

		ret = bt_cap_initiator_broadcast_audio_stop(broadcast_source[big_index]);
		if (ret) {
			return ret;
		}
	} else if (broadcast_source[big_index] != NULL) {
		ret = bt_cap_initiator_broadcast_audio_delete(broadcast_source[big_index]);
		if (ret) {
			return ret;
		}

		broadcast_source[big_index] = NULL;
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

	uint8_t src[3] = "eng";

	bt_audio_codec_cfg_meta_set_stream_lang(&subgroups.group_lc3_preset.codec_cfg,
						(uint32_t)sys_get_le24(src));
}

int broadcast_source_enable(struct broadcast_source_big const *const broadcast_param,
			    uint8_t num_bigs)
{
	int ret;

	struct bt_cap_initiator_broadcast_create_param create_param[CONFIG_BT_ISO_MAX_BIG];

	/* TODO: Remove once multiple BIGs are supported in all parts of the code */
	if (num_bigs > 1) {
		LOG_ERR("Only one BIG is supported at the moment");
		return -EINVAL;
	}

	if (num_bigs > CONFIG_BT_ISO_MAX_BIG) {
		LOG_ERR("Trying to set up %d BIGS, but only allocated memory for %d", num_bigs,
			CONFIG_BT_ISO_MAX_BIG);
		return -EINVAL;
	}

	if (initialized) {
		LOG_WRN("Already initialized");
		return -EALREADY;
	}

	ret = bt_le_audio_tx_init();
	if (ret) {
		return ret;
	}

	LOG_INF("Enabling broadcast_source %s", CONFIG_BT_AUDIO_BROADCAST_NAME);

	for (int i = 0; i < num_bigs; i++) {
		ret = create_param_produce(i, &broadcast_param[i], &create_param[i]);
		if (ret) {
			LOG_ERR("Failed to create the create_param: %d", ret);
			return ret;
		}

		/* Register callbacks per stream */
		for (size_t j = 0U; j < create_param[i].subgroup_count; j++) {
			for (size_t k = 0; k < create_param[i].subgroup_params[j].stream_count;
			     k++) {
				bt_cap_stream_ops_register(
					create_param[i].subgroup_params[j].stream_params[k].stream,
					&stream_ops);
			}
		}

		ret = bt_cap_initiator_broadcast_audio_create(&create_param[i],
							      &broadcast_source[i]);
		if (ret) {
			LOG_ERR("Failed to create broadcast source, ret: %d", ret);
			return ret;
		}
	}

	initialized = true;

	LOG_DBG("Broadcast source enabled");

	return 0;
}
