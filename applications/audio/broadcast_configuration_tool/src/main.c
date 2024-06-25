/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/kernel.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/shell/shell.h>
#include <nrfx_clock.h>

#include "broadcast_source.h"
#include "zbus_common.h"
#include "macros_common.h"
#include "bt_mgmt.h"
#include "sd_card.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, CONFIG_MAIN_LOG_LEVEL);

ZBUS_CHAN_DECLARE(bt_mgmt_chan);
ZBUS_CHAN_DECLARE(sdu_ref_chan);

struct bt_le_ext_adv *ext_adv;

/* Buffer for the UUIDs. */
#define EXT_ADV_UUID_BUF_SIZE (128)
NET_BUF_SIMPLE_DEFINE_STATIC(uuid_data, EXT_ADV_UUID_BUF_SIZE);
NET_BUF_SIMPLE_DEFINE_STATIC(uuid_data2, EXT_ADV_UUID_BUF_SIZE);

/* Buffer for periodic advertising BASE data. */
NET_BUF_SIMPLE_DEFINE_STATIC(base_data, 128);
NET_BUF_SIMPLE_DEFINE_STATIC(base_data2, 128);

/* Extended advertising buffer. */
static struct bt_data ext_adv_buf[CONFIG_BT_ISO_MAX_BIG][CONFIG_EXT_ADV_BUF_MAX];

/* Periodic advertising buffer. */
static struct bt_data per_adv_buf[CONFIG_BT_ISO_MAX_BIG];

#if (CONFIG_AURACAST)
/* Total size of the PBA buffer includes the 16-bit UUID, 8-bit features and the
 * meta data size.
 */
#define BROADCAST_SRC_PBA_BUF_SIZE                                                                 \
	(BROADCAST_SOURCE_PBA_HEADER_SIZE + CONFIG_BT_AUDIO_BROADCAST_PBA_METADATA_SIZE)

#define PRESET_NAME_MAX 8

/* Number of metadata items that can be assigned. */
#define BROADCAST_SOURCE_PBA_METADATA_VACANT                                                       \
	(CONFIG_BT_AUDIO_BROADCAST_PBA_METADATA_SIZE / (sizeof(struct bt_data)))

/* Make sure pba_buf is large enough for a 16bit UUID and meta data
 * (any addition to pba_buf requires an increase of this value)
 */
uint8_t pba_data[CONFIG_BT_ISO_MAX_BIG][BROADCAST_SRC_PBA_BUF_SIZE];

static struct bt_bap_lc3_preset lc3_preset_8_2_1 = BT_BAP_LC3_BROADCAST_PRESET_8_2_1(
	BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT, BT_AUDIO_CONTEXT_TYPE_MEDIA);

static struct bt_bap_lc3_preset lc3_preset_8_2_2 = BT_BAP_LC3_BROADCAST_PRESET_8_2_2(
	BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT, BT_AUDIO_CONTEXT_TYPE_MEDIA);

static struct bt_bap_lc3_preset lc3_preset_16_2_1 = BT_BAP_LC3_BROADCAST_PRESET_16_2_1(
	BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT, BT_AUDIO_CONTEXT_TYPE_MEDIA);

static struct bt_bap_lc3_preset lc3_preset_16_2_2 = BT_BAP_LC3_BROADCAST_PRESET_16_2_2(
	BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT, BT_AUDIO_CONTEXT_TYPE_MEDIA);

static struct bt_bap_lc3_preset lc3_preset_24_2_1 = BT_BAP_LC3_BROADCAST_PRESET_24_2_1(
	BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT, BT_AUDIO_CONTEXT_TYPE_MEDIA);

static struct bt_bap_lc3_preset lc3_preset_24_2_2 = BT_BAP_LC3_BROADCAST_PRESET_24_2_2(
	BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT, BT_AUDIO_CONTEXT_TYPE_MEDIA);

static struct bt_bap_lc3_preset lc3_preset_32_2_1 = BT_BAP_LC3_BROADCAST_PRESET_32_2_1(
	BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT, BT_AUDIO_CONTEXT_TYPE_MEDIA);

static struct bt_bap_lc3_preset lc3_preset_32_2_2 = BT_BAP_LC3_BROADCAST_PRESET_32_2_2(
	BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT, BT_AUDIO_CONTEXT_TYPE_MEDIA);

static struct bt_bap_lc3_preset lc3_preset_48_2_1 = BT_BAP_LC3_BROADCAST_PRESET_48_2_1(
	BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT, BT_AUDIO_CONTEXT_TYPE_MEDIA);

static struct bt_bap_lc3_preset lc3_preset_48_2_2 = BT_BAP_LC3_BROADCAST_PRESET_48_2_2(
	BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT, BT_AUDIO_CONTEXT_TYPE_MEDIA);

static struct bt_bap_lc3_preset lc3_preset_48_4_1 = BT_BAP_LC3_BROADCAST_PRESET_48_4_1(
	BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT, BT_AUDIO_CONTEXT_TYPE_MEDIA);

static struct bt_bap_lc3_preset lc3_preset_48_4_2 = BT_BAP_LC3_BROADCAST_PRESET_48_4_2(
	BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT, BT_AUDIO_CONTEXT_TYPE_MEDIA);

static struct bt_bap_lc3_preset lc3_preset_48_6_1 = BT_BAP_LC3_BROADCAST_PRESET_48_6_1(
	BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT, BT_AUDIO_CONTEXT_TYPE_MEDIA);

static struct bt_bap_lc3_preset lc3_preset_48_6_2 = BT_BAP_LC3_BROADCAST_PRESET_48_6_2(
	BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT, BT_AUDIO_CONTEXT_TYPE_MEDIA);

struct bap_preset {
	struct bt_bap_lc3_preset *preset;
	char name[PRESET_NAME_MAX];
};

static struct bap_preset bap_presets[] = {
	{.preset = &lc3_preset_8_2_1, .name = "8_2_1"},
	{.preset = &lc3_preset_8_2_2, .name = "8_2_2"},
	{.preset = &lc3_preset_16_2_1, .name = "16_2_1"},
	{.preset = &lc3_preset_16_2_2, .name = "16_2_2"},
	{.preset = &lc3_preset_24_2_1, .name = "24_2_1"},
	{.preset = &lc3_preset_24_2_2, .name = "24_2_2"},
	{.preset = &lc3_preset_32_2_1, .name = "32_2_1"},
	{.preset = &lc3_preset_32_2_2, .name = "32_2_2"},
	{.preset = &lc3_preset_48_2_1, .name = "48_2_1"},
	{.preset = &lc3_preset_48_2_2, .name = "48_2_2"},
	{.preset = &lc3_preset_48_4_1, .name = "48_4_1"},
	{.preset = &lc3_preset_48_4_2, .name = "48_4_2"},
	{.preset = &lc3_preset_48_6_1, .name = "48_6_1"},
	{.preset = &lc3_preset_48_6_2, .name = "48_6_2"},
};

/**
 * @brief	Broadcast source static extended advertising data.
 */
static struct broadcast_source_ext_adv_data ext_adv_data[] = {
	{.uuid_buf = &uuid_data,
	 .pba_metadata_vacant_cnt = BROADCAST_SOURCE_PBA_METADATA_VACANT,
	 .pba_buf = pba_data[0]},
	{.uuid_buf = &uuid_data2,
	 .pba_metadata_vacant_cnt = BROADCAST_SOURCE_PBA_METADATA_VACANT,
	 .pba_buf = pba_data[1]}};
#else
/**
 * @brief	Broadcast source static extended advertising data.
 */
static struct broadcast_source_ext_adv_data ext_adv_data[] = {{.uuid_buf = &uuid_data},
							      {.uuid_buf = &uuid_data2}};
#endif /* (CONFIG_AURACAST) */

/**
 * @brief	Broadcast source static periodic advertising data.
 */
static struct broadcast_source_per_adv_data per_adv_data[] = {{.base_buf = &base_data},
							      {.base_buf = &base_data2}};

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
		LOG_INF("Ext adv ready");

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

	ret = zbus_chan_add_obs(&bt_mgmt_chan, &bt_mgmt_evt_listen, ZBUS_ADD_OBS_TIMEOUT_MS);
	if (ret) {
		LOG_ERR("Failed to add bt_mgmt listener");
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

	ext_adv_buf[ext_adv_buf_cnt].type = BT_DATA_UUID16_SOME;
	ext_adv_buf[ext_adv_buf_cnt].data = ext_adv_data->uuid_buf->data;
	ext_adv_buf_cnt++;

	ret = bt_mgmt_manufacturer_uuid_populate(ext_adv_data->uuid_buf,
						 CONFIG_BT_DEVICE_MANUFACTURER_ID);
	if (ret) {
		LOG_ERR("Failed to add adv data with manufacturer ID: %d", ret);
		return ret;
	}

	ret = broadcast_source_ext_adv_populate(big_index, ext_adv_data,
						&ext_adv_buf[ext_adv_buf_cnt],
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

static void audio_send(void const *const data, size_t size, uint8_t num_ch)
{
	int ret;
	static int prev_ret;

	struct le_audio_encoded_audio enc_audio = {.data = data, .size = size, .num_ch = num_ch};

	ret = broadcast_source_send(0, 0, enc_audio);

	if (ret != 0 && ret != prev_ret) {
		if (ret == -ECANCELED) {
			LOG_WRN("Sending operation cancelled");
		} else {
			LOG_WRN("Problem with sending LE audio data, ret: %d", ret);
		}
	}

	prev_ret = ret;
}

static void broadcast_create(struct broadcast_source_big *broadcast_param,
			     struct bt_bap_lc3_preset *preset)
{
	static enum bt_audio_location location[2] = {BT_AUDIO_LOCATION_FRONT_LEFT,
						     BT_AUDIO_LOCATION_FRONT_RIGHT};
	static struct subgroup_config subgroups[1];

	subgroups[0].group_lc3_preset = *preset;
	subgroups[0].num_bises = 2;
	subgroups[0].context = BT_AUDIO_CONTEXT_TYPE_MEDIA;
	subgroups[0].location = location;

	broadcast_param->packing = BT_ISO_PACKING_INTERLEAVED;

	broadcast_param->encryption = false;

	bt_audio_codec_cfg_meta_set_bcast_audio_immediate_rend_flag(
		&subgroups[0].group_lc3_preset.codec_cfg);

	uint8_t english_src[3] = "eng";

	bt_audio_codec_cfg_meta_set_stream_lang(&subgroups[0].group_lc3_preset.codec_cfg,
						(uint32_t)sys_get_le24(english_src));

	broadcast_param->subgroups = subgroups;
	broadcast_param->num_subgroups = 1;
}

int main(void)
{
	int ret;
	LOG_DBG("Main started");

	ret = sd_card_init();
	if (ret != -ENODEV && ret != 0) {
		LOG_ERR("Failed to initialize SD card");
		return ret;
	}

	ret = nrfx_clock_divider_set(NRF_CLOCK_DOMAIN_HFCLK, NRF_CLOCK_HFCLK_DIV_1);
	ret -= NRFX_ERROR_BASE_NUM;
	if (ret) {
		return ret;
	}

	ret = bt_mgmt_init();
	ERR_CHK(ret);

	ret = zbus_link_producers_observers();
	ERR_CHK_MSG(ret, "Failed to link zbus producers and observers");

	return 0;
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

static int cmd_list(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	for (size_t i = 0; i < ARRAY_SIZE(bap_presets); i++) {
		shell_print(shell, "%s", bap_presets[i].name);
	}

	return 0;
}

static int cmd_start(const struct shell *shell, size_t argc, char **argv)
{
	int ret;
	size_t ext_adv_buf_cnt = 0;
	size_t per_adv_buf_cnt = 0;

	if (argc < 2) {
		shell_error(shell, "Usage: bct start <preset>");
		return -EINVAL;
	}

	struct bt_bap_lc3_preset *preset = preset_find(argv[1]);

	if (!preset) {
		shell_error(shell, "Preset not found");
		return -EINVAL;
	}

	struct broadcast_source_big broadcast_param;

	broadcast_create(&broadcast_param, preset);

	ret = broadcast_source_enable(&broadcast_param, 1);
	if (ret) {
		shell_error(shell, "Failed to enable broadcaster(s): %d", ret);
		return ret;
	}

	/* Get advertising set for BIG0 */
	ret = ext_adv_populate(0, &ext_adv_data[0], ext_adv_buf[0], ARRAY_SIZE(ext_adv_buf[0]),
			       &ext_adv_buf_cnt);
	if (ret) {
		return ret;
	}

	ret = per_adv_populate(0, &per_adv_data[0], &per_adv_buf[0], 1, &per_adv_buf_cnt);
	if (ret) {
		return ret;
	}

	/* Start broadcaster */
	ret = bt_mgmt_adv_start(0, ext_adv_buf[0], ext_adv_buf_cnt, &per_adv_buf[0],
				per_adv_buf_cnt, false);
	if (ret) {
		shell_error(shell, "Failed to start first advertiser");
		return ret;
	}

	shell_print(shell, "Broadcaster started");
	return 0;
}

static int cmd_stop(const struct shell *shell, size_t argc, char **argv)
{
	int ret;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	ret = broadcast_source_disable(0);
	if (ret) {
		shell_error(shell, "Failed to stop broadcaster(s) %d", ret);
		return ret;
	}

	ret = bt_mgmt_adv_stop(0);
	if (ret) {
		shell_error(shell, "Failed to stop advertiser");
		return ret;
	}

	net_buf_simple_reset(&base_data);
	net_buf_simple_reset(&base_data2);

	net_buf_simple_reset(&uuid_data);
	net_buf_simple_reset(&uuid_data2);

	per_adv_buf[0].data_len = 0;
	per_adv_buf[0].type = 0;

	for (size_t i = 0; i < ARRAY_SIZE(ext_adv_buf[0]); i++) {
		ext_adv_buf[0][i].data_len = 0;
		ext_adv_buf[0][i].type = 0;
	}

	shell_print(shell, "Broadcaster stopped");

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	configuration_cmd, SHELL_COND_CMD(CONFIG_SHELL, list, NULL, "List presets", cmd_list),
	SHELL_COND_CMD(CONFIG_SHELL, start, NULL, "Start broadcaster", cmd_start),
	SHELL_COND_CMD(CONFIG_SHELL, stop, NULL, "Stop broadcaster", cmd_stop),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(bct, &configuration_cmd, "Broadcast Configuration Tool", NULL);
