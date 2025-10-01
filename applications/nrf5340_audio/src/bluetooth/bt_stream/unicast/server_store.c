/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/sys/slist.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/addr.h>
#include <string.h>

#include "server_store.h"
#include "macros_common.h"
#include "le_audio.h"

#include <../subsys/bluetooth/audio/bap_endpoint.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(server_store, CONFIG_SERVER_STORE_LOG_LEVEL);

#ifndef CONFIG_BT_AUDIO_PREF_SOURCE_SAMPLE_RATE_VALUE
#define CONFIG_BT_AUDIO_PREF_SOURCE_SAMPLE_RATE_VALUE 0x08
#endif

#ifndef CONFIG_BT_AUDIO_PREF_SINK_SAMPLE_RATE_VALUE
#define CONFIG_BT_AUDIO_PREF_SINK_SAMPLE_RATE_VALUE 0x08
#endif

static struct bt_bap_lc3_preset lc3_preset_48_4_1 = BT_BAP_LC3_UNICAST_PRESET_48_4_1(
	BT_AUDIO_LOCATION_ANY, (BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED));
static struct bt_bap_lc3_preset lc3_preset_24_2_1 = BT_BAP_LC3_UNICAST_PRESET_24_2_1(
	BT_AUDIO_LOCATION_ANY, (BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED));
static struct bt_bap_lc3_preset lc3_preset_16_2_1 = BT_BAP_LC3_UNICAST_PRESET_16_2_1(
	BT_AUDIO_LOCATION_ANY, (BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED));

K_SEM_DEFINE(sem, 1, 1);
static atomic_ptr_t lock_owner;
#if CONFIG_DEBUG
static char owner_file[50];
static int owner_line;
#endif

static void valid_entry_check(char const *const str)
{
	LOG_DBG("Stored: %p current: %p", atomic_ptr_get(&lock_owner), k_current_get());
	__ASSERT(k_sem_count_get(&sem) == 0, "%s: Semaphore not taken", str);
	__ASSERT(atomic_ptr_get(&lock_owner) == k_current_get(), "%s: Thread mismatch", str);
}

/* Any address stored here will be there if the server is connected, or
 *if the device is disconnected but bonded
 */
static struct server_store servers[CONFIG_BT_MAX_CONN];

static int server_add(struct server_store *server)
{
	for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		char peer_str[BT_ADDR_LE_STR_LEN];

		bt_addr_le_to_str(&server->addr, peer_str, BT_ADDR_LE_STR_LEN);

		if (bt_addr_le_eq(&servers[i].addr, BT_ADDR_LE_ANY)) {
			memcpy(&servers[i], server, sizeof(struct server_store));
			LOG_INF("Added server %s to index %d", peer_str, i);
			return 0;
		}
	}

	return -ENOMEM;
}

static int server_remove(struct server_store *server)
{
	if (server == NULL) {
		LOG_ERR("Server is NULL");
		return -ENODEV;
	}

	memset(server, 0, sizeof(struct server_store));
	return 0;
}

struct pd {
	uint32_t min;
	uint32_t max;
	uint32_t pref_min;
	uint32_t pref_max;
};

static bool pres_dly_in_range(uint32_t existing_pres_dly_us,
			      struct bt_bap_qos_cfg_pref const *const qos_cfg_pref_in)
{
	if (qos_cfg_pref_in->pd_max == 0 || qos_cfg_pref_in->pd_min == 0) {
		LOG_ERR("No max or min presentation delay set");
		return false;
	}

	if (existing_pres_dly_us > qos_cfg_pref_in->pd_max) {
		LOG_DBG("Existing presentation delay %u is greater than max %u",
			existing_pres_dly_us, qos_cfg_pref_in->pd_max);
		return false;
	}

	if (existing_pres_dly_us < qos_cfg_pref_in->pd_min) {
		LOG_DBG("Existing presentation delay %u is less than min %u", existing_pres_dly_us,
			qos_cfg_pref_in->pd_min);
		return false;
	}
	/* We will not check the preferred presentation delay if we already have a running stream in
	 * the same group
	 */

	return true;
}

/* Looking for the smallest commonly acceptable set of presentation delays
 */
static int pres_delay_find(struct bt_bap_qos_cfg_pref *common,
			   struct bt_bap_qos_cfg_pref const *const in)
{
	/* Checking min values
	 */
	if (!IN_RANGE(in->pd_min, common->pd_min, common->pd_max)) {
		LOG_ERR("Incoming pd_min %d is not in range [%d, %d]", in->pd_min, common->pd_min,
			common->pd_max);
		return -EINVAL;
	}

	common->pd_min = MAX(in->pd_min, common->pd_min);

	if (common->pref_pd_min == 0) {
		common->pref_pd_min = in->pd_min;
	} else if (in->pref_pd_min == 0) {
		/* Incoming has no preferred min */
	} else {
		if (!IN_RANGE(in->pref_pd_min, common->pd_min, common->pd_max)) {
			LOG_WRN("Incoming pref_pd_min %d is not in range [%d, %d]. Will revert to "
				"common "
				"pd_min %d",
				in->pref_pd_min, common->pd_min, common->pd_max, common->pd_min);
			common->pref_pd_min = UINT32_MAX;
		}
	}

	/* Checking max values
	 */
	if (!IN_RANGE(in->pd_max, common->pd_min, common->pd_max)) {
		LOG_WRN("Incoming pd_max %d is not in range [%d, %d]. Ignored.", in->pd_max,
			common->pd_min, common->pd_max);
		common->pd_max = 0;
	} else {
		common->pd_max = MIN(in->pd_max, common->pd_max);
	}

	if (common->pref_pd_max == 0) {
		common->pref_pd_max = in->pd_max;
	} else if (in->pref_pd_max == 0) {
		/* Incoming has no preferred max
		 */
	} else {
		if (!IN_RANGE(in->pref_pd_max, common->pd_min, common->pd_max)) {
			LOG_WRN("Incoming pref_pd_max %d is not in range [%d, %d]. Will revert to "
				"common "
				"pd_max %d. Ignored.",
				in->pref_pd_max, common->pd_min, common->pd_max, common->pd_max);
			common->pref_pd_max = 0;
		} else {
			common->pref_pd_max = MIN(in->pref_pd_max, common->pref_pd_max);
		}
	}

	return 0;
}

static int sample_rate_check(uint16_t lc3_freq_bit, struct bt_bap_lc3_preset *preset,
			     uint8_t pref_sample_rate)
{
	/* Try with the preferred sample rate first
	 */
	switch (pref_sample_rate) {
	case BT_AUDIO_CODEC_CFG_FREQ_48KHZ:
		if (lc3_freq_bit & BT_AUDIO_CODEC_CAP_FREQ_48KHZ) {
			memcpy(preset, &lc3_preset_48_4_1, sizeof(struct bt_bap_lc3_preset));
			return 0;
		}

		break;

	case BT_AUDIO_CODEC_CFG_FREQ_24KHZ:
		if (lc3_freq_bit & BT_AUDIO_CODEC_CAP_FREQ_24KHZ) {
			memcpy(preset, &lc3_preset_24_2_1, sizeof(struct bt_bap_lc3_preset));
			return 0;
		}

		break;

	case BT_AUDIO_CODEC_CFG_FREQ_16KHZ:
		if (lc3_freq_bit & BT_AUDIO_CODEC_CAP_FREQ_16KHZ) {
			memcpy(preset, &lc3_preset_16_2_1, sizeof(struct bt_bap_lc3_preset));
			return 0;
		}

		break;

	default:
		break;
	}

	/* If no match with the preferred, revert to trying highest first
	 */
	if (lc3_freq_bit & BT_AUDIO_CODEC_CAP_FREQ_48KHZ) {
		memcpy(preset, &lc3_preset_48_4_1, sizeof(struct bt_bap_lc3_preset));
		return 0;
	} else if (lc3_freq_bit & BT_AUDIO_CODEC_CAP_FREQ_24KHZ) {
		memcpy(preset, &lc3_preset_24_2_1, sizeof(struct bt_bap_lc3_preset));
		return 0;
	} else if (lc3_freq_bit & BT_AUDIO_CODEC_CAP_FREQ_16KHZ) {
		memcpy(preset, &lc3_preset_16_2_1, sizeof(struct bt_bap_lc3_preset));
		return 0;
	}

	LOG_DBG("No supported sample rate found");
	return -ENOTSUP;
}

static bool sink_pac_parse(struct bt_data *data, void *user_data)
{
	int ret;
	struct bt_bap_lc3_preset *preset = (struct bt_bap_lc3_preset *)user_data;

	switch (data->type) {
	case BT_AUDIO_CODEC_CAP_TYPE_FREQ:
		uint16_t lc3_freq_bit = sys_get_le16(data->data);

		ret = sample_rate_check(lc3_freq_bit, preset,
					CONFIG_BT_AUDIO_PREF_SINK_SAMPLE_RATE_VALUE);
		if (ret) {
			/* This PAC record from the server is not supported by the client.
			 * Stop parsing this record.
			 */
			return false;
		}
		break;

	case BT_AUDIO_CODEC_CAP_TYPE_FRAME_LEN:
		/* Make sure the presets octets per frame is within the range of the codec
		 * capabilities.
		 */
		uint16_t lc3_min_frame_length = sys_get_le16(data->data);
		uint16_t lc3_max_frame_length = sys_get_le16(data->data + sizeof(uint16_t));

		int preset_octets_per_frame = 0;

		ret = le_audio_octets_per_frame_get(&preset->codec_cfg, &preset_octets_per_frame);
		if (ret) {
			LOG_WRN("Failed to get preset octets per frame: %d", ret);
			memset(preset, 0, sizeof(struct bt_bap_lc3_preset));
			return false;
		}

		if (!IN_RANGE(preset_octets_per_frame, lc3_min_frame_length,
			      lc3_max_frame_length)) {
			LOG_DBG("Sink preset octets/frame %d not in range [%d, %d]",
				preset_octets_per_frame, lc3_min_frame_length,
				lc3_max_frame_length);
			ret = bt_audio_codec_cfg_set_octets_per_frame(&preset->codec_cfg,
								      lc3_max_frame_length);
			if (ret < 0) {
				LOG_ERR("Failed to set preset octets per frame: %d", ret);
				memset(preset, 0, sizeof(struct bt_bap_lc3_preset));
				return false;
			}
		}
		break;

	default:
		break;
	}

	/* Did not find what we were looking for, continue parsing LTV
	 */
	return true;
}

static bool source_pac_parse(struct bt_data *data, void *user_data)
{
	if (data->type == BT_AUDIO_CODEC_CAP_TYPE_FREQ) {
		uint16_t lc3_freq_bit = sys_get_le16(data->data);

		sample_rate_check(lc3_freq_bit, (struct bt_bap_lc3_preset *)user_data,
				  CONFIG_BT_AUDIO_PREF_SOURCE_SAMPLE_RATE_VALUE);

		/* Found what we were looking for, stop parsing LTV
		 */
		return false;
	}

	/* Make sure the presets octets per frame is within the range of the codec capabilities
	 */
	if (data->type == BT_AUDIO_CODEC_CAP_TYPE_FRAME_LEN) {
		uint16_t lc3_min_frame_length = sys_get_le16(data->data);
		uint16_t lc3_max_frame_length = sys_get_le16(data->data + sizeof(uint16_t));

		int preset_octets_per_frame;
		struct bt_bap_lc3_preset *preset = (struct bt_bap_lc3_preset *)user_data;

		le_audio_octets_per_frame_get(&preset->codec_cfg, &preset_octets_per_frame);

		if (!IN_RANGE(preset_octets_per_frame, lc3_min_frame_length,
			      lc3_max_frame_length)) {
			LOG_DBG("Source preset octets/frame %d not in range [%d, %d]",
				preset_octets_per_frame, lc3_min_frame_length,
				lc3_max_frame_length);
			bt_audio_codec_cfg_set_octets_per_frame(&preset->codec_cfg,
								lc3_max_frame_length);
		}
	}

	/* Did not find what we were looking for, continue parsing LTV
	 */
	return true;
}

static void set_color_if_supported(char *str, uint16_t bitfield, uint16_t mask)
{
	if (bitfield & mask) {
		strcat(str, COLOR_GREEN);
	} else {
		strcat(str, COLOR_RED);
		strcat(str, "!");
	}
}

static bool pac_record_print(struct bt_data *data, void *user_data)
{
	if (data->type == BT_AUDIO_CODEC_CAP_TYPE_FREQ) {
		uint16_t freq_bit = sys_get_le16(data->data);
		char supported_freq[320] = "";

		set_color_if_supported(supported_freq, freq_bit, BT_AUDIO_CODEC_CAP_FREQ_8KHZ);
		strcat(supported_freq, "8, ");
		set_color_if_supported(supported_freq, freq_bit, BT_AUDIO_CODEC_CAP_FREQ_11KHZ);
		strcat(supported_freq, "11.025, ");
		set_color_if_supported(supported_freq, freq_bit, BT_AUDIO_CODEC_CAP_FREQ_16KHZ);
		strcat(supported_freq, "16, ");
		set_color_if_supported(supported_freq, freq_bit, BT_AUDIO_CODEC_CAP_FREQ_22KHZ);
		strcat(supported_freq, "22.05, ");
		set_color_if_supported(supported_freq, freq_bit, BT_AUDIO_CODEC_CAP_FREQ_24KHZ);
		strcat(supported_freq, "24, ");
		set_color_if_supported(supported_freq, freq_bit, BT_AUDIO_CODEC_CAP_FREQ_32KHZ);
		strcat(supported_freq, "32, ");
		set_color_if_supported(supported_freq, freq_bit, BT_AUDIO_CODEC_CAP_FREQ_44KHZ);
		strcat(supported_freq, "44.1, ");
		set_color_if_supported(supported_freq, freq_bit, BT_AUDIO_CODEC_CAP_FREQ_48KHZ);
		strcat(supported_freq, "48, ");
		set_color_if_supported(supported_freq, freq_bit, BT_AUDIO_CODEC_CAP_FREQ_88KHZ);
		strcat(supported_freq, "88.2, ");
		set_color_if_supported(supported_freq, freq_bit, BT_AUDIO_CODEC_CAP_FREQ_96KHZ);
		strcat(supported_freq, "96, ");
		set_color_if_supported(supported_freq, freq_bit, BT_AUDIO_CODEC_CAP_FREQ_176KHZ);
		strcat(supported_freq, "176, ");
		set_color_if_supported(supported_freq, freq_bit, BT_AUDIO_CODEC_CAP_FREQ_192KHZ);
		strcat(supported_freq, "192, ");
		set_color_if_supported(supported_freq, freq_bit, BT_AUDIO_CODEC_CAP_FREQ_384KHZ);
		strcat(supported_freq, "384");

		LOG_INF("\tFreq kHz: %s", supported_freq);
	}

	if (data->type == BT_AUDIO_CODEC_CAP_TYPE_DURATION) {
		uint16_t dur_bit = sys_get_le16(data->data);
		char supported_dur[80] = "";

		set_color_if_supported(supported_dur, dur_bit, BT_AUDIO_CODEC_CAP_DURATION_7_5);
		strcat(supported_dur, "7.5, ");
		set_color_if_supported(supported_dur, dur_bit, BT_AUDIO_CODEC_CAP_DURATION_10);
		strcat(supported_dur, "10");

		LOG_INF("\tFrame duration ms: %s", supported_dur);
	}

	if (data->type == BT_AUDIO_CODEC_CAP_TYPE_CHAN_COUNT) {
		uint16_t chan_bit = sys_get_le16(data->data);
		char supported_chan[140] = "";

		set_color_if_supported(supported_chan, chan_bit, BT_AUDIO_CODEC_CAP_CHAN_COUNT_1);
		strcat(supported_chan, "1, ");
		set_color_if_supported(supported_chan, chan_bit, BT_AUDIO_CODEC_CAP_CHAN_COUNT_2);
		strcat(supported_chan, "2, ");
		set_color_if_supported(supported_chan, chan_bit, BT_AUDIO_CODEC_CAP_CHAN_COUNT_3);
		strcat(supported_chan, "3, ");
		set_color_if_supported(supported_chan, chan_bit, BT_AUDIO_CODEC_CAP_CHAN_COUNT_4);
		strcat(supported_chan, "4, ");
		set_color_if_supported(supported_chan, chan_bit, BT_AUDIO_CODEC_CAP_CHAN_COUNT_5);
		strcat(supported_chan, "5, ");
		set_color_if_supported(supported_chan, chan_bit, BT_AUDIO_CODEC_CAP_CHAN_COUNT_6);
		strcat(supported_chan, "6, ");
		set_color_if_supported(supported_chan, chan_bit, BT_AUDIO_CODEC_CAP_CHAN_COUNT_7);
		strcat(supported_chan, "7, ");
		set_color_if_supported(supported_chan, chan_bit, BT_AUDIO_CODEC_CAP_CHAN_COUNT_8);
		strcat(supported_chan, "8");

		LOG_INF("\tChannels supported: %s", supported_chan);
	}

	if (data->type == BT_AUDIO_CODEC_CAP_TYPE_FRAME_LEN) {
		uint16_t lc3_min_frame_length = sys_get_le16(data->data);
		uint16_t lc3_max_frame_length = sys_get_le16(data->data + sizeof(uint16_t));

		LOG_INF("\tFrame length bytes: %d - %d", lc3_min_frame_length,
			lc3_max_frame_length);
	}

	if (data->type == BT_AUDIO_CODEC_CAP_TYPE_FRAME_COUNT) {
		uint16_t lc3_frame_per_sdu = sys_get_le16(data->data);

		LOG_INF("\tMax frames per SDU: %d", lc3_frame_per_sdu);
	}

	return true;
}

/* Check to see if the existing stream can be ignored when we try to find a valid presentation delay
 * when a new stream is added.
 */
static bool pres_dly_ignore_stream(struct bt_bap_stream const *const existing_stream,
				   struct bt_bap_stream const *const stream_in)
{
	if (existing_stream == NULL || stream_in == NULL) {
		LOG_ERR("NULL parameter");
		return true;
	}

	if (existing_stream->group == NULL) {
		return true;
	}

	if (existing_stream->group != stream_in->group) {
		/* The existing stream is not in the same group as the incoming stream */
		return true;
	}

	if (existing_stream == stream_in) {
		/* The existing stream is not in the same group as the incoming stream */
		return true;
	}

	if (existing_stream->ep == NULL) {
		return true;
	}

	return false;
}

static int srv_store_from_conn_get_internal(struct bt_conn const *const conn,
					    struct server_store **server)
{
	for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		const bt_addr_le_t *peer_addr = bt_conn_get_dst(conn);
		char peer_str[BT_ADDR_LE_STR_LEN];
		char local_str[BT_ADDR_LE_STR_LEN];

		bt_addr_le_to_str(&servers[i].addr, local_str, BT_ADDR_LE_STR_LEN);
		bt_addr_le_to_str(peer_addr, peer_str, BT_ADDR_LE_STR_LEN);

		if (bt_addr_le_eq(&servers[i].addr, bt_conn_get_dst(conn))) {
			*server = &servers[i];
			return 0;
		}
	}

	*server = NULL;
	return -ENOENT;
}

static int srv_store_from_addr_get_internal(bt_addr_le_t const *const addr,
					    struct server_store **server)
{
	for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		if (bt_addr_le_eq(&servers[i].addr, addr)) {
			*server = &servers[i];
			return 0;
		}
	}

	*server = NULL;
	return -ENOENT;
}

bool srv_store_preset_validated(struct bt_audio_codec_cfg *new, struct bt_audio_codec_cfg *existing,
				uint8_t pref_sample_rate_value)
{
	int ret;

	if (new == NULL || existing == NULL) {
		return false;
	}

	/* Higher sampling frequency is more preferred */
	int new_freq_hz = 0;
	int existing_freq_hz = 0;
	int pref_freq_hz = 0;

	ret = le_audio_freq_hz_get(new, &new_freq_hz);
	if (ret) {
		LOG_ERR("Failed to get new freq hz: %d", ret);
		return false;
	}

	const struct bt_audio_codec_cfg zero_dummy = {0};

	if (memcmp(existing, &zero_dummy, sizeof(struct bt_audio_codec_cfg)) == 0) {
		/* No existing preset, will use new one */
		return true;
	}

	ret = le_audio_freq_hz_get(existing, &existing_freq_hz);
	if (ret) {
		LOG_ERR("Failed to get existing freq hz: %d", ret);
		return false;
	}

	switch (pref_sample_rate_value) {
	case BT_AUDIO_CODEC_CFG_FREQ_48KHZ:
		pref_freq_hz = 48000;
		break;
	case BT_AUDIO_CODEC_CFG_FREQ_24KHZ:
		pref_freq_hz = 24000;
		break;
	case BT_AUDIO_CODEC_CFG_FREQ_16KHZ:
		pref_freq_hz = 16000;
		break;
	default:
		pref_freq_hz = -1;
		break;
	}

	if ((new_freq_hz >= existing_freq_hz && existing_freq_hz != pref_freq_hz) ||
	    new_freq_hz == pref_freq_hz) {
		LOG_DBG("New preset has higher frequency, or pref freq met: %d > %d", new_freq_hz,
			existing_freq_hz);

		if (new_freq_hz == pref_freq_hz && existing_freq_hz != pref_freq_hz) {
			LOG_DBG("New preset has preferred frequency: %d == %d", new_freq_hz,
				pref_freq_hz);
			return true;
		}

		int new_octets = 0;
		int existing_octets = 0;

		ret = le_audio_octets_per_frame_get(new, &new_octets);
		if (ret) {
			LOG_ERR("Failed to get new octets/frame: %d", ret);
			return false;
		}

		ret = le_audio_octets_per_frame_get(existing, &existing_octets);
		if (ret) {
			LOG_ERR("Failed to get existing octets/frame: %d", ret);
			return false;
		}

		if (new_octets >= existing_octets) {
			LOG_DBG("New preset has higher or equal octets/frame: %d >= %d", new_octets,
				existing_octets);
			return true;
		}
	}

	return false;
}

/* One needs to look at the group pointer, if this group pointer already exists, we may need
 * to update the entire group. If it is a new group, no reconfig is needed.
 *  New A, no ext group
 *  New A, existing A. If new pres delay needed, reconfig all
 *  New B, existing A, no reconfig needed.
 */

/* The presentation delay within a CIG for a given dir needs to be the same.
 * bt_bap_ep -> cig_id;
 */
int srv_store_pres_dly_find(struct bt_bap_stream *stream, uint32_t *computed_pres_dly_us,
			    uint32_t *existing_pres_dly_us,
			    struct bt_bap_qos_cfg_pref const *qos_cfg_pref_in,
			    bool *group_reconfig_needed)
{
	valid_entry_check(__func__);

	*existing_pres_dly_us = 0;

	int ret;

	if (stream == NULL || computed_pres_dly_us == NULL || qos_cfg_pref_in == NULL ||
	    group_reconfig_needed == NULL) {
		LOG_ERR("NULL parameter");
		return -EINVAL;
	}

	if (stream->group == NULL) {
		LOG_ERR("The incoming stream %p has no group", (void *)stream);
		return -EINVAL;
	}

	if (qos_cfg_pref_in->pd_min == 0 || qos_cfg_pref_in->pd_max == 0) {
		LOG_ERR("Incoming pd_min or pd_max is zero");
		return -EINVAL;
	}

	*group_reconfig_needed = false;
	*computed_pres_dly_us = UINT32_MAX;
	struct bt_bap_qos_cfg_pref common_pd = *qos_cfg_pref_in;
	struct bt_bap_ep_info ep_info;

	bt_bap_ep_get_info(stream->ep, &ep_info);
	enum bt_audio_dir dir = ep_info.dir;
	struct server_store *server = NULL;

	for (int srv_idx = 0; srv_idx < CONFIG_BT_MAX_CONN; srv_idx++) {

		/* Across all servers, we need to first check if another stream is in the
		 * same subgroup as the new incoming stream.
		 */
		server = &servers[srv_idx];
		if (server == NULL) {
			continue;
		}

		if (server->conn == stream->conn) {
			/* Do not compare against the same unicast server
			 */
			continue;
		}

		switch (dir) {
		case BT_AUDIO_DIR_SINK:
			for (int i = 0; i < CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT; i++) {

				if (pres_dly_ignore_stream(&server->snk.cap_streams[i].bap_stream,
							   stream)) {
					continue;
				}

				*existing_pres_dly_us =
					server->snk.cap_streams[i].bap_stream.qos->pd;

				if (*existing_pres_dly_us == 0) {
					LOG_ERR("Existing presentation delay is zero");
					return -EINVAL;
				}

				if (pres_dly_in_range(*existing_pres_dly_us, qos_cfg_pref_in)) {
					*computed_pres_dly_us = *existing_pres_dly_us;
					return 0;
				}

				*group_reconfig_needed = true;

				ret = pres_delay_find(
					&server->snk.cap_streams[i].bap_stream.ep->qos_pref,
					&common_pd);
				if (ret) {
					return ret;
				}
			}
			break;

		case BT_AUDIO_DIR_SOURCE:
			for (int i = 0; i < CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT; i++) {
				if (pres_dly_ignore_stream(&server->snk.cap_streams[i].bap_stream,
							   stream)) {
					continue;
				}

				*existing_pres_dly_us =
					server->src.cap_streams[i].bap_stream.qos->pd;

				if (*existing_pres_dly_us == 0) {
					LOG_ERR("Existing presentation delay is zero");
					return -EINVAL;
				}

				if (pres_dly_in_range(*existing_pres_dly_us, qos_cfg_pref_in)) {
					*computed_pres_dly_us = *existing_pres_dly_us;
					return 0;
				}

				*group_reconfig_needed = true;

				ret = pres_delay_find(
					&server->src.cap_streams[i].bap_stream.ep->qos_pref,
					&common_pd);
				if (ret) {
					return ret;
				}
			}
			break;
		default:
			LOG_ERR("Unknown direction: %d", dir);
			return -EINVAL;
		};
	}

	if (group_reconfig_needed) {
		/* Found other streams, need to use common denominator */
		if (common_pd.pref_pd_min == UINT32_MAX || common_pd.pref_pd_min == 0) {
			/* No preferred min, use common min */
			*computed_pres_dly_us = common_pd.pd_min;
		} else {
			/* Use preferred min */
			*computed_pres_dly_us = common_pd.pref_pd_min;
		}
		return 0;
	}

	/* No other streams in the same group found */

	if (qos_cfg_pref_in->pref_pd_min == 0) {
		*computed_pres_dly_us = qos_cfg_pref_in->pd_min;
	} else {
		*computed_pres_dly_us = qos_cfg_pref_in->pref_pd_min;
	}
	return 0;
}

int srv_store_location_set(struct bt_conn const *const conn, enum bt_audio_dir dir,
			   enum bt_audio_location loc)
{
	valid_entry_check(__func__);

	int ret;

	if (conn == NULL) {
		LOG_ERR("No valid connection pointer received");
		return -EINVAL;
	}

	if (dir != BT_AUDIO_DIR_SINK && dir != BT_AUDIO_DIR_SOURCE) {
		LOG_ERR("Invalid direction: %d", dir);
		return -EINVAL;
	}

	struct server_store *server = NULL;

	ret = srv_store_from_conn_get_internal(conn, &server);
	if (ret < 0) {
		return ret;
	}

	switch (dir) {
	case BT_AUDIO_DIR_SINK:
		server->snk.locations = loc;
		break;

	case BT_AUDIO_DIR_SOURCE:
		server->src.locations = loc;
		break;

	default:
		LOG_ERR("Unknown direction: %d", dir);
		return -EINVAL;
	}

	return 0;
}

int srv_store_valid_codec_cap_check(struct bt_conn const *const conn, enum bt_audio_dir dir,
				    uint32_t *valid_codec_caps,
				    struct client_supp_configs const **const client_supp_cfgs,
				    uint8_t num_client_supp_cfgs)
{
	valid_entry_check(__func__);
	/* Ref: OCT-3480. These parameters can be checked to determine a full match of
	 * client versus server(s) capabilities.
	 */
	ARG_UNUSED(client_supp_cfgs);
	ARG_UNUSED(num_client_supp_cfgs);

	int ret;
	struct bt_bap_lc3_preset zero_preset = {0};

	*valid_codec_caps = 0;
	if (conn == NULL || valid_codec_caps == NULL) {
		LOG_ERR("Invalid parameters: conn or valid_codec_caps is NULL");
		return -EINVAL;
	}

	struct server_store *server = NULL;

	ret = srv_store_from_conn_get_internal(conn, &server);
	if (ret < 0) {
		LOG_ERR("Unknown connection, should not reach here");
		return ret;
	}
	/* Only the sampling frequency is checked */
	if (dir == BT_AUDIO_DIR_SINK) {
		LOG_DBG("Discovered %d sink endpoint(s) for device", server->snk.num_eps);

		for (int i = 0; i < server->snk.num_codec_caps; i++) {
			struct bt_bap_lc3_preset preset = {0};

			if (IS_ENABLED(CONFIG_BT_AUDIO_PAC_REC_PRINT)) {
				LOG_INF("Sink PAC %d:", i);
				ret = bt_audio_data_parse(server->snk.codec_caps[i].data,
							  server->snk.codec_caps[i].data_len,
							  pac_record_print, NULL);
				if (ret) {
					LOG_ERR("Failed data parse %d:", ret);
				}
				LOG_INF("__________________________");
			}

			ret = bt_audio_data_parse(server->snk.codec_caps[i].data,
						  server->snk.codec_caps[i].data_len,
						  sink_pac_parse, &preset);
			if (ret && ret != -ECANCELED) {
				LOG_ERR("PAC record sink parse failed %d:", ret);
				continue;
			}

			if (memcmp(&preset, &zero_preset, sizeof(struct bt_bap_lc3_preset)) == 0) {
				continue;
			}

			LOG_DBG("Valid codec capabilities found for device, sink EP %d", i);
			*valid_codec_caps |= 1 << i;
			if (srv_store_preset_validated(
				    &(preset.codec_cfg), &(server->snk.lc3_preset[0].codec_cfg),
				    CONFIG_BT_AUDIO_PREF_SINK_SAMPLE_RATE_VALUE)) {
				memcpy(&server->snk.lc3_preset[0], &preset,
				       sizeof(struct bt_bap_lc3_preset));
			}
		}
	} else if (dir == BT_AUDIO_DIR_SOURCE) {
		LOG_DBG("Discovered %d source endpoint(s) for device", server->src.num_eps);

		for (int i = 0; i < server->src.num_codec_caps; i++) {
			struct bt_bap_lc3_preset preset = {0};

			if (IS_ENABLED(CONFIG_BT_AUDIO_PAC_REC_PRINT)) {
				LOG_INF("Source PAC %d:", i);
				ret = bt_audio_data_parse(server->src.codec_caps[i].data,
							  server->src.codec_caps[i].data_len,
							  pac_record_print, NULL);
				if (ret) {
					LOG_ERR("Failed data parse %d:", ret);
				}
				LOG_INF("__________________________");
			}

			ret = bt_audio_data_parse(server->src.codec_caps[i].data,
						  server->src.codec_caps[i].data_len,
						  source_pac_parse, &preset);

			if (ret && ret != -ECANCELED) {
				LOG_ERR("PAC record source parse failed %d:", ret);
				continue;
			}

			if (memcmp(&preset, &zero_preset, sizeof(struct bt_bap_lc3_preset)) == 0) {
				continue;
			}

			LOG_DBG("Valid codec capabilities found for device, source EP %d", i);
			*valid_codec_caps |= 1 << i;

			if (srv_store_preset_validated(
				    &(preset.codec_cfg), &(server->src.lc3_preset[0].codec_cfg),
				    CONFIG_BT_AUDIO_PREF_SOURCE_SAMPLE_RATE_VALUE)) {
				memcpy(&server->src.lc3_preset[0], &preset,
				       sizeof(struct bt_bap_lc3_preset));
			}
		}
	} else {
		LOG_ERR("Unknown direction");
	}

	return 0;
}

int srv_store_from_stream_get(struct bt_bap_stream const *const stream,
			      struct server_store **server)
{
	valid_entry_check(__func__);

	struct server_store *tmp_server = NULL;
	uint32_t matches = 0;

	*server = NULL;

	if (stream == NULL || server == NULL) {
		LOG_ERR("Invalid parameters: stream or server is NULL");
		return -EINVAL;
	}

	for (int srv_idx = 0; srv_idx < CONFIG_BT_MAX_CONN; srv_idx++) {
		tmp_server = &servers[srv_idx];
		if (bt_addr_le_eq(&tmp_server->addr, BT_ADDR_LE_ANY)) {
			continue;
		}
		for (int i = 0; i < CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT; i++) {

			LOG_DBG("checking server %d, sink stream %d %p %p", srv_idx, i,
				&tmp_server->snk.cap_streams[i].bap_stream, stream);

			if (&tmp_server->snk.cap_streams[i].bap_stream == stream) {
				*server = tmp_server;
				LOG_DBG("Found server for sink stream %p "
					"at index %d",
					stream, srv_idx);
				matches++;
			}
		}
		for (int i = 0; i < CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT; i++) {
			LOG_DBG("checking server %d, source stream %d", srv_idx, i);
			if (&tmp_server->src.cap_streams[i].bap_stream == stream) {
				*server = tmp_server;
				LOG_DBG("Found server for source stream %p "
					"at index %d",
					stream, srv_idx);
				matches++;
			}
		}
	}

	if (matches == 0) {
		LOG_ERR("No server found for the given stream");
		*server = NULL;
		return -ENOENT;
	} else if (matches > 1) {
		LOG_ERR("Multiple servers found for the same stream, this "
			"should "
			"not happen");
		*server = NULL;
		return -ESPIPE;
	}

	return 0;
}

static int srv_store_ep_state_count(struct bt_conn const *const conn, enum bt_bap_ep_state state,
				    enum bt_audio_dir dir)
{
	valid_entry_check(__func__);

	int ret;
	int count = 0;
	struct server_store *server = NULL;

	ret = srv_store_from_conn_get_internal(conn, &server);
	if (ret < 0) {
		return ret;
	}

	switch (dir) {
	case BT_AUDIO_DIR_SINK:
		for (int i = 0; i < CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT; i++) {
			if (server->snk.cap_streams[i].bap_stream.ep == NULL) {
				break;
			}

			struct bt_bap_ep_info ep_info;

			ret = bt_bap_ep_get_info(server->snk.cap_streams[i].bap_stream.ep,
						 &ep_info);
			if (ret) {
				return ret;
			}

			if (ep_info.state == state) {
				count++;
			}
		}
		break;

	case BT_AUDIO_DIR_SOURCE:
		for (int i = 0; i < CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT; i++) {
			if (server->src.cap_streams[i].bap_stream.ep != NULL) {
				struct bt_bap_ep_info ep_info;

				ret = bt_bap_ep_get_info(server->src.cap_streams[i].bap_stream.ep,
							 &ep_info);
				if (ret) {
					return ret;
				}

				if (ep_info.state == state) {
					count++;
				}
			} else {
				break;
			}
		}
		break;

	default:
		LOG_ERR("Unknown direction: %d", dir);
		return -EINVAL;
	}

	return count;
}

int srv_store_all_ep_state_count(enum bt_bap_ep_state state, enum bt_audio_dir dir)
{
	valid_entry_check(__func__);

	int count = 0;
	int count_total = 0;
	struct server_store *server = NULL;

	for (int srv_idx = 0; srv_idx < CONFIG_BT_MAX_CONN; srv_idx++) {
		server = &servers[srv_idx];
		if (bt_addr_le_eq(&server->addr, BT_ADDR_LE_ANY)) {
			/* Empty entry */
			continue;
		}

		if (server->conn == NULL) {
			continue;
		}

		count = srv_store_ep_state_count(server->conn, state, dir);
		if (count < 0) {
			LOG_ERR("Failed to get ep state count for server "
				"%d: %d",
				srv_idx, count);
			return count;
		}
		count_total += count;
	}

	return count_total;
}

int srv_store_avail_context_set(struct bt_conn *conn, enum bt_audio_context snk_ctx,
				enum bt_audio_context src_ctx)
{
	valid_entry_check(__func__);
	int ret;

	if (conn == NULL) {
		LOG_ERR("No valid connection pointer received");
		return -EINVAL;
	}

	struct server_store *server = NULL;

	ret = srv_store_from_conn_get(conn, &server);
	if (ret < 0) {
		return ret;
	}

	server->snk.available_ctx = snk_ctx;
	server->src.available_ctx = src_ctx;

	return 0;
}

int srv_store_codec_cap_set(struct bt_conn const *const conn, enum bt_audio_dir dir,
			    struct bt_audio_codec_cap const *const codec)
{
	valid_entry_check(__func__);
	int ret;

	if (conn == NULL) {
		LOG_ERR("No valid connection pointer received");
		return -EINVAL;
	}

	if (codec == NULL) {
		LOG_ERR("Codec capability is NULL");
		return -EINVAL;
	}

	if (codec->data_len > CONFIG_BT_AUDIO_CODEC_CAP_MAX_DATA_SIZE) {
		LOG_ERR("Codec data length exceeds maximum size: %d", codec->data_len);
		return -ENOMEM;
	}

	struct server_store *server = NULL;

	ret = srv_store_from_conn_get_internal(conn, &server);
	if (ret < 0) {
		return ret;
	}

	if (codec->data_len > CONFIG_BT_AUDIO_CODEC_CAP_MAX_DATA_SIZE) {
		LOG_ERR("Codec data length exceeds maximum size: %d", codec->data_len);
		return -EINVAL;
	}

	if (dir == BT_AUDIO_DIR_SINK) {
		/* num_codec_caps is an increasing index that starts at 0 */
		if (server->snk.num_codec_caps >= ARRAY_SIZE(server->snk.codec_caps)) {
			LOG_WRN("No more space (%d) for sink codec "
				"capabilities, increase "
				"CONFIG_CODEC_CAP_COUNT_MAX(%d)",
				server->snk.num_codec_caps, ARRAY_SIZE(server->snk.codec_caps));
			return -ENOMEM;
		}

		memcpy(&server->snk.codec_caps[server->snk.num_codec_caps], codec,
		       sizeof(struct bt_audio_codec_cap));
		server->snk.num_codec_caps++;
	} else if (dir == BT_AUDIO_DIR_SOURCE) {
		/* num_codec_caps is an increasing index that starts at 0 */
		if (server->src.num_codec_caps >= ARRAY_SIZE(server->src.codec_caps)) {
			LOG_WRN("No more space for source codec "
				"capabilities, increase "
				"CONFIG_CODEC_CAP_COUNT_MAX");
			return -ENOMEM;
		}

		memcpy(&server->src.codec_caps[server->src.num_codec_caps], codec,
		       sizeof(struct bt_audio_codec_cap));
		server->src.num_codec_caps++;
	} else {
		LOG_ERR("PAC record direction not recognized: %d", dir);
		return -EINVAL;
	}

	return 0;
}

int srv_store_from_addr_get(bt_addr_le_t const *const addr, struct server_store **server)
{
	valid_entry_check(__func__);
	int ret;

	ret = srv_store_from_addr_get_internal(addr, server);
	return ret;
}

bool srv_store_server_exists(bt_addr_le_t const *const addr)
{
	valid_entry_check(__func__);
	int ret;

	if (addr == NULL) {
		LOG_ERR("No valid address pointer received");
		return false;
	}

	struct server_store *temp_server = NULL;

	ret = srv_store_from_addr_get_internal(addr, &temp_server);
	if (ret == 0) {
		return true;
	} else if (ret == -ENOENT) {
		return false;
	}

	LOG_ERR("Error checking if server exists: %d", ret);
	return false;
}

int srv_store_from_conn_get(struct bt_conn const *const conn, struct server_store **server)
{
	valid_entry_check(__func__);
	int ret;

	ret = srv_store_from_conn_get_internal(conn, server);
	return ret;
}

int srv_store_num_get(bool check_consecutive)
{
	valid_entry_check(__func__);
	int num_servers = 0;
	bool prev_found = true;

	for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		if (!bt_addr_le_eq(&servers[i].addr, BT_ADDR_LE_ANY)) {
			num_servers++;
			if (!prev_found && check_consecutive) {
				LOG_ERR("Non-consecutive server storage detected");
				return -EINVAL;
			}
		} else {
			prev_found = false;
		}
	}

	return num_servers;
}

int srv_store_server_get(struct server_store **server, uint8_t index)
{
	valid_entry_check(__func__);

	if (index >= CONFIG_BT_MAX_CONN) {
		return -EINVAL;
	}

	if (servers[index].conn == NULL || bt_addr_le_eq(&servers[index].addr, BT_ADDR_LE_ANY)) {
		*server = NULL;
		return -ENOENT;
	}

	*server = &servers[index];

	return 0;
}

int srv_store_add_by_conn(struct bt_conn *conn)
{
	valid_entry_check(__func__);

	int ret;
	struct server_store *temp_server = NULL;

	LOG_INF("Adding server by conn");

	const bt_addr_le_t *peer_addr = bt_conn_get_dst(conn);
	char peer_str[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(peer_addr, peer_str, BT_ADDR_LE_STR_LEN);

	LOG_INF("Adding server for peer: %s", peer_str);
	/*TODO: Check. If the address is random, we delete when the connection is removed */

	/* Check if server already exists
	 */
	ret = srv_store_from_conn_get(conn, &temp_server);
	if (ret == 0) {
		/* Server already exists, no need to add again, but we update the conn pointer
		 */
		temp_server->conn = conn;
		LOG_DBG("Server already exists for conn: %p", (void *)conn);
		return -EALREADY;
	}

	struct server_store server;

	ret = server_remove(&server);
	if (ret) {
		return ret;
	}

	server.conn = conn;
	bt_addr_le_copy(&server.addr, peer_addr);

	return server_add(&server);
}

int srv_store_add_by_addr(const bt_addr_le_t *addr)
{
	valid_entry_check(__func__);

	int ret;
	char peer_str[BT_ADDR_LE_STR_LEN];
	struct server_store *temp_server = NULL;

	ret = srv_store_from_addr_get_internal(addr, &temp_server);
	if (ret == 0) {
		bt_addr_le_to_str(addr, peer_str, BT_ADDR_LE_STR_LEN);
		LOG_DBG("Server already exists for addr: %s", peer_str);
		return -EALREADY;
	}

	struct server_store server;

	ret = server_remove(&server);
	if (ret) {
		return ret;
	}

	server.conn = NULL;
	bt_addr_le_copy(&server.addr, addr);

	bt_addr_le_to_str(addr, peer_str, BT_ADDR_LE_STR_LEN);
	LOG_DBG("Adding server for addr: %s", peer_str);

	return server_add(&server);
}

int srv_store_conn_update(struct bt_conn *conn, bt_addr_le_t const *const addr)
{
	valid_entry_check(__func__);
	int ret;

	struct server_store *server = NULL;

	ret = srv_store_from_addr_get_internal(addr, &server);
	if (ret < 0) {
		return ret;
	}

	const bt_addr_le_t *peer_addr = bt_conn_get_dst(conn);

	if (!bt_addr_le_eq(&server->addr, peer_addr)) {
		LOG_ERR("Address does not match the connection's peer address");
		return -EPERM;
	}

	if (server->conn != NULL) {
		LOG_ERR("Server already has a different conn assigned");
		return -EACCES;
	} else if (server->conn == conn) {
		LOG_WRN("Server is already assigned to the same conn");
		return -EALREADY;
	}

	server->conn = conn;

	return 0;
}

int srv_store_clear_by_conn(struct bt_conn *conn)
{
	valid_entry_check(__func__);
	int ret;

	struct server_store *server = NULL;

	ret = srv_store_from_conn_get_internal(conn, &server);
	if (ret < 0) {
		return ret;
	}

	server->name = "NOT_SET";
	server->conn = NULL;
	server->member = NULL;

	memset(&server->snk.eps, 0, sizeof(server->snk.eps));
	memset(&server->src.eps, 0, sizeof(server->src.eps));
	memset(&server->snk.lc3_preset, 0, sizeof(server->snk.lc3_preset));
	memset(&server->src.lc3_preset, 0, sizeof(server->src.lc3_preset));
	memset(&server->snk.codec_caps, 0, sizeof(server->snk.codec_caps));
	memset(&server->src.codec_caps, 0, sizeof(server->src.codec_caps));
	server->snk.num_codec_caps = 0;
	server->src.num_codec_caps = 0;
	server->snk.num_eps = 0;
	server->src.num_eps = 0;
	server->snk.locations = 0;
	server->src.locations = 0;
	server->snk.waiting_for_disc = false;
	server->src.waiting_for_disc = false;

	return 0;
}

int srv_store_remove_by_addr(bt_addr_le_t const *const addr)
{
	valid_entry_check(__func__);

	for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		if (bt_addr_le_eq(&servers[i].addr, addr)) {
			return server_remove(&servers[i]);
		}
	}

	LOG_ERR("Server not found");
	return -ENOENT;
}

int srv_store_remove_by_conn(struct bt_conn const *const conn)
{
	valid_entry_check(__func__);

	struct server_store *server = NULL;
	const bt_addr_le_t *peer_addr = bt_conn_get_dst(conn);

	if (bt_le_bond_exists(BT_ID_DEFAULT, peer_addr)) {
		/* If the peer is bonded, we can use the stored information
		 */
		LOG_DBG("Peer is bonded");
	}

	for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		if (bt_addr_le_eq(&servers[i].addr, peer_addr)) {
			server = &servers[i];
			break;
		}
	}

	if (server == NULL) {
		LOG_ERR("Server does not exist");
		return -ENOENT;
	}

	memset(server, 0, sizeof(struct server_store));
	return 0;
}

static int srv_store_remove_all_internal(void)
{
	valid_entry_check(__func__);
	int ret;

	for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		ret = server_remove(&servers[i]);
		if (ret) {
			LOG_ERR("Failed to remove server at index %d, error: %d", i, ret);
			return ret;
		}
	}
	return 0;
}

int srv_store_remove_all(void)
{
	valid_entry_check(__func__);
	int ret;

	ret = srv_store_remove_all_internal();
	if (ret) {
		return ret;
	}

	return 0;
}

int _srv_store_lock(k_timeout_t timeout, const char *file, int line)
{
	int ret;

	ret = k_sem_take(&sem, timeout);
	if (ret) {

#if CONFIG_DEBUG
		LOG_ERR("Sem take error: %d. Owner: %s Line: %d", ret, owner_file, owner_line);
#else
		LOG_ERR("Sem take error: %d", ret);
#endif
		return ret;
	}

	atomic_ptr_set(&lock_owner, k_current_get());
#if CONFIG_DEBUG
	if (strlen(file) > ARRAY_SIZE(owner_file) - 1) {
		strncpy(owner_file, file, ARRAY_SIZE(owner_file) - 1);
	} else {
		strcpy(owner_file, file);
	}
	owner_line = line;
#endif
	return 0;
}

void srv_store_unlock(void)
{
	valid_entry_check(__func__);

	LOG_DBG("Unlocking srv_store");

	atomic_ptr_set(&lock_owner, NULL);
#if CONFIG_DEBUG
	owner_line = -1;
	owner_file[0] = '\0';
#endif
	k_sem_give(&sem);
}

int srv_store_init(void)
{
	valid_entry_check(__func__);

	return srv_store_remove_all_internal();
}
