/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <strings.h>
#include <ctype.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/kernel.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/shell/shell.h>
#include <nrfx_clock.h>
#include <audio_defines.h>

#include "presets.h"
#include "broadcast_source.h"
#include "zbus_common.h"
#include "macros_common.h"
#include "bt_mgmt.h"
#include "lc3_streamer.h"
#include "led_assignments.h"
#include "led.h"
#include "sd_card.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, CONFIG_MAIN_LOG_LEVEL);

ZBUS_CHAN_DECLARE(bt_mgmt_chan);
ZBUS_CHAN_DECLARE(sdu_ref_chan);
ZBUS_CHAN_DECLARE(le_audio_chan);
ZBUS_MSG_SUBSCRIBER_DEFINE(le_audio_evt_sub);

/* SD card detection */
static bool sd_card_present = true;

static struct k_thread le_audio_msg_sub_thread_data;
static k_tid_t le_audio_msg_sub_thread_id;
K_THREAD_STACK_DEFINE(le_audio_msg_sub_thread_stack, CONFIG_LE_AUDIO_MSG_SUB_STACK_SIZE);
NET_BUF_POOL_FIXED_DEFINE(ble_tx_pool, CONFIG_BT_ISO_TX_BUF_COUNT, CONFIG_BT_ISO_TX_MTU,
			  sizeof(struct audio_metadata), NULL);

static void lecture_set(const struct shell *shell);
static int big_enable(const struct shell *shell, uint8_t big_index);

struct bt_le_ext_adv *ext_adv;

/* Buffer for the UUIDs. */
#define EXT_ADV_UUID_BUF_SIZE (128)
NET_BUF_SIMPLE_DEFINE_STATIC(uuid_data0, EXT_ADV_UUID_BUF_SIZE);
NET_BUF_SIMPLE_DEFINE_STATIC(uuid_data1, EXT_ADV_UUID_BUF_SIZE);

/* Buffer for periodic advertising BASE data. */
#define BASE_DATA_BUF_SIZE (256)
NET_BUF_SIMPLE_DEFINE_STATIC(base_data0, BASE_DATA_BUF_SIZE);
NET_BUF_SIMPLE_DEFINE_STATIC(base_data1, BASE_DATA_BUF_SIZE);

/* Extended advertising buffer. */
static struct bt_data ext_adv_buf[CONFIG_BT_ISO_MAX_BIG][CONFIG_EXT_ADV_BUF_MAX];

/* Periodic advertising buffer. */
static struct bt_data per_adv_buf[CONFIG_BT_ISO_MAX_BIG];

/* Total size of the PBA buffer includes the 16-bit UUID, 8-bit features and the
 * meta data size.
 */
#define BROADCAST_SRC_PBA_BUF_SIZE                                                                 \
	(BROADCAST_SOURCE_PBA_HEADER_SIZE + CONFIG_BT_AUDIO_BROADCAST_PBA_METADATA_SIZE)

/* Number of metadata items that can be assigned. */
#define BROADCAST_SOURCE_PBA_METADATA_VACANT                                                       \
	(CONFIG_BT_AUDIO_BROADCAST_PBA_METADATA_SIZE / (sizeof(struct bt_data)))

/* Make sure pba_buf is large enough for a 16bit UUID and meta data
 * (any addition to pba_buf requires an increase of this value)
 */
uint8_t pba_data[CONFIG_BT_ISO_MAX_BIG][BROADCAST_SRC_PBA_BUF_SIZE];

/**
 * @brief	Broadcast source static extended advertising data.
 */
static struct broadcast_source_ext_adv_data ext_adv_data[] = {
	{.uuid_buf = &uuid_data0,
	 .pba_metadata_vacant_cnt = BROADCAST_SOURCE_PBA_METADATA_VACANT,
	 .pba_buf = pba_data[0]},
	{.uuid_buf = &uuid_data1,
	 .pba_metadata_vacant_cnt = BROADCAST_SOURCE_PBA_METADATA_VACANT,
	 .pba_buf = pba_data[1]}};

/**
 * @brief	Broadcast source static periodic advertising data.
 */
static struct broadcast_source_per_adv_data per_adv_data[] = {{.base_buf = &base_data0},
							      {.base_buf = &base_data1}};

static struct broadcast_source_big broadcast_param[CONFIG_BT_ISO_MAX_BIG];
static struct subgroup_config subgroups[CONFIG_BT_ISO_MAX_BIG]
				       [CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT];

static enum bt_audio_location stream_location[CONFIG_BT_ISO_MAX_BIG]
					     [CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT]
					     [CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT] = {
						     {{BT_AUDIO_LOCATION_MONO_AUDIO}}};

#define LC3_STREAMER_INDEX_UNUSED 0xFF

struct subgroup_lc3_stream_info {
	size_t frame_size;
	uint8_t lc3_streamer_idx[CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT];
	bool frame_loaded[CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT];
	uint8_t *frame_ptrs[CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT];
};

static struct subgroup_lc3_stream_info lc3_stream_infos[CONFIG_BT_ISO_MAX_BIG]
						       [CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT];

/* Find the most significant 1 in bits, bits will not be 0. */
static int8_t context_msb_one(uint16_t bits)
{
	uint8_t pos = 0;

	while (bits >>= 1) {
		pos++;
	}

	return pos;
}

static struct bt_bap_lc3_preset *preset_find(const char *name)
{
	for (size_t i = 0; i < ARRAY_SIZE(bap_presets); i++) {
		if (strcmp(bap_presets[i].name, name) == 0) {
			return bap_presets[i].preset;
		}
	}

	return NULL;
}

static char *preset_name_find(struct bt_bap_lc3_preset *preset)
{
	for (size_t i = 0; i < ARRAY_SIZE(bap_presets); i++) {
		if (bap_presets[i].preset == preset) {
			return bap_presets[i].name;
		}
	}

	return "Custom";
}

/**
 * @brief	Check if a given string is a hexadecimal value.
 *
 * @param[in]	str	Pointer to the string to check.
 * @param[out]	len	Pointer to the length of the string.
 *
 * @return	True if the string is a hexadecimal value, false otherwise.
 */
static bool is_hex(const char *str, uint8_t *len)
{
	/* Check if the string is empty */
	if (*str == '\0') {
		return false;
	}

	*len = strlen(str);

	/* Skip 0x if present */
	if (*str == '0' && (*(str + 1) == 'x' || *(str + 1) == 'X')) {
		str += 2;
		*len -= 2;
	}

	/* Iterate through each character of the string */
	for (uint8_t i = 0; i < *len; i++) {
		/* Check if the character is not a hexadecimal digit */
		if (!isxdigit((int)*str)) {
			return false;
		}

		str++;
	}

	/* All characters are hexadecimal digits */
	return true;
}

static bool is_number(const char *str)
{
	/* Check if the string is empty */
	if (*str == '\0') {
		return false;
	}

	uint8_t len = strlen(str);

	/* Iterate through each character of the string */
	for (uint8_t i = 0; i < len; i++) {
		/* Check if the character is not a digit */
		if (!isdigit((int)*str)) {
			return false;
		}
		str++;
	}

	/* All characters are digits */
	return true;
}

static void subgroup_send(struct stream_index stream_idx)
{
	int ret;
	static int prev_ret;

	uint8_t num_bis = subgroups[stream_idx.lvl1][stream_idx.lvl2].num_bises;
	size_t frame_size = lc3_stream_infos[stream_idx.lvl1][stream_idx.lvl2].frame_size;
	struct net_buf *audio_frame = net_buf_alloc(&ble_tx_pool, K_NO_WAIT);
	struct audio_metadata *meta = net_buf_user_data(audio_frame);

	if (audio_frame == NULL) {
		LOG_ERR("Out of RX buffers");
		return;
	}

	for (int i = 0; i < num_bis; i++) {
		uint8_t *frame_ptr =
			lc3_stream_infos[stream_idx.lvl1][stream_idx.lvl2].frame_ptrs[i];

		net_buf_add_mem(audio_frame, frame_ptr, frame_size);

		meta->locations |=
			(uint32_t)subgroups[stream_idx.lvl1][stream_idx.lvl2].location[i];
	}

	meta->data_coding = LC3;

	struct bt_audio_codec_cfg *codec_cfg =
		&subgroups[stream_idx.lvl1][stream_idx.lvl2].group_lc3_preset.codec_cfg;

	ret = le_audio_freq_hz_get(codec_cfg, &meta->sample_rate_hz);
	if (ret) {
		LOG_ERR("Failed to get frequency: %d", ret);
	}

	ret = le_audio_duration_us_get(codec_cfg, &meta->data_len_us);
	if (ret) {
		LOG_ERR("Failed to get frame duration: %d", ret);
	}

	ret = broadcast_source_send(audio_frame, stream_idx.lvl1, stream_idx.lvl2);
	if (ret != 0 && ret != prev_ret) {
		if (ret == -ECANCELED) {
			LOG_WRN("Sending cancelled");
		} else {
			LOG_WRN("broadcast_source_send returned: %d", ret);
		}
	}

	net_buf_unref(audio_frame);

	prev_ret = ret;
}

static void stream_frame_dummy_get_and_send(struct stream_index stream_idx)
{
	struct subgroup_lc3_stream_info *bis_info =
		&(lc3_stream_infos[stream_idx.lvl1][stream_idx.lvl2]);

	static uint8_t frame_value;

	uint8_t num_bis = subgroups[stream_idx.lvl1][stream_idx.lvl2].num_bises;
	bis_info->frame_size = subgroups[stream_idx.lvl1][stream_idx.lvl2].group_lc3_preset.qos.sdu;

	uint8_t frame_buffer[bis_info->frame_size];

	memset(frame_buffer, frame_value, bis_info->frame_size);

	bis_info->frame_ptrs[stream_idx.lvl3] = frame_buffer;
	bis_info->frame_loaded[stream_idx.lvl3] = true;

	int loaded_count = 0;

	for (int i = 0; i < num_bis; i++) {
		if (bis_info->frame_loaded[i]) {
			++loaded_count;
		}
	}

	if (loaded_count == num_bis) {
		frame_value++;
		subgroup_send(stream_idx);

		for (int i = 0; i < num_bis; i++) {
			bis_info->frame_loaded[i] = false;
			bis_info->frame_ptrs[i] = NULL;
		}
	}
}

static void stream_frame_get_and_send(struct stream_index stream_idx)
{
	int ret;

	struct subgroup_lc3_stream_info *bis_info =
		&(lc3_stream_infos[stream_idx.lvl1][stream_idx.lvl2]);

	uint8_t num_bis = subgroups[stream_idx.lvl1][stream_idx.lvl2].num_bises;
	uint8_t stream_file_idx = bis_info->lc3_streamer_idx[stream_idx.lvl3];

	if (stream_file_idx == LC3_STREAMER_INDEX_UNUSED) {
		LOG_ERR("Stream index for stream big %d sub: %d bis: %d is unused", stream_idx.lvl1,
			stream_idx.lvl2, stream_idx.lvl3);
		return;
	}

	ret = lc3_streamer_next_frame_get(
		stream_file_idx, (const uint8_t **const)&(bis_info->frame_ptrs[stream_idx.lvl3]));
	if (ret == -ENODATA) {
		LOG_WRN("No more frames to read");
		ret = lc3_streamer_stream_close(stream_file_idx);
		if (ret) {
			LOG_ERR("Failed to close stream: %d", ret);
		}

		bis_info->lc3_streamer_idx[stream_idx.lvl3] = LC3_STREAMER_INDEX_UNUSED;

		return;
	} else if (ret == -ENOMSG) {
		LOG_DBG("Frame from SD card not ready for stream %d, using zero packet",
			stream_idx.lvl3);
		bis_info->frame_ptrs[stream_idx.lvl3] = NULL;
	} else if (ret) {
		LOG_ERR("Failed to get next frame: %d", ret);
		return;
	}

	bis_info->frame_loaded[stream_idx.lvl3] = true;

	int loaded_count = 0;

	for (int i = 0; i < num_bis; i++) {
		if (bis_info->frame_loaded[i]) {
			++loaded_count;
		}
	}
	if (loaded_count == num_bis) {
		subgroup_send(stream_idx);

		for (int i = 0; i < num_bis; i++) {
			bis_info->frame_loaded[i] = false;
			bis_info->frame_ptrs[i] = NULL;
		}
	}
}

/**
 * @brief	Handle Bluetooth LE audio events.
 */
static void le_audio_msg_sub_thread(void)
{
	int ret;
	const struct zbus_channel *chan;

	LOG_DBG("Zbus subscription thread started");

	while (1) {
		struct le_audio_msg msg;

		ret = zbus_sub_wait_msg(&le_audio_evt_sub, &chan, &msg, K_FOREVER);
		ERR_CHK(ret);

		switch (msg.event) {
		case LE_AUDIO_EVT_STREAM_SENT:
			LOG_DBG("LE_AUDIO_EVT_STREAM_SENT for stream BIG %d sub: %d BIS: %d",
				msg.idx.lvl1, msg.idx.lvl2, msg.idx.lvl3);

			if (sd_card_present) {
				stream_frame_get_and_send(msg.idx);
			} else {
				stream_frame_dummy_get_and_send(msg.idx);
			}

			break;

		case LE_AUDIO_EVT_STREAMING:
			LOG_DBG("LE audio evt streaming for stream BIG %d sub: %d BIS: %d",
				msg.idx.lvl1, msg.idx.lvl2, msg.idx.lvl3);

			if (sd_card_present) {
				stream_frame_get_and_send(msg.idx);
			} else {
				stream_frame_dummy_get_and_send(msg.idx);
			}

			break;

		case LE_AUDIO_EVT_NOT_STREAMING:
			LOG_DBG("LE audio evt not_streaming");

			break;

		default:
			LOG_WRN("Unexpected/unhandled le_audio event: %d", msg.event);

			break;
		}

		STACK_USAGE_PRINT("le_audio_msg_thread", &le_audio_msg_sub_thread_data);
	}
}

/**
 * @brief	Create zbus subscriber threads.
 *
 * @return	0 for success, error otherwise.
 */
static int zbus_subscribers_create(void)
{
	int ret;

	le_audio_msg_sub_thread_id = k_thread_create(
		&le_audio_msg_sub_thread_data, le_audio_msg_sub_thread_stack,
		CONFIG_LE_AUDIO_MSG_SUB_STACK_SIZE, (k_thread_entry_t)le_audio_msg_sub_thread, NULL,
		NULL, NULL, K_PRIO_PREEMPT(CONFIG_LE_AUDIO_MSG_SUB_THREAD_PRIO), 0, K_NO_WAIT);
	ret = k_thread_name_set(le_audio_msg_sub_thread_id, "LE_AUDIO_MSG_SUB");
	if (ret) {
		LOG_ERR("Failed to create le_audio_msg thread");
		return ret;
	}

	return 0;
}

/**
 * @brief	Zbus listener to receive events from bt_mgmt.
 *
 * @param[in]	chan	Zbus channel.
 *
 * @note	Will in most cases be called from BT_RX context,
 *		so there should not be too much processing done here.
 */
static void bt_mgmt_evt_handler(const struct zbus_channel *chan)
{
	int ret;
	const struct bt_mgmt_msg *msg;

	msg = zbus_chan_const_msg(chan);

	switch (msg->event) {
	case BT_MGMT_EXT_ADV_WITH_PA_READY:
		ext_adv = msg->ext_adv;

		ret = broadcast_source_start(msg->index, ext_adv);
		if (ret) {
			LOG_ERR("Failed to start broadcaster: %d", ret);
		}

		break;

	default:
		LOG_WRN("Unexpected/unhandled bt_mgmt event: %d", msg->event);
		break;
	}
}

ZBUS_LISTENER_DEFINE(bt_mgmt_evt_listen, bt_mgmt_evt_handler);

/**
 * @brief	Link zbus producers and observers.
 *
 * @return	0 for success, error otherwise.
 */
static int zbus_link_producers_observers(void)
{
	int ret;

	if (!IS_ENABLED(CONFIG_ZBUS)) {
		return -ENOTSUP;
	}

	ret = zbus_chan_add_obs(&bt_mgmt_chan, &bt_mgmt_evt_listen,
				ZBUS_ADD_OBS_TIMEOUT_MS);
	if (ret) {
		LOG_ERR("Failed to add bt_mgmt listener");
		return ret;
	}

	ret = zbus_chan_add_obs(&le_audio_chan, &le_audio_evt_sub,
				ZBUS_ADD_OBS_TIMEOUT_MS);
	if (ret) {
		LOG_ERR("Failed to add le_audio sub");
		return ret;
	}

	return 0;
}

/*
 * @brief  The following configures the data for the extended advertising.
 *         This includes the Broadcast Audio Announcements [BAP 3.7.2.1] and Broadcast_ID
 *         [BAP 3.7.2.1.1] in the AUX_ADV_IND Extended Announcements.
 *
 * @param  big_index         Index of the Broadcast Isochronous Group (BIG) to get
 *                           advertising data for.
 * @param  ext_adv_data      Pointer to the extended advertising buffers.
 * @param  ext_adv_buf       Pointer to the bt_data used for extended advertising.
 * @param  ext_adv_buf_size  Size of @p ext_adv_buf.
 * @param  ext_adv_count     Pointer to the number of elements added to @p adv_buf.
 *
 * @return  0 for success, error otherwise.
 */
static int ext_adv_populate(uint8_t big_index, struct broadcast_source_ext_adv_data *ext_adv_data,
			    struct bt_data *ext_adv_buf, size_t ext_adv_buf_size,
			    size_t *ext_adv_count)
{
	int ret;
	size_t ext_adv_buf_cnt = 0;

	ext_adv_buf[ext_adv_buf_cnt].type = BT_DATA_UUID16_ALL;
	ext_adv_buf[ext_adv_buf_cnt].data = ext_adv_data->uuid_buf->data;
	ext_adv_buf_cnt++;

	ext_adv_buf[ext_adv_buf_cnt].type = BT_DATA_NAME_COMPLETE;
	if (strnlen(broadcast_param[big_index].adv_name,
		    ARRAY_SIZE(broadcast_param[big_index].adv_name)) > 0) {
		/* Use custom advertising name */
		ext_adv_buf[ext_adv_buf_cnt].data = broadcast_param[big_index].adv_name;
		ext_adv_buf[ext_adv_buf_cnt].data_len = strlen(broadcast_param[big_index].adv_name);
	} else {
		/* Use default device name */
		ext_adv_buf[ext_adv_buf_cnt].data = CONFIG_BT_DEVICE_NAME;
		ext_adv_buf[ext_adv_buf_cnt].data_len = strlen(CONFIG_BT_DEVICE_NAME);
	}
	ext_adv_buf_cnt++;

	ret = bt_mgmt_manufacturer_uuid_populate(ext_adv_data->uuid_buf,
						 CONFIG_BT_DEVICE_MANUFACTURER_ID);
	if (ret) {
		LOG_ERR("Failed to add adv data with manufacturer ID: %d", ret);
		return ret;
	}

	ret = broadcast_source_ext_adv_populate(big_index, broadcast_param[big_index].fixed_id,
						broadcast_param[big_index].broadcast_id,
						ext_adv_data, &ext_adv_buf[ext_adv_buf_cnt],
						ext_adv_buf_size - ext_adv_buf_cnt);
	if (ret < 0) {
		LOG_ERR("Failed to add ext adv data for broadcast source: %d", ret);
		return ret;
	}

	ext_adv_buf_cnt += ret;

	/* Add the number of UUIDs */
	ext_adv_buf[0].data_len = ext_adv_data->uuid_buf->len;

	LOG_DBG("Size of adv data: %d, num_elements: %d", sizeof(struct bt_data) * ext_adv_buf_cnt,
		ext_adv_buf_cnt);

	*ext_adv_count = ext_adv_buf_cnt;

	return 0;
}

/*
 * @brief  The following configures the data for the periodic advertising.
 *         This includes the Basic Audio Announcement, including the
 *         BASE [BAP 3.7.2.2] and BIGInfo.
 *
 * @param  big_index         Index of the Broadcast Isochronous Group (BIG) to get
 *                           advertising data for.
 * @param  pre_adv_data      Pointer to the periodic advertising buffers.
 * @param  per_adv_buf       Pointer to the bt_data used for periodic advertising.
 * @param  per_adv_buf_size  Size of @p ext_adv_buf.
 * @param  per_adv_count     Pointer to the number of elements added to @p adv_buf.
 *
 * @return  0 for success, error otherwise.
 */
static int per_adv_populate(uint8_t big_index, struct broadcast_source_per_adv_data *pre_adv_data,
			    struct bt_data *per_adv_buf, size_t per_adv_buf_size,
			    size_t *per_adv_count)
{
	int ret;
	size_t per_adv_buf_cnt = 0;

	ret = broadcast_source_per_adv_populate(big_index, pre_adv_data, per_adv_buf,
						per_adv_buf_size - per_adv_buf_cnt);
	if (ret < 0) {
		LOG_ERR("Failed to add per adv data for broadcast source: %d", ret);
		return ret;
	}

	per_adv_buf_cnt += ret;

	LOG_DBG("Size of per adv data: %d, num_elements: %d",
		sizeof(struct bt_data) * per_adv_buf_cnt, per_adv_buf_cnt);

	*per_adv_count = per_adv_buf_cnt;

	return 0;
}

static void broadcast_create(uint8_t big_index)
{
	if (big_index >= CONFIG_BT_ISO_MAX_BIG) {
		LOG_ERR("BIG index out of range");
		return;
	}

	struct broadcast_source_big *brdcst_param = &broadcast_param[big_index];

	for (size_t i = 0; i < ARRAY_SIZE(subgroups[big_index]); i++) {
		if (subgroups[big_index][i].num_bises == 0) {
			/* Default to 2 BISes */
			subgroups[big_index][i].num_bises = 2;
		}

		if (subgroups[big_index][i].context == 0) {
			subgroups[big_index][i].context = BT_AUDIO_CONTEXT_TYPE_MEDIA;
		}

		subgroups[big_index][i].location = stream_location[big_index][i];
	}

	brdcst_param->subgroups = subgroups[big_index];

	if (brdcst_param->broadcast_name[0] == '\0') {
		/* Name not set, using default */
		if (sizeof(CONFIG_BT_AUDIO_BROADCAST_NAME) > sizeof(brdcst_param->broadcast_name)) {
			LOG_ERR("Broadcast name too long");
			return;
		}

		size_t brdcst_name_size = sizeof(CONFIG_BT_AUDIO_BROADCAST_NAME) - 1;

		memcpy(brdcst_param->broadcast_name, CONFIG_BT_AUDIO_BROADCAST_NAME,
		       brdcst_name_size);
	}

	/* If no subgroups are defined, set to 1 */
	if (brdcst_param->num_subgroups == 0) {
		brdcst_param->num_subgroups = 1;
	}
}

/**
 * @brief	Clear all broadcast configurations.
 */
static void nrf_auraconfig_clear(void)
{
	int ret;

	if (sd_card_present) {
		ret = lc3_streamer_close_all_streams();
		if (ret) {
			LOG_ERR("Failed to close all LC3 streams: %d", ret);
		}
	}

	for (size_t i = 0; i < ARRAY_SIZE(broadcast_param); i++) {
		memset(&broadcast_param[i], 0, sizeof(broadcast_param[i]));
		for (size_t j = 0; j < ARRAY_SIZE(subgroups[i]); j++) {
			memset(&subgroups[i][j], 0, sizeof(subgroups[i][j]));
			for (size_t k = 0; k < ARRAY_SIZE(stream_location[i][j]); k++) {
				stream_location[i][j][k] = BT_AUDIO_LOCATION_MONO_AUDIO;
				lc3_stream_infos[i][j].lc3_streamer_idx[k] =
					LC3_STREAMER_INDEX_UNUSED;
			}
		}
	}
}

#define SD_FILECOUNT_MAX 420
#define SD_PATHLEN_MAX	 190
static char sd_paths_and_files[SD_FILECOUNT_MAX][SD_PATHLEN_MAX];
static uint32_t num_files_added;

static int sd_card_toc_gen(void)
{

	/* Traverse SD tree */
	int ret;

	ret = sd_card_list_files_match(SD_FILECOUNT_MAX, SD_PATHLEN_MAX, sd_paths_and_files, NULL,
				       ".lc3");
	if (ret < 0) {
		return ret;
	}

	num_files_added = ret;

	LOG_INF("Number of *.lc3 files on SD card: %d", num_files_added);

	return 0;
}

void nrf_auraconfig_main(void)
{
	int ret;

	LOG_DBG("Main started");

	ret = nrfx_clock_divider_set(NRF_CLOCK_DOMAIN_HFCLK, NRF_CLOCK_HFCLK_DIV_1);
	ret -= NRFX_ERROR_BASE_NUM;
	ERR_CHK_MSG(ret, "Failed to set HFCLK divider");

	ret = led_init();
	ERR_CHK_MSG(ret, "Failed to initialize LED module");

	ret = led_on(LED_AUDIO_APP_STATUS, LED_COLOR_GREEN);
	ERR_CHK(ret);

	ret = bt_mgmt_init();
	ERR_CHK(ret);

	ret = zbus_link_producers_observers();
	ERR_CHK_MSG(ret, "Failed to link zbus producers and observers");

	ret = lc3_streamer_init();
	if (ret) {
		LOG_WRN("Failed to initialize LC3 streamer. Transmitting fixed buffers");
		sd_card_present = false;
	}

	for (int i = 0; i < CONFIG_BT_ISO_MAX_BIG; i++) {
		for (int j = 0; j < CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT; j++) {
			for (int k = 0; k < CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT; k++) {
				lc3_stream_infos[i][j].lc3_streamer_idx[k] =
					LC3_STREAMER_INDEX_UNUSED;
			}
		}
	}

	ret = zbus_subscribers_create();
	ERR_CHK_MSG(ret, "Failed to create zbus subscriber threads");

	if (sd_card_present) {
		ret = sd_card_toc_gen();
		ERR_CHK_MSG(ret, "Failed to generate SD card table");
	}
}

static void context_print(const struct shell *shell)
{
	shell_print(shell, "%s", bt_audio_context_bit_to_str(0));

	for (int i = 0; i <= context_msb_one(BT_AUDIO_CONTEXT_TYPE_ANY); i++) {
		shell_print(shell, "%s", bt_audio_context_bit_to_str(BIT(i)));
	}
}

static void codec_qos_print(const struct shell *shell, struct bt_bap_qos_cfg *qos)
{
	if (qos->phy == BT_BAP_QOS_CFG_1M || qos->phy == BT_BAP_QOS_CFG_2M) {
		shell_print(shell, "\t\t\tPHY: %dM", qos->phy);
	} else if (qos->phy == BT_BAP_QOS_CFG_CODED) {
		shell_print(shell, "\t\t\tPHY: LE Coded");
	} else {
		shell_print(shell, "\t\t\tPHY: Unknown");
	}

	shell_print(shell, "\t\t\tFraming: %s",
		    (qos->framing == BT_BAP_QOS_CFG_FRAMING_UNFRAMED ? "unframed" : "framed"));
	shell_print(shell, "\t\t\tRTN: %d", qos->rtn);
	shell_print(shell, "\t\t\tSDU size: %d", qos->sdu);
	shell_print(shell, "\t\t\tMax Transport Latency: %d ms", qos->latency);
	shell_print(shell, "\t\t\tFrame Interval: %d us", qos->interval);
	shell_print(shell, "\t\t\tPresentation Delay: %d us", qos->pd);
}

static void nrf_auraconfig_print(const struct shell *shell, uint8_t group_index)
{
	int ret;

	struct broadcast_source_big *brdcst_param = &broadcast_param[group_index];

	shell_print(shell, "\tAdvertising name: %s",
		    strlen(brdcst_param->adv_name) > 0 ? brdcst_param->adv_name
						       : CONFIG_BT_DEVICE_NAME);

	shell_print(shell, "\tBroadcast name: %s", brdcst_param->broadcast_name);

	if (brdcst_param->fixed_id) {
		shell_print(shell, "\tBroadcast ID (fixed): 0x%04x", brdcst_param->broadcast_id);
	} else if (!broadcast_source_is_streaming(group_index)) {
		shell_print(shell, "\tBroadcast ID (random): Will be set on start");
	} else {
		uint32_t broadcast_id;

		broadcast_source_id_get(group_index, &broadcast_id);
		shell_print(shell, "\tBroadcast ID (random): 0x%04x", broadcast_id);
	}

	shell_print(shell, "\tPacking: %s",
		    (brdcst_param->packing == BT_ISO_PACKING_INTERLEAVED ? "interleaved"
									 : "sequential"));
	shell_print(shell, "\tEncryption: %s",
		    (brdcst_param->encryption == true ? "true" : "false"));

	shell_print(shell, "\tBroadcast code: %s", brdcst_param->broadcast_code);

	for (size_t i = 0; i < brdcst_param->num_subgroups; i++) {
		struct bt_audio_codec_cfg *codec_cfg =
			&brdcst_param->subgroups[i].group_lc3_preset.codec_cfg;

		shell_print(shell, "\tSubgroup %d:", i);

		shell_print(shell, "\t\tPreset: %s", brdcst_param->subgroups[i].preset_name);

		codec_qos_print(shell, &brdcst_param->subgroups[i].group_lc3_preset.qos);

		int freq_hz = 0;

		ret = le_audio_freq_hz_get(codec_cfg, &freq_hz);
		if (ret) {
			shell_error(shell, "Failed to get sampling rate: %d", ret);
		} else {
			shell_print(shell, "\t\tSampling rate: %d Hz", freq_hz);
		}

		uint32_t bitrate = 0;

		ret = le_audio_bitrate_get(codec_cfg, &bitrate);
		if (ret) {
			shell_error(shell, "Failed to get bit rate: %d", ret);
		} else {
			shell_print(shell, "\t\tBit rate: %d bps", bitrate);
		}

		int frame_duration = 0;

		ret = le_audio_duration_us_get(codec_cfg, &frame_duration);
		if (ret) {
			shell_error(shell, "Failed to get frame duration: %d", ret);
		} else {
			shell_print(shell, "\t\tFrame duration: %d us", frame_duration);
		}

		uint32_t octets_per_sdu = 0;

		ret = le_audio_octets_per_frame_get(codec_cfg, &octets_per_sdu);
		if (ret) {
			shell_error(shell, "Failed to get octets per frame: %d", ret);
		} else {
			shell_print(shell, "\t\tOctets per frame: %d", octets_per_sdu);
		}

		const uint8_t *language;

		ret = bt_audio_codec_cfg_meta_get_lang(codec_cfg, &language);

		if (ret) {
			shell_print(shell, "\t\tLanguage: not_set");
		} else {
			char lang[LANGUAGE_LEN + 1] = {'\0'};

			memcpy(lang, language, LANGUAGE_LEN);

			shell_print(shell, "\t\tLanguage: %s", lang);
		}

		shell_print(shell, "\t\tContext(s):");

		/* Context container is a bit field */
		for (size_t j = 0U; j <= context_msb_one(BT_AUDIO_CONTEXT_TYPE_ANY); j++) {
			const uint16_t bit_val = BIT(j);

			if (brdcst_param->subgroups[i].context & bit_val) {
				shell_print(shell, "\t\t\t%s",
					    bt_audio_context_bit_to_str(bit_val));
			}
		}

		const unsigned char *program_info;

		ret = bt_audio_codec_cfg_meta_get_program_info(codec_cfg, &program_info);
		if (ret > 0) {
			shell_print(shell, "\t\tProgram info: %s", program_info);
		}

		int immediate =
			bt_audio_codec_cfg_meta_get_bcast_audio_immediate_rend_flag(codec_cfg);

		shell_print(shell, "\t\tImmediate rendering flag: %s",
			    (immediate == 0 ? "set" : "not set"));
		shell_print(shell, "\t\tNumber of BIS: %d", brdcst_param->subgroups[i].num_bises);
		shell_print(shell, "\t\tLocation:");

		for (size_t j = 0; j < brdcst_param->subgroups[i].num_bises; j++) {
			shell_print(shell, "\t\t\tBIS %d: %s", j,
				    bt_audio_location_bit_to_str(
					    brdcst_param->subgroups[i].location[j]));
		}

		shell_print(shell, "\t\tFiles:");

		for (size_t j = 0; j < brdcst_param->subgroups[i].num_bises; j++) {
			uint8_t streamer_idx = lc3_stream_infos[group_index][i].lc3_streamer_idx[j];

			if (!sd_card_present) {
				shell_print(shell, "\t\t\tBIS %d: Fixed data", j);
			} else if (streamer_idx == LC3_STREAMER_INDEX_UNUSED) {
				shell_print(shell, "\t\t\tBIS %d: Not set", j);
			} else {
				char file_name[CONFIG_FS_FATFS_MAX_LFN];
				bool looping = lc3_streamer_is_looping(streamer_idx);

				lc3_streamer_file_path_get(streamer_idx, file_name,
							   sizeof(file_name));
				shell_print(shell, "\t\t\tBIS %d: %s %s", j, file_name,
					    looping ? "(looping)" : "");
			}
		}
	}
}

static int cmd_list(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	for (size_t i = 0; i < ARRAY_SIZE(bap_presets); i++) {
		shell_print(shell, "%s", bap_presets[i].name);
	}

	return 0;
}

static int adv_create_and_start(const struct shell *shell, uint8_t big_index)
{
	int ret;

	size_t ext_adv_buf_cnt = 0;
	size_t per_adv_buf_cnt = 0;

	if (big_index >= CONFIG_BT_ISO_MAX_BIG) {
		shell_error(shell, "BIG index out of range");
		return -EINVAL;
	}

	if (broadcast_param[big_index].broadcast_name[0] == '\0') {
		/* Name not set, using default */
		size_t brdcst_name_size = sizeof(CONFIG_BT_AUDIO_BROADCAST_NAME) - 1;

		if (brdcst_name_size >= ARRAY_SIZE(ext_adv_data[big_index].brdcst_name_buf)) {
			shell_error(shell, "Broadcast name too long, using parts of the name");
			brdcst_name_size = ARRAY_SIZE(ext_adv_data[big_index].brdcst_name_buf) - 1;
		}

		memcpy(ext_adv_data[big_index].brdcst_name_buf, CONFIG_BT_AUDIO_BROADCAST_NAME,
		       brdcst_name_size);
	} else {
		size_t brdcst_name_size = strlen(broadcast_param[big_index].broadcast_name);

		if (brdcst_name_size >= ARRAY_SIZE(ext_adv_data[big_index].brdcst_name_buf)) {
			shell_error(shell, "Broadcast name too long, using parts of the name");
			brdcst_name_size = ARRAY_SIZE(ext_adv_data[big_index].brdcst_name_buf) - 1;
		}

		memcpy(ext_adv_data[big_index].brdcst_name_buf,
		       broadcast_param[big_index].broadcast_name, brdcst_name_size);
	}

	if (broadcast_param[big_index].adv_name[0] == '\0') {
		/* Name not set, using default */
		size_t adv_name_size = sizeof(CONFIG_BT_AUDIO_BROADCAST_NAME) - 1;

		memcpy(broadcast_param[big_index].adv_name, CONFIG_BT_AUDIO_BROADCAST_NAME,
		       adv_name_size);
	}

	bt_set_name(broadcast_param[big_index].adv_name);

	/* Get advertising set for BIG0 */
	ret = ext_adv_populate(big_index, &ext_adv_data[big_index], ext_adv_buf[big_index],
			       ARRAY_SIZE(ext_adv_buf[big_index]), &ext_adv_buf_cnt);
	if (ret) {
		return ret;
	}

	ret = per_adv_populate(big_index, &per_adv_data[big_index], &per_adv_buf[big_index], 1,
			       &per_adv_buf_cnt);
	if (ret) {
		return ret;
	}

	/* Start broadcaster */
	ret = bt_mgmt_adv_start(big_index, ext_adv_buf[big_index], ext_adv_buf_cnt,
				&per_adv_buf[big_index], per_adv_buf_cnt, false);
	if (ret) {
		shell_error(shell, "Failed to start advertising for BIG%d", big_index);
		return ret;
	}

	return 0;
}

/**
 * @brief	Converts the arguments to BIG and subgroup indexes.
 *
 * @param[in]	shell		Pointer to the shell instance.
 * @param[in]	argc		Number of arguments.
 * @param[in]	argv		Pointer to the arguments.
 * @param[out]	big_index	Pointer to the BIG index.
 * @param[in]	argv_big	Index of the BIG index in the arguments.
 * @param[out]	subgroup_index	Pointer to the subgroup index.
 * @param[in]	argv_subgroup	Index of the subgroup index in the arguments.
 *
 * @return	0 for success, error otherwise.
 */
static int argv_to_indexes(const struct shell *shell, size_t argc, char **argv, uint8_t *big_index,
			   uint8_t argv_big, uint8_t *subgroup_index, uint8_t argv_subgroup)
{

	if (big_index == NULL) {
		shell_error(shell, "BIG index not provided");
		return -EINVAL;
	}

	if (argv_big >= argc) {
		shell_error(shell, "BIG index not provided");
		return -EINVAL;
	}

	if (!is_number(argv[argv_big])) {
		shell_error(shell, "BIG index must be a digit");
		return -EINVAL;
	}

	*big_index = (uint8_t)atoi(argv[argv_big]);

	if (*big_index >= CONFIG_BT_ISO_MAX_BIG) {
		shell_error(shell, "BIG index out of range");
		return -EINVAL;
	}

	if (subgroup_index != NULL) {
		if (argv_subgroup >= argc) {
			shell_error(shell, "Subgroup index not provided");
			return -EINVAL;
		}

		if (!is_number(argv[argv_subgroup])) {
			shell_error(shell, "Subgroup index must be a digit");
			return -EINVAL;
		}

		*subgroup_index = (uint8_t)atoi(argv[argv_subgroup]);

		if (*subgroup_index >= CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT) {
			shell_error(shell, "Subgroup index exceeds max value: %d",
				    CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT - 1);
			return -EINVAL;
		}

		if (broadcast_param[*big_index].subgroups == NULL) {
			shell_error(shell, "No subgroups defined for BIG%d", *big_index);
			return -EINVAL;
		}

		if (*subgroup_index >= broadcast_param[*big_index].num_subgroups) {
			shell_error(shell, "Subgroup index out of range");
			return -EINVAL;
		}
	}

	return 0;
}

static int big_enable(const struct shell *shell, uint8_t big_index)
{
	int ret;

	if (big_index >= CONFIG_BT_ISO_MAX_BIG) {
		return -EINVAL;
	}

	if ((broadcast_param[big_index].subgroups == NULL) ||
	    (broadcast_param[big_index].num_subgroups == 0)) {
		LOG_ERR("No subgroups defined for BIG%d", big_index);
		return -EINVAL;
	}

	if (broadcast_source_is_streaming(big_index)) {
		LOG_WRN("BIG %d is already streaming", big_index);
		/* Do not return error code as this might be called from a for-loop */
		return 0;
	}

	ret = broadcast_source_enable(&broadcast_param[big_index], big_index);
	if (ret) {
		shell_error(shell, "Failed to enable broadcaster: %d", ret);
		return ret;
	}

	ret = adv_create_and_start(shell, big_index);
	if (ret) {
		shell_error(shell, "Failed to start advertising for BIG%d: %d", big_index, ret);
		return ret;
	}

	return 0;
}

static int cmd_start(const struct shell *shell, size_t argc, char **argv)
{
	int ret;

	if (argc == 2) {
		uint8_t big_index;

		ret = argv_to_indexes(shell, argc, argv, &big_index, 1, NULL, 0);
		if (ret) {
			return ret;
		}

		ret = big_enable(shell, big_index);
		if (ret) {
			return ret;
		}

	} else {
		for (int i = 0; i < CONFIG_BT_ISO_MAX_BIG; i++) {
			if (broadcast_param[i].subgroups != NULL ||
			    broadcast_param[i].num_subgroups > 0) {
				ret = big_enable(shell, i);
				if (ret) {
					return ret;
				}
			}
		}
	}

	led_blink(LED_AUDIO_APP_STATUS, LED_COLOR_GREEN);

	return 0;
}

static int broadcaster_stop(const struct shell *shell, uint8_t big_index)
{
	int ret;

	if (!broadcast_source_is_streaming(big_index)) {
		return 0;
	}

	ret = broadcast_source_disable(big_index);
	if (ret) {
		shell_error(shell, "Failed to stop broadcaster(s) %d", ret);
		return ret;
	}

	ret = bt_mgmt_per_adv_stop(big_index);
	if (ret) {
		shell_error(shell, "Failed to stop periodic advertiser");
		return ret;
	}

	ret = bt_mgmt_ext_adv_stop(big_index);
	if (ret) {
		shell_error(shell, "Failed to stop extended advertiser");
		return ret;
	}

	if (big_index == 0) {
		net_buf_simple_reset(&base_data0);
		net_buf_simple_reset(&uuid_data0);
	} else if (big_index == 1) {
		net_buf_simple_reset(&base_data1);
		net_buf_simple_reset(&uuid_data1);
	} else {
		shell_error(shell, "BIG index out of range");
		return -EINVAL;
	}

	per_adv_buf[big_index].data_len = 0;
	per_adv_buf[big_index].type = 0;

	memset(pba_data[big_index], 0, sizeof(pba_data[big_index]));

	for (size_t j = 0; j < ARRAY_SIZE(ext_adv_buf[big_index]); j++) {
		ext_adv_buf[big_index][j].data_len = 0;
		ext_adv_buf[big_index][j].type = 0;
	}

	return 0;
}

static int cmd_stop(const struct shell *shell, size_t argc, char **argv)
{
	int ret;

	if (argc == 2) {
		uint8_t big_index;

		ret = argv_to_indexes(shell, argc, argv, &big_index, 1, NULL, 0);
		if (ret) {
			return ret;
		}

		ret = broadcaster_stop(shell, big_index);

	} else {
		for (size_t i = 0; i < CONFIG_BT_ISO_MAX_BIG; i++) {
			if (broadcast_param[i].subgroups != NULL) {
				ret = broadcaster_stop(shell, i);
				if (ret) {
					return ret;
				}
			}
		}
	}

	led_on(LED_AUDIO_APP_STATUS, LED_COLOR_GREEN);

	return 0;
}

static int cmd_show(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	for (size_t i = 0; i < CONFIG_BT_ISO_MAX_BIG; i++) {
		if (broadcast_param[i].subgroups == NULL) {
			continue;
		}

		bool streaming = broadcast_source_is_streaming(i);

		shell_print(shell, "BIG %d:", i);
		shell_print(shell, "\tStreaming: %s", (streaming ? "true" : "false"));

		nrf_auraconfig_print(shell, i);
	}

	return 0;
}

static int cmd_preset(const struct shell *shell, size_t argc, char **argv)
{
	int ret;
	uint8_t big_index;

	if ((argc >= 2) && (strcmp(argv[1], "print") == 0)) {
		for (size_t i = 0; i < ARRAY_SIZE(bap_presets); i++) {
			shell_print(shell, "%s", bap_presets[i].name);
		}
		return 0;
	}

	if (argc < 3) {
		shell_error(shell,
			    "Usage: nac preset <preset> <BIG index> optional:<subgroup index>");
		return -EINVAL;
	}

	struct bt_bap_lc3_preset *preset = preset_find(argv[1]);

	if (!preset) {
		shell_error(shell,
			    "Preset not found, use 'nac preset print' to see available presets");
		return -EINVAL;
	}

	ret = argv_to_indexes(shell, argc, argv, &big_index, 2, NULL, 0);
	if (ret) {
		return ret;
	}

	broadcast_create(big_index);

	if (argc == 4) {
		uint8_t sub_index;

		ret = argv_to_indexes(shell, argc, argv, &big_index, 2, &sub_index, 3);

		broadcast_param[big_index].subgroups[sub_index].group_lc3_preset = *preset;
		broadcast_param[big_index].subgroups[sub_index].preset_name =
			preset_name_find(preset);
	} else {
		for (size_t i = 0; i < broadcast_param[big_index].num_subgroups; i++) {
			broadcast_param[big_index].subgroups[i].group_lc3_preset = *preset;
			broadcast_param[big_index].subgroups[i].preset_name =
				preset_name_find(preset);
		}
	}

	return 0;
}

static int cmd_packing(const struct shell *shell, size_t argc, char **argv)
{
	int ret;
	uint8_t big_index;

	if (argc < 3) {
		shell_error(shell, "Usage: nac packing <seq/int> <BIG index>");
		return -EINVAL;
	}

	ret = argv_to_indexes(shell, argc, argv, &big_index, 2, NULL, 0);
	if (ret) {
		return ret;
	}

	if (strcmp(argv[1], "seq") == 0) {
		broadcast_param[big_index].packing = BT_ISO_PACKING_SEQUENTIAL;
	} else if (strcmp(argv[1], "int") == 0) {
		broadcast_param[big_index].packing = BT_ISO_PACKING_INTERLEAVED;
	} else {
		shell_error(shell, "Invalid packing type");
		return -EINVAL;
	}

	return 0;
}

static int cmd_lang(const struct shell *shell, size_t argc, char **argv)
{
	int ret;
	uint8_t big_index;
	uint8_t sub_index;

	if (argc < 4) {
		shell_error(shell, "Usage: nac lang <language> <BIG index> <subgroup index>");
		return -EINVAL;
	}

	if (strlen(argv[1]) != LANGUAGE_LEN) {
		shell_error(shell, "Language must be %d characters long", LANGUAGE_LEN);
		return -EINVAL;
	}

	char *language = argv[1];

	ret = argv_to_indexes(shell, argc, argv, &big_index, 2, &sub_index, 3);
	if (ret) {
		return ret;
	}

	bt_audio_codec_cfg_meta_set_lang(
		&broadcast_param[big_index].subgroups[sub_index].group_lc3_preset.codec_cfg,
		language);

	return 0;
}

static int cmd_immediate_set(const struct shell *shell, size_t argc, char **argv)
{
	int ret;
	uint8_t big_index;
	uint8_t sub_index;

	if (argc < 3) {
		shell_error(shell, "Usage: nac immediate <0/1> <BIG index> <subgroup index>");
		return -EINVAL;
	}

	bool immediate = strtoul(argv[1], NULL, 10);

	if ((immediate != 0) && (immediate != 1)) {
		shell_error(shell, "Immediate must be 0 or 1");
		return -EINVAL;
	}

	ret = argv_to_indexes(shell, argc, argv, &big_index, 2, &sub_index, 3);
	if (ret) {
		return ret;
	}

	if (immediate) {
		ret = bt_audio_codec_cfg_meta_set_bcast_audio_immediate_rend_flag(
			&broadcast_param[big_index]
				 .subgroups[sub_index]
				 .group_lc3_preset.codec_cfg);
		if (ret < 0) {
			shell_error(shell, "Failed to set immediate rendering flag: %d", ret);
			return ret;
		}
	} else {
		ret = bt_audio_codec_cfg_meta_unset_val(
			&broadcast_param[big_index].subgroups[sub_index].group_lc3_preset.codec_cfg,
			BT_AUDIO_METADATA_TYPE_BROADCAST_IMMEDIATE);
		if (ret < 0) {
			shell_error(shell, "Failed to unset immediate rendering flag: %d", ret);
			return ret;
		}
	}

	return 0;
}

static int cmd_num_subgroups(const struct shell *shell, size_t argc, char **argv)
{
	int ret;
	uint8_t big_index;

	if (argc < 3) {
		shell_error(shell, "Usage: nac num_subgroups <num> <BIG index>");
		return -EINVAL;
	}

	if (!is_number(argv[1])) {
		shell_error(shell, "Number of subgroups must be a digit");
		return -EINVAL;
	}

	uint8_t num_subgroups = (uint8_t)atoi(argv[1]);

	if (num_subgroups > CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT) {
		shell_error(shell, "Max allowed subgroups is: %d",
			    CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT);
		return -EINVAL;
	}

	ret = argv_to_indexes(shell, argc, argv, &big_index, 2, NULL, 0);
	if (ret) {
		return ret;
	}

	broadcast_param[big_index].num_subgroups = num_subgroups;

	return 0;
}

static int cmd_num_bises(const struct shell *shell, size_t argc, char **argv)
{
	int ret;
	uint8_t big_index;
	uint8_t sub_index;

	if (argc < 4) {
		shell_error(shell, "Usage: nac num_bises <num> <BIG index> <subgroup index>");
		return -EINVAL;
	}

	if (!is_number(argv[1])) {
		shell_error(shell, "Number of BISes must be a digit");
		return -EINVAL;
	}

	uint8_t num_bises = (uint8_t)atoi(argv[1]);

	if (!is_number(argv[2])) {
		shell_error(shell, "BIG index must be a digit");
		return -EINVAL;
	}

	if (num_bises > CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT) {
		shell_error(shell, "Max allowed BISes is: %d",
			    CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT);
		return -EINVAL;
	}

	ret = argv_to_indexes(shell, argc, argv, &big_index, 2, &sub_index, 3);
	if (ret) {
		return ret;
	}

	broadcast_param[big_index].subgroups[sub_index].num_bises = num_bises;

	return 0;
}

static int cmd_context(const struct shell *shell, size_t argc, char **argv)
{
	int ret;
	uint8_t big_index;
	uint8_t sub_index;
	uint16_t context = 0;

	if ((argc >= 2) && (strcmp(argv[1], "print") == 0)) {
		context_print(shell);
		return 0;
	}

	if (argc < 4) {
		shell_error(shell, "Usage: nac context <context> <BIG index> <subgroup index>");
		return -EINVAL;
	}

	/* Context container is a bit field */
	for (size_t i = 0U; i <= context_msb_one(BT_AUDIO_CONTEXT_TYPE_ANY); i++) {
		if (strcasecmp(argv[1], bt_audio_context_bit_to_str(BIT(i))) == 0) {
			context = BIT(i);
			break;
		}
	}

	if (!context) {
		shell_error(shell, "Context not found, use 'nac context print' to see "
				   "available contexts");
		return -EINVAL;
	}

	ret = argv_to_indexes(shell, argc, argv, &big_index, 2, &sub_index, 3);
	if (ret) {
		return ret;
	}

	broadcast_param[big_index].subgroups[sub_index].context = context;

	return 0;
}

static int location_find(const struct shell *shell, const char *name)
{
	for (size_t i = 0; i < ARRAY_SIZE(locations); i++) {
		if (strcasecmp(locations[i].name, name) == 0) {
			return locations[i].location;
		}
	}

	shell_error(shell, "Location not found");

	return -ESRCH;
}

static void location_print(const struct shell *shell)
{
	for (size_t i = 0; i < ARRAY_SIZE(locations); i++) {
		shell_print(shell, "%s - %s", locations[i].name,
			    bt_audio_location_bit_to_str(locations[i].location));
	}
}

static int cmd_location(const struct shell *shell, size_t argc, char **argv)
{
	int ret;
	uint8_t big_index;
	uint8_t sub_index;

	if ((argc >= 2) && (strcmp(argv[1], "print") == 0)) {
		location_print(shell);
		return 0;
	}

	if (argc < 4) {
		shell_error(shell, "Usage: nac location <location> <BIG index> <subgroup "
				   "index> <BIS index>");
		return -EINVAL;
	}

	ret = argv_to_indexes(shell, argc, argv, &big_index, 2, &sub_index, 3);
	if (ret) {
		return ret;
	}

	if (!is_number(argv[4])) {
		shell_error(shell, "BIS index must be a digit");
		return -EINVAL;
	}

	uint8_t bis_index = (uint8_t)atoi(argv[4]);

	if ((bis_index >= CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT) ||
	    (bis_index >= broadcast_param[big_index].subgroups[sub_index].num_bises)) {
		shell_error(shell, "BIS index out of range");
		return -EINVAL;
	}

	int location = location_find(shell, argv[1]);

	if (location < 0) {
		shell_error(shell, "Location not found");
		return -EINVAL;
	}

	broadcast_param[big_index].subgroups[sub_index].location[bis_index] = location;

	return 0;
}

static int cmd_broadcast_name(const struct shell *shell, size_t argc, char **argv)
{
	int ret;
	uint8_t big_index;

	if (argc < 2) {
		shell_error(shell, "Usage: nac broadcast_name <name> <BIG index>");
		return -EINVAL;
	}

	ret = argv_to_indexes(shell, argc, argv, &big_index, 2, NULL, 0);
	if (ret) {
		return ret;
	}

	size_t name_length = strlen(argv[1]);

	/* Leave space for null termination */
	if (name_length >= ARRAY_SIZE(broadcast_param[big_index].broadcast_name) - 1) {
		shell_error(shell, "Name too long");
		return -EINVAL;
	}

	/* Delete old name if set */
	memset(broadcast_param[big_index].broadcast_name, '\0',
	       ARRAY_SIZE(broadcast_param[big_index].broadcast_name));
	memcpy(broadcast_param[big_index].broadcast_name, argv[1], name_length);

	return 0;
}

static int cmd_encrypt(const struct shell *shell, size_t argc, char **argv)
{
	int ret;
	uint8_t big_index;

	if (argc < 3) {
		shell_error(shell,
			    "Usage: nac encrypt <0/1> <BIG index> Optional:<broadcast_code>");
		return -EINVAL;
	}

	if (!is_number(argv[1])) {
		shell_error(shell, "Encryption must be 0 or 1");
		return -EINVAL;
	}

	uint8_t encrypt = (uint8_t)atoi(argv[1]);

	if (encrypt != 1 && encrypt != 0) {
		shell_error(shell, "Encryption must be 0 or 1");
		return -EINVAL;
	}

	ret = argv_to_indexes(shell, argc, argv, &big_index, 2, NULL, 0);
	if (ret) {
		return ret;
	}

	broadcast_param[big_index].encryption = encrypt;

	if (encrypt == 1) {
		if (argc < 4) {
			shell_error(shell, "Broadcast code must be set");
			return -EINVAL;
		}

		if (strlen(argv[3]) > BT_ISO_BROADCAST_CODE_SIZE) {
			shell_error(shell, "Broadcast code must be %d characters long",
				    BT_ISO_BROADCAST_CODE_SIZE);
			return -EINVAL;
		}
		memset(broadcast_param[big_index].broadcast_code, '\0',
		       ARRAY_SIZE(broadcast_param[big_index].broadcast_code));
		memcpy(broadcast_param[big_index].broadcast_code, argv[3], strlen(argv[3]));
	}

	return 0;
}

static int cmd_adv_name(const struct shell *shell, size_t argc, char **argv)
{
	int ret;
	uint8_t big_index;

	if (argc < 2) {
		shell_error(shell, "Usage: nac device_name <name> <BIG index>");
		return -EINVAL;
	}

	if (strlen(argv[1]) > CONFIG_BT_DEVICE_NAME_MAX) {
		shell_error(shell, "Device name too long");
		return -EINVAL;
	}

	ret = argv_to_indexes(shell, argc, argv, &big_index, 2, NULL, 0);
	if (ret) {
		return ret;
	}

	memset(broadcast_param[big_index].adv_name, '\0',
	       ARRAY_SIZE(broadcast_param[big_index].adv_name));
	memcpy(broadcast_param[big_index].adv_name, argv[1], strlen(argv[1]));

	return 0;
}

static int cmd_program_info(const struct shell *shell, size_t argc, char **argv)
{
	int ret;
	uint8_t big_index;
	uint8_t sub_index;

	if (argc != 4) {
		shell_error(shell, "Usage: nac program_info \"info\" <BIG index> <subgroup index>");
		return -EINVAL;
	}

	ret = argv_to_indexes(shell, argc, argv, &big_index, 2, &sub_index, 3);
	if (ret) {
		return ret;
	}

	ret = bt_audio_codec_cfg_meta_set_program_info(
		&broadcast_param[big_index].subgroups[sub_index].group_lc3_preset.codec_cfg,
		argv[1], strlen(argv[1]));
	if (ret < 0) {
		shell_error(shell, "Failed to set program info: %d", ret);
		return ret;
	}

	return 0;
}

#define FILE_LIST_BUF_SIZE 1024

static int cmd_file_list(const struct shell *shell, size_t argc, char **argv)
{

	int ret;
	char buf[FILE_LIST_BUF_SIZE];
	size_t buf_size = FILE_LIST_BUF_SIZE;
	char *dir_path = NULL;

	if (!sd_card_present) {
		shell_error(shell, "No SD card present: files cannot be listed");
		return -EFAULT;
	}

	if (argc > 2) {
		shell_error(shell, "Usage: nac file list [dir path]");
		return -EINVAL;
	}

	if (argc == 2) {
		dir_path = argv[1];
	}

	ret = sd_card_list_files(dir_path, buf, &buf_size, true);
	if (ret) {
		shell_error(shell, "List files err: %d", ret);
		return ret;
	}

	shell_print(shell, "%s", buf);

	return 0;
}

static int cmd_file_select(const struct shell *shell, size_t argc, char **argv)
{
	int ret;
	uint8_t big_index;
	uint8_t sub_index;
	uint8_t bis_index;

	if (!sd_card_present) {
		shell_error(shell, "No SD card present: files cannot be selected");
		return -EFAULT;
	}

	if (argc != 5) {
		shell_error(shell,
			    "Usage: nac file select <file path> <BIG index> <subgroup index> "
			    "<BIS index>");
		return -EINVAL;
	}

	ret = argv_to_indexes(shell, argc, argv, &big_index, 2, &sub_index, 3);
	if (ret) {
		shell_error(shell, "Failed to get indexes: %d", ret);
		return ret;
	}

	if (broadcast_source_is_streaming(big_index)) {
		shell_error(shell, "Files cannot be selected while streaming");
		return -EFAULT;
	}

	if (!is_number(argv[4])) {
		shell_error(shell, "BIS index must be a digit");
		return -EINVAL;
	}

	bis_index = (uint8_t)atoi(argv[4]);

	if ((bis_index >= CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT) ||
	    (bis_index >= broadcast_param[big_index].subgroups[sub_index].num_bises)) {
		shell_error(shell, "BIS index %d is out of range (max %d)", bis_index,
			    broadcast_param[big_index].subgroups[sub_index].num_bises - 1);
		return -EINVAL;
	}

	char *file_name = argv[1];

	LOG_DBG("Selecting file %s for stream big: %d sub: %d bis: %d", file_name, big_index,
		sub_index, bis_index);

	struct bt_audio_codec_cfg *codec_cfg =
		&broadcast_param[big_index].subgroups[sub_index].group_lc3_preset.codec_cfg;

	struct lc3_stream_cfg cfg;

	ret = le_audio_freq_hz_get(codec_cfg, &cfg.sample_rate_hz);
	if (ret) {
		shell_error(shell, "Failed to get frequency: %d", ret);
		return ret;
	}

	ret = le_audio_duration_us_get(codec_cfg, &cfg.frame_duration_us);
	if (ret) {
		shell_error(shell, "Failed to get frame duration: %d", ret);
	}

	ret = le_audio_bitrate_get(codec_cfg, &cfg.bit_rate_bps);
	if (ret) {
		shell_error(shell, "Failed to get bit rate: %d", ret);
	}

	/* Verify that the file header matches the stream configurationn */
	/* NOTE: This will not abort the streamer if the file is not valid, only give a warning */
	bool header_valid = lc3_streamer_file_compatible_check(file_name, &cfg);

	if (!header_valid) {
		shell_warn(shell, "File header verification failed. File may not be compatible "
				  "with stream config.");
	}

	ret = lc3_streamer_stream_register(
		file_name, &lc3_stream_infos[big_index][sub_index].lc3_streamer_idx[bis_index],
		true);
	if (ret) {
		shell_error(shell, "Failed to register stream: %d", ret);
		return ret;
	}

	lc3_stream_infos[big_index][sub_index].frame_size =
		broadcast_param[big_index].subgroups[sub_index].group_lc3_preset.qos.sdu;

	return 0;
}

static int cmd_phy(const struct shell *shell, size_t argc, char **argv)
{
	int ret;
	uint8_t big_index;
	uint8_t sub_index;

	if (argc != 4) {
		shell_error(shell,
			    "Usage: nac phy <1, 2 or 4 (coded)> <BIG index> <subgroup index>");
		return -EINVAL;
	}

	if (!is_number(argv[1])) {
		shell_error(shell, "PHY must be a digit");
		return -EINVAL;
	}

	uint8_t phy = (uint8_t)atoi(argv[1]);

	if (phy != BT_BAP_QOS_CFG_1M && phy != BT_BAP_QOS_CFG_2M && phy != BT_BAP_QOS_CFG_CODED) {
		shell_error(shell, "Invalid PHY");
		return -EINVAL;
	}

	ret = argv_to_indexes(shell, argc, argv, &big_index, 2, &sub_index, 3);
	if (ret) {
		return ret;
	}

	broadcast_param[big_index].subgroups[sub_index].group_lc3_preset.qos.phy = phy;
	broadcast_param[big_index].subgroups[sub_index].preset_name = "Custom";

	return 0;
}

static int cmd_framing(const struct shell *shell, size_t argc, char **argv)
{
	int ret;
	uint8_t big_index;
	uint8_t sub_index;

	if (argc != 4) {
		shell_error(shell,
			    "Usage: nac framing <unframed/framed> <BIG index> <subgroup index>");
		return -EINVAL;
	}

	ret = argv_to_indexes(shell, argc, argv, &big_index, 2, &sub_index, 3);
	if (ret) {
		return ret;
	}

	if (strcasecmp(argv[1], "unframed") == 0) {
		broadcast_param[big_index].subgroups[sub_index].group_lc3_preset.qos.framing =
			BT_BAP_QOS_CFG_FRAMING_UNFRAMED;
		broadcast_param[big_index].subgroups[sub_index].preset_name = "Custom";
	} else if (strcasecmp(argv[1], "framed") == 0) {
		broadcast_param[big_index].subgroups[sub_index].group_lc3_preset.qos.framing =
			BT_BAP_QOS_CFG_FRAMING_FRAMED;
		broadcast_param[big_index].subgroups[sub_index].preset_name = "Custom";
	} else {
		shell_error(shell, "Invalid framing type");
		return -EINVAL;
	}

	return 0;
}

static int cmd_rtn(const struct shell *shell, size_t argc, char **argv)
{
	int ret;
	uint8_t big_index;
	uint8_t sub_index;

	if (argc != 4) {
		shell_error(shell, "Usage: nac rtn <num> <BIG index> <subgroup index>");
		return -EINVAL;
	}

	if (!is_number(argv[1])) {
		shell_error(shell, "RTN must be a digit");
		return -EINVAL;
	}

	if (strtoul(argv[1], NULL, 10) > UINT8_MAX) {
		shell_error(shell, "RTN must be less than %d", UINT8_MAX);
		return -EINVAL;
	}

	uint8_t rtn = (uint8_t)atoi(argv[1]);

	ret = argv_to_indexes(shell, argc, argv, &big_index, 2, &sub_index, 3);
	if (ret) {
		return ret;
	}

	broadcast_param[big_index].subgroups[sub_index].group_lc3_preset.qos.rtn = rtn;
	broadcast_param[big_index].subgroups[sub_index].preset_name = "Custom";

	return 0;
}

static int cmd_sdu(const struct shell *shell, size_t argc, char **argv)
{
	int ret;
	uint8_t big_index;
	uint8_t sub_index;

	if (argc != 4) {
		shell_error(shell, "Usage: nac sdu <octets> <BIG index> <subgroup index>");
		return -EINVAL;
	}

	if (!is_number(argv[1])) {
		shell_error(shell, "SDU must be a digit");
		return -EINVAL;
	}

	if (strtoul(argv[1], NULL, 10) > UINT16_MAX) {
		shell_error(shell, "SDU must be less than %d", UINT16_MAX);
		return -EINVAL;
	}

	uint16_t sdu = (uint16_t)atoi(argv[1]);

	ret = argv_to_indexes(shell, argc, argv, &big_index, 2, &sub_index, 3);
	if (ret) {
		return ret;
	}

	broadcast_param[big_index].subgroups[sub_index].group_lc3_preset.qos.sdu = sdu;
	broadcast_param[big_index].subgroups[sub_index].preset_name = "Custom";

	return 0;
}

static int cmd_mtl(const struct shell *shell, size_t argc, char **argv)
{
	int ret;
	uint8_t big_index;
	uint8_t sub_index;

	if (argc != 4) {
		shell_error(shell, "Usage: nac mtl <time in ms> <BIG index> <subgroup index>");
		return -EINVAL;
	}

	if (!is_number(argv[1])) {
		shell_error(shell, "MTL must be a digit");
		return -EINVAL;
	}

	if (strtoul(argv[1], NULL, 10) > UINT16_MAX) {
		shell_error(shell, "MTL must be less than %d", UINT16_MAX);
		return -EINVAL;
	}

	uint16_t mtl = (uint16_t)atoi(argv[1]);

	ret = argv_to_indexes(shell, argc, argv, &big_index, 2, &sub_index, 3);
	if (ret) {
		return ret;
	}

	broadcast_param[big_index].subgroups[sub_index].group_lc3_preset.qos.latency = mtl;
	broadcast_param[big_index].subgroups[sub_index].preset_name = "Custom";

	return 0;
}

static int cmd_frame_interval(const struct shell *shell, size_t argc, char **argv)
{
	int ret;
	uint8_t big_index;
	uint8_t sub_index;

	if (argc != 4) {
		shell_error(shell,
			    "Usage: nac frame_interval <time in us> <BIG index> <subgroup index>");
		return -EINVAL;
	}

	if (!is_number(argv[1])) {
		shell_error(shell, "Frame interval must be a digit");
		return -EINVAL;
	}

	if (strtoul(argv[1], NULL, 10) > UINT32_MAX) {
		shell_error(shell, "Frame interval must be less than %ld", UINT32_MAX);
		return -EINVAL;
	}

	uint32_t frame_interval = (uint32_t)atoi(argv[1]);

	if (frame_interval != 7500 && frame_interval != 10000) {
		shell_error(shell, "Frame interval must be 7500 or 10000");
		return -EINVAL;
	}

	ret = argv_to_indexes(shell, argc, argv, &big_index, 2, &sub_index, 3);
	if (ret) {
		return ret;
	}

	broadcast_param[big_index].subgroups[sub_index].group_lc3_preset.qos.interval =
		frame_interval;
	broadcast_param[big_index].subgroups[sub_index].preset_name = "Custom";

	return 0;
}

static int cmd_pd(const struct shell *shell, size_t argc, char **argv)
{
	int ret;
	uint8_t big_index;
	uint8_t sub_index;

	if (argc != 4) {
		shell_error(shell, "Usage: nac pd <time us> <BIG index> <subgroup index>");
		return -EINVAL;
	}

	if (!is_number(argv[1])) {
		shell_error(shell, "presentation delay must be a digit");
		return -EINVAL;
	}

	if (strtoul(argv[1], NULL, 10) > UINT32_MAX) {
		shell_error(shell, "Presentation delay must be less than %ld", UINT32_MAX);
		return -EINVAL;
	}

	uint32_t pd = (uint32_t)atoi(argv[1]);

	ret = argv_to_indexes(shell, argc, argv, &big_index, 2, &sub_index, 3);
	if (ret) {
		return ret;
	}

	broadcast_param[big_index].subgroups[sub_index].group_lc3_preset.qos.pd = pd;
	broadcast_param[big_index].subgroups[sub_index].preset_name = "Custom";

	return 0;
}

static int cmd_random_id(const struct shell *shell, size_t argc, char **argv)
{
	uint8_t big_index;

	if (argc < 2) {
		shell_error(shell, "Usage: nac broadcast_id random <BIG index>");
		return -EINVAL;
	}

	if (!is_number(argv[1])) {
		shell_error(shell, "BIG index must be a digit");
		return -EINVAL;
	}

	big_index = (uint8_t)atoi(argv[1]);

	if (big_index >= CONFIG_BT_ISO_MAX_BIG) {
		shell_error(shell, "BIG index out of range");
		return -EINVAL;
	}

	if (broadcast_source_is_streaming(big_index)) {
		shell_error(shell, "Broadcast ID can not be changed while streaming");
		return -EFAULT;
	}

	broadcast_param[big_index].fixed_id = false;

	return 0;
}

static int cmd_fixed_id(const struct shell *shell, size_t argc, char **argv)
{
	uint8_t big_index;

	if (argc < 3) {
		shell_error(shell, "Usage: nac broadcast_id fixed <BIG index> <broadcast_id in hex "
				   "(3 octets)>");
		return -EINVAL;
	}

	if (!is_number(argv[1])) {
		shell_error(shell, "BIG index must be a digit");
		return -EINVAL;
	}

	big_index = (uint8_t)atoi(argv[1]);

	if (big_index >= CONFIG_BT_ISO_MAX_BIG) {
		shell_error(shell, "BIG index out of range");
		return -EINVAL;
	}

	if (broadcast_source_is_streaming(big_index)) {
		shell_error(shell, "Broadcast ID can not be changed while streaming");
		return -EFAULT;
	}

	uint8_t len;

	if (!is_hex(argv[2], &len)) {
		shell_error(shell, "Broadcast ID must be a hexadecimal value");
		return -EINVAL;
	}

	if (len > 6) {
		shell_error(shell, "Broadcast ID must be three octets long at maximum");
		return -EINVAL;
	}

	uint32_t broadcast_id = (uint32_t)strtoul(argv[2], NULL, 16);

	broadcast_param[big_index].broadcast_id = broadcast_id;

	broadcast_param[big_index].fixed_id = true;

	return 0;
}

static int usecase_find(const struct shell *shell, const char *name)
{
	for (size_t i = 0; i < ARRAY_SIZE(pre_defined_use_cases); i++) {
		if (strcasecmp(pre_defined_use_cases[i].name, name) == 0) {
			return pre_defined_use_cases[i].use_case;
		}
	}

	shell_error(shell, "Use case not found");

	return -ESRCH;
}

static void usecase_print(const struct shell *shell)
{
	for (size_t i = 0; i < ARRAY_SIZE(pre_defined_use_cases); i++) {
		shell_print(shell, "%d - %s", pre_defined_use_cases[i].use_case,
			    pre_defined_use_cases[i].name);
	}
}

static void lecture_set(const struct shell *shell)
{
	char *preset_argv[3] = {"preset", "24_2_1", "0"};
	char *adv_name_argv[3] = {"adv_name", "Lecture hall", "0"};
	char *name_argv[3] = {"broadcast_name", "Lecture", "0"};
	char *broadcast_id_argv[3] = {"fixed", "0", "0x123456"};
	char *packing_argv[3] = {"packing", "int", "0"};

	char *lang_argv[4] = {"lang", "eng", "0", "0"};

	char *context_argv[4] = {"context", "live", "0", "0"};

	char *program_info_argv[4] = {"program_info", "Mathematics 101", "0", "0"};

	char *imm_argv[4] = {"immediate", "1", "0", "0"};

	char *num_bis_argv[4] = {"num_bises", "1", "0", "0"};

	char *fileselect_argv[5] = {
		"file select", "10ms/24000hz/48_kbps/auditorium-english_24kHz_left_48kbps_10ms.lc3",
		"0", "0", "0"};

	cmd_preset(shell, 3, preset_argv);
	cmd_adv_name(shell, 3, adv_name_argv);
	cmd_broadcast_name(shell, 3, name_argv);
	cmd_fixed_id(shell, 3, broadcast_id_argv);
	cmd_packing(shell, 3, packing_argv);

	cmd_lang(shell, 4, lang_argv);

	cmd_context(shell, 4, context_argv);

	cmd_program_info(shell, 4, program_info_argv);

	cmd_immediate_set(shell, 4, imm_argv);

	cmd_num_bises(shell, 4, num_bis_argv);

	if (sd_card_present) {
		cmd_file_select(shell, 5, fileselect_argv);
	}
}

static void silent_tv_1_set(const struct shell *shell)
{
	char *preset_argv[3] = {"preset", "24_2_1", "0"};
	char *adv_name_argv[3] = {"adv_name", "Silent TV1", "0"};
	char *name_argv[3] = {"broadcast_name", "TV-audio", "0"};
	char *packing_argv[3] = {"packing", "int", "0"};
	char *encrypt_argv[4] = {"encrypt", "1", "0", "Auratest"};

	char *context_argv[4] = {"context", "media", "0", "0"};

	char *program_info_argv[4] = {"program_info", "News", "0", "0"};

	char *imm_argv[4] = {"immediate", "1", "0", "0"};

	char *num_bis_argv[4] = {"num_bises", "2", "0", "0"};

	char *location_FL_argv[5] = {"location", "FL", "0", "0", "0"};
	char *location_FR_argv[5] = {"location", "FR", "0", "0", "1"};

	char *fileselect0_argv[5] = {"file select",
				     "10ms/24000hz/48_kbps/left-channel_24kHz_left_48kbps_10ms.lc3",
				     "0", "0", "0"};
	char *fileselect1_argv[5] = {
		"file select", "10ms/24000hz/48_kbps/right-channel_24kHz_left_48kbps_10ms.lc3", "0",
		"0", "1"};

	cmd_preset(shell, 3, preset_argv);
	cmd_adv_name(shell, 3, adv_name_argv);
	cmd_broadcast_name(shell, 3, name_argv);
	cmd_packing(shell, 3, packing_argv);
	cmd_encrypt(shell, 4, encrypt_argv);

	cmd_context(shell, 4, context_argv);

	cmd_program_info(shell, 4, program_info_argv);

	cmd_immediate_set(shell, 4, imm_argv);

	cmd_num_bises(shell, 4, num_bis_argv);

	cmd_location(shell, 5, location_FL_argv);
	cmd_location(shell, 5, location_FR_argv);

	if (sd_card_present) {
		cmd_file_select(shell, 5, fileselect0_argv);
		cmd_file_select(shell, 5, fileselect1_argv);
	}
}

static void silent_tv_2_set(const struct shell *shell)
{
	/* BIG0 */
	char *preset0_argv[3] = {"preset", "24_2_1", "0"};
	char *adv_name0_argv[3] = {"adv_name", "Silent_TV2_std", "0"};
	char *name0_argv[3] = {"broadcast_name", "TV - standard qual.", "0"};
	char *packing0_argv[3] = {"packing", "int", "0"};
	char *encrypt0_argv[4] = {"encrypt", "1", "0", "Auratest"};

	char *context0_argv[4] = {"context", "media", "0", "0"};

	char *program_info0_argv[4] = {"program_info", "Biathlon", "0", "0"};

	char *imm0_argv[4] = {"immediate", "1", "0", "0"};

	char *num_bis0_argv[4] = {"num_bises", "2", "0", "0"};

	char *location_FL0_argv[5] = {"location", "FL", "0", "0", "0"};
	char *location_FR0_argv[5] = {"location", "FR", "0", "0", "1"};

	char *fileselect00_argv[5] = {
		"file select", "10ms/24000hz/48_kbps/left-channel_24kHz_left_48kbps_10ms.lc3", "0",
		"0", "0"};
	char *fileselect01_argv[5] = {
		"file select", "10ms/24000hz/48_kbps/right-channel_24kHz_left_48kbps_10ms.lc3", "0",
		"0", "1"};

	cmd_preset(shell, 3, preset0_argv);
	cmd_adv_name(shell, 3, adv_name0_argv);
	cmd_broadcast_name(shell, 3, name0_argv);
	cmd_packing(shell, 3, packing0_argv);
	cmd_encrypt(shell, 4, encrypt0_argv);

	cmd_context(shell, 4, context0_argv);

	cmd_program_info(shell, 4, program_info0_argv);

	cmd_immediate_set(shell, 4, imm0_argv);

	cmd_num_bises(shell, 4, num_bis0_argv);

	cmd_location(shell, 5, location_FL0_argv);
	cmd_location(shell, 5, location_FR0_argv);

	if (sd_card_present) {
		cmd_file_select(shell, 5, fileselect00_argv);
		cmd_file_select(shell, 5, fileselect01_argv);
	}

	/* BIG1 */
	char *preset1_argv[3] = {"preset", "48_2_1", "1"};
	char *adv_name1_argv[3] = {"adv_name", "Silent_TV2_high", "1"};
	char *name1_argv[3] = {"broadcast_name", "TV - high qual.", "1"};
	char *packing1_argv[3] = {"packing", "int", "1"};
	char *encrypt1_argv[4] = {"encrypt", "1", "1", "Auratest"};

	char *context1_argv[4] = {"context", "media", "1", "0"};

	char *program_info1_argv[4] = {"program_info", "Biathlon", "1", "0"};

	char *imm1_argv[4] = {"immediate", "1", "1", "0"};

	char *num_bis1_argv[4] = {"num_bises", "2", "1", "0"};

	char *location_FL1_argv[5] = {"location", "FL", "1", "0", "0"};
	char *location_FR1_argv[5] = {"location", "FR", "1", "0", "1"};

	char *fileselect10_argv[5] = {
		"file select", "10ms/48000hz/80_kbps/left-channel_48kHz_left_80kbps_10ms.lc3", "1",
		"0", "0"};
	char *fileselect11_argv[5] = {
		"file select", "10ms/48000hz/80_kbps/right-channel_48kHz_left_80kbps_10ms.lc3", "1",
		"0", "1"};
	char *num_rtn_argv[4] = {"rtn", "2", "1", "0"};

	cmd_preset(shell, 3, preset1_argv);
	cmd_adv_name(shell, 3, adv_name1_argv);
	cmd_broadcast_name(shell, 3, name1_argv);
	cmd_packing(shell, 3, packing1_argv);
	cmd_encrypt(shell, 4, encrypt1_argv);

	cmd_context(shell, 4, context1_argv);

	cmd_program_info(shell, 4, program_info1_argv);

	cmd_immediate_set(shell, 4, imm1_argv);

	cmd_num_bises(shell, 4, num_bis1_argv);

	cmd_location(shell, 5, location_FL1_argv);
	cmd_location(shell, 5, location_FR1_argv);

	cmd_rtn(shell, 4, num_rtn_argv);

	if (sd_card_present) {
		cmd_file_select(shell, 5, fileselect10_argv);
		cmd_file_select(shell, 5, fileselect11_argv);
	}
}

static void multi_language_set(const struct shell *shell)
{
	char *num_subs_argv[3] = {"num_subgroups", "3", "0"};
	char *preset_argv[3] = {"preset", "24_2_2", "0"};
	char *adv_name_argv[3] = {"adv_name", "Multi-language", "0"};
	char *name_argv[3] = {"broadcast_name", "Multi-language", "0"};
	char *packing_argv[3] = {"packing", "int", "0"};

	char *lang0_argv[4] = {"lang", "eng", "0", "0"};
	char *lang1_argv[4] = {"lang", "chi", "0", "1"};
	char *lang2_argv[4] = {"lang", "nor", "0", "2"};

	char *context0_argv[4] = {"context", "unspecified", "0", "0"};
	char *context1_argv[4] = {"context", "unspecified", "0", "1"};
	char *context2_argv[4] = {"context", "unspecified", "0", "2"};

	char *num_bis0_argv[4] = {"num_bises", "1", "0", "0"};
	char *num_bis1_argv[4] = {"num_bises", "1", "0", "1"};
	char *num_bis2_argv[4] = {"num_bises", "1", "0", "2"};

	char *num_rtn0_argv[4] = {"rtn", "2", "0", "0"};
	char *num_rtn1_argv[4] = {"rtn", "2", "0", "1"};
	char *num_rtn2_argv[4] = {"rtn", "2", "0", "2"};

	char *fileselect0_argv[5] = {
		"file select", "10ms/24000hz/48_kbps/gate-b23-english_24kHz_left_48kbps_10ms.lc3",
		"0", "0", "0"};
	char *fileselect1_argv[5] = {
		"file select", "10ms/24000hz/48_kbps/gate-b23-mandarin_24kHz_left_48kbps_10ms.lc3",
		"0", "1", "0"};
	char *fileselect2_argv[5] = {
		"file select",
		"10ms/24000hz/48_kbps/adventuresherlockholmes_01_doyle_24kHz_left_48kbps_10ms.lc3",
		"0", "2", "0"};

	cmd_num_subgroups(shell, 3, num_subs_argv);
	cmd_preset(shell, 3, preset_argv);
	cmd_adv_name(shell, 3, adv_name_argv);
	cmd_broadcast_name(shell, 3, name_argv);
	cmd_packing(shell, 3, packing_argv);

	cmd_lang(shell, 4, lang0_argv);
	cmd_lang(shell, 4, lang1_argv);
	cmd_lang(shell, 4, lang2_argv);

	cmd_context(shell, 4, context0_argv);
	cmd_context(shell, 4, context1_argv);
	cmd_context(shell, 4, context2_argv);

	cmd_num_bises(shell, 4, num_bis0_argv);
	cmd_num_bises(shell, 4, num_bis1_argv);
	cmd_num_bises(shell, 4, num_bis2_argv);

	cmd_rtn(shell, 4, num_rtn0_argv);
	cmd_rtn(shell, 4, num_rtn1_argv);
	cmd_rtn(shell, 4, num_rtn2_argv);

	if (sd_card_present) {
		cmd_file_select(shell, 5, fileselect0_argv);
		cmd_file_select(shell, 5, fileselect1_argv);
		cmd_file_select(shell, 5, fileselect2_argv);
	}
}

static void personal_sharing_set(const struct shell *shell)
{
	char *preset_argv[3] = {"preset", "48_2_2", "0"};
	char *adv_name_argv[3] = {"adv_name", "John's phone", "0"};
	char *name_argv[3] = {"broadcast_name", "Personal sharing", "0"};
	char *packing_argv[3] = {"packing", "int", "0"};
	char *encrypt_argv[4] = {"encrypt", "1", "0", "Auratest"};

	char *context_argv[4] = {"context", "media", "0", "0"};

	char *num_bis_argv[4] = {"num_bises", "2", "0", "0"};

	char *location_FL_argv[5] = {"location", "FL", "0", "0", "0"};
	char *location_FR_argv[5] = {"location", "FR", "0", "0", "1"};

	char *fileselect0_argv[5] = {
		"file select",
		"10ms/48000hz/80_kbps/groovy-ambient-funk-201745_48kHz_left_80kbps_10ms.lc3", "0",
		"0", "0"};
	char *fileselect1_argv[5] = {
		"file select",
		"10ms/48000hz/80_kbps/groovy-ambient-funk-201745_48kHz_right_80kbps_10ms.lc3", "0",
		"0", "1"};

	cmd_preset(shell, 3, preset_argv);
	cmd_adv_name(shell, 3, adv_name_argv);
	cmd_broadcast_name(shell, 3, name_argv);
	cmd_packing(shell, 3, packing_argv);
	cmd_encrypt(shell, 4, encrypt_argv);

	cmd_context(shell, 4, context_argv);

	cmd_num_bises(shell, 4, num_bis_argv);

	cmd_location(shell, 5, location_FL_argv);
	cmd_location(shell, 5, location_FR_argv);

	if (sd_card_present) {
		cmd_file_select(shell, 5, fileselect0_argv);
		cmd_file_select(shell, 5, fileselect1_argv);
	}
}

static void personal_multi_language_set(const struct shell *shell)
{
	char *num_subs_argv[3] = {"num_subgroups", "2", "0"};
	char *preset_argv[3] = {"preset", "24_2_2", "0"};
	char *adv_name_argv[3] = {"adv_name", "Brian's laptop", "0"};
	char *name_argv[3] = {"broadcast_name", "Personal multi-lang.", "0"};
	char *packing_argv[3] = {"packing", "int", "0"};
	char *encrypt_argv[4] = {"encrypt", "1", "0", "Auratest"};

	char *lang0_argv[4] = {"lang", "eng", "0", "0"};
	char *lang1_argv[4] = {"lang", "chi", "0", "1"};

	char *context0_argv[4] = {"context", "media", "0", "0"};
	char *context1_argv[4] = {"context", "media", "0", "1"};

	char *num_bis0_argv[4] = {"num_bises", "2", "0", "0"};
	char *num_bis1_argv[4] = {"num_bises", "2", "0", "1"};

	char *location_FL0_argv[5] = {"location", "FL", "0", "0", "0"};
	char *location_FR0_argv[5] = {"location", "FR", "0", "0", "1"};
	char *location_FL1_argv[5] = {"location", "FL", "0", "1", "0"};
	char *location_FR1_argv[5] = {"location", "FR", "0", "1", "1"};

	char *fileselect000_argv[5] = {
		"file select", "10ms/24000hz/48_kbps/auditorium-english_24kHz_left_48kbps_10ms.lc3",
		"0", "0", "0"};
	char *fileselect001_argv[5] = {
		"file select",
		"10ms/24000hz/48_kbps/auditorium-english_24kHz_right_48kbps_10ms.lc3", "0", "0",
		"1"};
	char *fileselect010_argv[5] = {
		"file select",
		"10ms/24000hz/48_kbps/auditorium-mandarin_24kHz_left_48kbps_10ms.lc3", "0", "1",
		"0"};
	char *fileselect011_argv[5] = {
		"file select",
		"10ms/24000hz/48_kbps/auditorium-mandarin_24kHz_right_48kbps_10ms.lc3", "0", "1",
		"1"};

	char *num_rtn000_argv[4] = {"rtn", "2", "0", "0"};
	char *num_rtn010_argv[4] = {"rtn", "2", "0", "1"};

	cmd_num_subgroups(shell, 3, num_subs_argv);
	cmd_preset(shell, 3, preset_argv);
	cmd_adv_name(shell, 3, adv_name_argv);
	cmd_broadcast_name(shell, 3, name_argv);
	cmd_packing(shell, 3, packing_argv);
	cmd_encrypt(shell, 4, encrypt_argv);

	cmd_lang(shell, 4, lang0_argv);
	cmd_lang(shell, 4, lang1_argv);

	cmd_context(shell, 4, context0_argv);
	cmd_context(shell, 4, context1_argv);

	cmd_num_bises(shell, 4, num_bis0_argv);
	cmd_num_bises(shell, 4, num_bis1_argv);

	cmd_location(shell, 5, location_FL0_argv);
	cmd_location(shell, 5, location_FR0_argv);
	cmd_location(shell, 5, location_FL1_argv);
	cmd_location(shell, 5, location_FR1_argv);

	cmd_rtn(shell, 4, num_rtn000_argv);
	cmd_rtn(shell, 4, num_rtn010_argv);

	if (sd_card_present) {
		cmd_file_select(shell, 5, fileselect000_argv);
		cmd_file_select(shell, 5, fileselect001_argv);
		cmd_file_select(shell, 5, fileselect010_argv);
		cmd_file_select(shell, 5, fileselect011_argv);
	}
}

static int cmd_usecase(const struct shell *shell, size_t argc, char **argv)
{
	if ((argc >= 2) && (strcmp(argv[1], "print") == 0)) {
		usecase_print(shell);
		return 0;
	}

	if (argc < 2) {
		shell_error(shell, "Usage: nac usecase <use_case> - Either index or text");
		return -EINVAL;
	}

	int use_case_nr = -1;

	if (is_number(argv[1])) {
		use_case_nr = atoi(argv[1]);
	} else {
		use_case_nr = usecase_find(shell, argv[1]);

		if (use_case_nr < 0) {
			return -EINVAL;
		}
	}

	if (use_case_nr > ARRAY_SIZE(pre_defined_use_cases)) {
		shell_error(shell, "Use case not found");
		return -EINVAL;
	}

	/* Clear previous configurations */
	nrf_auraconfig_clear();

	switch (use_case_nr) {
	case LECTURE:
		lecture_set(shell);
		break;
	case SILENT_TV_1:
		silent_tv_1_set(shell);
		break;
	case SILENT_TV_2:
		silent_tv_2_set(shell);
		break;
	case MULTI_LANGUAGE:
		multi_language_set(shell);
		break;
	case PERSONAL_SHARING:
		personal_sharing_set(shell);
		break;
	case PERSONAL_MULTI_LANGUAGE:
		personal_multi_language_set(shell);
		break;
	default:
		shell_error(shell, "Use case not found");
		return -EINVAL;
	}

	cmd_show(shell, 0, NULL);

	return 0;
}

static int cmd_clear(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	nrf_auraconfig_clear();

	return 0;
}

static void file_paths_get(size_t idx, struct shell_static_entry *entry);

SHELL_DYNAMIC_CMD_CREATE(folder_names, file_paths_get);

static void file_paths_get(size_t idx, struct shell_static_entry *entry)
{
	if (idx < num_files_added) {
		entry->syntax = sd_paths_and_files[idx];
	} else {
		entry->syntax = NULL;
	}

	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = &folder_names;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_file_cmd,
			       SHELL_COND_CMD(CONFIG_SHELL, list, NULL, "List files on SD card",
					      cmd_file_list),
			       /* 5 required arguments */
			       SHELL_CMD_ARG(select, &folder_names, "Select file on SD card",
					     cmd_file_select, 5, 0),
			       SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_broadcast_id_cmd,
	SHELL_COND_CMD(CONFIG_SHELL, random, NULL,
		       "Set the broadcast ID to be random generated on each start", cmd_random_id),
	SHELL_COND_CMD(CONFIG_SHELL, fixed, NULL, "Set the broadcast ID to be a fixed value",
		       cmd_fixed_id),
	SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
	configuration_cmd, SHELL_COND_CMD(CONFIG_SHELL, list, NULL, "List presets", cmd_list),
	SHELL_COND_CMD(CONFIG_SHELL, start, NULL, "Start broadcaster", cmd_start),
	SHELL_COND_CMD(CONFIG_SHELL, stop, NULL, "Stop broadcaster", cmd_stop),
	SHELL_COND_CMD(CONFIG_SHELL, show, NULL, "Show current configuration", cmd_show),
	SHELL_COND_CMD(CONFIG_SHELL, packing, NULL, "Set type of packing", cmd_packing),
	SHELL_COND_CMD(CONFIG_SHELL, preset, NULL, "Set preset", cmd_preset),
	SHELL_COND_CMD(CONFIG_SHELL, lang, NULL, "Set language", cmd_lang),
	SHELL_COND_CMD(CONFIG_SHELL, immediate, NULL, "Set immediate rendering flag",
		       cmd_immediate_set),
	SHELL_COND_CMD(CONFIG_SHELL, num_subgroups, NULL, "Set number of subgroups",
		       cmd_num_subgroups),
	SHELL_COND_CMD(CONFIG_SHELL, num_bises, NULL, "Set number of BISes", cmd_num_bises),
	SHELL_COND_CMD(CONFIG_SHELL, context, NULL, "Set context", cmd_context),
	SHELL_COND_CMD(CONFIG_SHELL, location, NULL, "Set location", cmd_location),
	SHELL_COND_CMD(CONFIG_SHELL, broadcast_name, NULL, "Set broadcast name",
		       cmd_broadcast_name),
	SHELL_COND_CMD(CONFIG_SHELL, encrypt, NULL, "Set broadcast code", cmd_encrypt),
	SHELL_COND_CMD(CONFIG_SHELL, usecase, NULL, "Set use case", cmd_usecase),
	SHELL_COND_CMD(CONFIG_SHELL, clear, NULL, "Clear configuration", cmd_clear),
	SHELL_COND_CMD(CONFIG_SHELL, adv_name, NULL, "Set advertising name", cmd_adv_name),
	SHELL_COND_CMD(CONFIG_SHELL, program_info, NULL, "Set program info", cmd_program_info),
	SHELL_COND_CMD(CONFIG_SHELL, phy, NULL, "Set PHY", cmd_phy),
	SHELL_COND_CMD(CONFIG_SHELL, framing, NULL, "Set framing", cmd_framing),
	SHELL_COND_CMD(CONFIG_SHELL, rtn, NULL, "Set number of re-transmits", cmd_rtn),
	SHELL_COND_CMD(CONFIG_SHELL, sdu, NULL, "Set SDU (octets)", cmd_sdu),
	SHELL_COND_CMD(CONFIG_SHELL, mtl, NULL, "Set Max Transport latency (ms)", cmd_mtl),
	SHELL_COND_CMD(CONFIG_SHELL, frame_interval, NULL, "Set frame interval (us)",
		       cmd_frame_interval),
	SHELL_COND_CMD(CONFIG_SHELL, pd, NULL, "Set presentation delay (us)", cmd_pd),
	SHELL_COND_CMD(CONFIG_SHELL, file, &sub_file_cmd, "File commands", NULL),
	SHELL_COND_CMD(CONFIG_SHELL, broadcast_id, &sub_broadcast_id_cmd, "Broadcast ID commands",
		       NULL),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(nac, &configuration_cmd, "nRF Auraconfig", NULL);
