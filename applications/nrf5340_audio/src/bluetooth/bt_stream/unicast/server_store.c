/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <string.h>
#include <stdio.h>
#include <../subsys/bluetooth/audio/bap_endpoint.h>

#include "server_store.h"
#include "macros_common.h"
#include "le_audio.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(server_store, CONFIG_SERVER_STORE_LOG_LEVEL);

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

#define MAX_SERVERS (CONFIG_BT_MAX_CONN + CONFIG_BT_MAX_PAIRED)
#define NO_ADDR	    BT_ADDR_LE_ANY

static struct server_store servers[MAX_SERVERS];

/* NOTE: This is set to static as one instance of server_store is very large, we do not
 * want to pull this from whatever stack has called this function. Used when adding new servers
 */
static struct server_store add_server;

struct pd {
	uint32_t min;
	uint32_t pref_min;
	uint32_t pref_max;
	uint32_t max;
};

/* Add a new server */
static int server_add(struct server_store *server)
{
	if (server == NULL) {
		return -EINVAL;
	}

	for (int i = 0; i < MAX_SERVERS; i++) {
		char peer_str[BT_ADDR_LE_STR_LEN];

		bt_addr_le_to_str(&server->addr, peer_str, BT_ADDR_LE_STR_LEN);

		if (bt_addr_le_eq(&servers[i].addr, NO_ADDR)) {
			memcpy(&servers[i], server, sizeof(struct server_store));
			LOG_DBG("Added server %s to index %d", peer_str, i);
			return 0;
		}
	}

	return -ENOMEM;
}

/* Fully remove a server */
static int server_remove(struct server_store *server, bool force)
{
	if (server == NULL) {
		LOG_ERR("Server is NULL");
		return -ENODEV;
	}

	if (server->conn != NULL && !force) {
		LOG_ERR("Cannot remove server with active connection");
		return -EACCES;
	}

	memset(server, 0, sizeof(struct server_store));
	return 0;
}

/* Find the smallest commonly acceptable set of presentation delays */
static int pres_delay_compute(struct bt_bap_qos_cfg_pref *common,
			      struct bt_bap_qos_cfg_pref const *const in)
{
	if (in->pd_min) {
		common->pd_min = MAX(in->pd_min, common->pd_min);
	} else {
		LOG_ERR("pd_min is zero.");
		return -EINVAL;
	}

	if (in->pref_pd_min) {
		common->pref_pd_min = MAX(in->pref_pd_min, common->pref_pd_min);
	}

	if (in->pref_pd_max) {
		common->pref_pd_max = MIN(in->pref_pd_max, common->pref_pd_max);
	}

	if (in->pd_max) {
		common->pd_max = MIN(in->pd_max, common->pd_max);
	} else {
		LOG_ERR("No max presentation delay required");
		return -EINVAL;
	}

	return 0;
}

/* Populate a preset based on preferred sample rate */
static int preset_pop_on_sample_rate(uint16_t lc3_freq_bit, struct bt_bap_lc3_preset *preset,
				     uint8_t pref_sample_rate)
{

	switch (pref_sample_rate) {
	case BT_AUDIO_CODEC_CFG_FREQ_48KHZ:
		if (lc3_freq_bit & BT_AUDIO_CODEC_CAP_FREQ_48KHZ) {
			memcpy(preset, &lc3_preset_48_4_1, sizeof(struct bt_bap_lc3_preset));
			/* SET RTN to 2 to reduce latency. */
			preset->qos.rtn = 2;
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

	/* If no match with the preferred, find highest sample rate */
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

/* Generic parser function for sink and source */
static bool pac_parse(struct bt_data *data, struct bt_bap_lc3_preset *preset,
		      uint8_t pref_sample_rate)
{
	int ret;

	switch (data->type) {
	case BT_AUDIO_CODEC_CAP_TYPE_FREQ:
		uint16_t lc3_freq_bit = sys_get_le16(data->data);

		ret = preset_pop_on_sample_rate(lc3_freq_bit, preset, pref_sample_rate);
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
			LOG_DBG("Preset octets/frame %d not in range [%d, %d]",
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

	/* Did not find what we were looking for, continue parsing LTV */
	return true;
}

/* Parse published audio capabilities (PAC) for sink direction (client->server)/(gateway->headset)
 */
static bool sink_pac_parse(struct bt_data *data, void *user_data)
{
	struct bt_bap_lc3_preset *preset = (struct bt_bap_lc3_preset *)user_data;

	return pac_parse(data, preset, CONFIG_BT_AUDIO_PREF_SINK_SAMPLE_RATE_VALUE);
}

/* Parse published audio capabilities (PAC) for source direction (server->client)/(headset->gateway)
 */
static bool source_pac_parse(struct bt_data *data, void *user_data)
{
	struct bt_bap_lc3_preset *preset = (struct bt_bap_lc3_preset *)user_data;

	return pac_parse(data, preset, CONFIG_BT_AUDIO_PREF_SOURCE_SAMPLE_RATE_VALUE);
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

/* Print all published audio capabilities records */
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

		LOG_INF("\tNum channels supported: %s", supported_chan);
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

static void stream_print(struct bt_bap_qos_cfg_pref const *const qos, bool evaluated,
			 const char *usr_string)
{
	char buf[10] = {0};

	if (evaluated) {
		strcpy(buf, "(Eval)");
	} else {
		strcpy(buf, "(N/A) ");
	}

	LOG_DBG("%s%s\t abs min: %05u pref min: %05u pref max: %05u  abs max: "
		"%05u ",
		usr_string, buf, qos->pd_min, qos->pref_pd_min, qos->pref_pd_max, qos->pd_max);
}

static void done_print(uint8_t existing_streams_checked,
		       struct bt_bap_qos_cfg_pref const *const common_qos,
		       uint32_t computed_pres_dly_us, uint32_t existing_pres_dly_us)
{
	char buf[20] = {0};

	if (computed_pres_dly_us != UINT32_MAX) {
		snprintf(buf, sizeof(buf), "%u", computed_pres_dly_us);
	} else {
		strcpy(buf, "No common value");
	}

	LOG_DBG("Outcome \t\t abs min: %05u pref min: %05u pref max: %05u  abs max: %05u\r\n"
		"\t selected: %s us, existing: %u us, 1 incoming + %d existing stream(s) "
		"compared.",
		common_qos->pd_min, common_qos->pref_pd_min, common_qos->pref_pd_max,
		common_qos->pd_max, buf, existing_pres_dly_us, existing_streams_checked);
}

/* Check to see if the existing stream can be ignored when we try to find a valid presentation delay
 * when a new stream is added.
 */
static bool pres_dly_stream_ignore(struct bt_bap_stream const *const existing_stream,
				   struct bt_bap_stream const *const stream_in)
{
	int ret;

	if (existing_stream == NULL || stream_in == NULL) {
		LOG_ERR("NULL parameter");
		return true;
	}

	if (existing_stream->group == NULL) {
		return true;
	}

	if (existing_stream == stream_in) {
		/* The existing stream is the same as the incoming stream */
		return true;
	}

	if (existing_stream->ep == NULL) {
		return true;
	}

	struct bt_bap_ep_info ep_info_existing_stream;
	struct bt_bap_ep_info ep_info_stream_in;

	ret = bt_bap_ep_get_info(existing_stream->ep, &ep_info_existing_stream);
	if (ret != 0) {
		LOG_ERR("Failed to get existing stream ep info: %d", ret);
		return true;
	}

	ret = bt_bap_ep_get_info(stream_in->ep, &ep_info_stream_in);
	if (ret != 0) {
		LOG_ERR("Failed to get existing stream ep info: %d", ret);
		return true;
	}

	if (ep_info_existing_stream.dir != ep_info_stream_in.dir) {
		/* The existing stream is not in the same direction as the incoming stream */
		LOG_DBG("Existing stream not in same direction as incoming stream");
		return true;
	}

	/* Check if the existing stream has gotten into a codec configured, QoS configured,
	 * enabling or streaming state.
	 */
	if (!le_audio_ep_state_check(existing_stream->ep, BT_BAP_EP_STATE_CODEC_CONFIGURED) &&
	    !le_audio_ep_state_check(existing_stream->ep, BT_BAP_EP_STATE_QOS_CONFIGURED) &&
	    !le_audio_ep_state_check(existing_stream->ep, BT_BAP_EP_STATE_ENABLING) &&
	    !le_audio_ep_state_check(existing_stream->ep, BT_BAP_EP_STATE_STREAMING)) {
		LOG_DBG("Existing stream not in codec configured, QoS configured, enabling or "
			"streaming state");
		return true;
	}

	stream_print(&existing_stream->ep->qos_pref, true, "Existing");
	return false;
}

int srv_store_foreach_server(srv_store_foreach_func_t func, void *user_data)
{
	valid_entry_check(__func__);

	if (func == NULL) {
		return -EINVAL;
	}

	/* Call the given function for each server with a valid conn pointer */
	for (int i = 0; i < MAX_SERVERS; i++) {
		struct server_store *server = &servers[i];

		if (server->conn == NULL) {
			continue;
		}

		const bool keep_iterating = func(server, user_data);

		if (!keep_iterating) {
			return -ECANCELED;
		}
	}

	return 0;
}

bool srv_store_preset_validated(struct bt_audio_codec_cfg const *const new,
				struct bt_audio_codec_cfg const *const existing,
				uint8_t pref_sample_rate_value)
{
	valid_entry_check(__func__);

	int ret;

	if (new == NULL || existing == NULL) {
		return false;
	}

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

struct foreach_stream_data {
	int ret;
	struct bt_bap_ep_info *incoming_ep_info;
	struct bt_bap_qos_cfg_pref *common_qos;
	struct bt_bap_qos_cfg_pref const *incoming_qos;
	struct bt_bap_stream const *const incoming_stream;
	uint8_t *streams_checked;
	uint32_t *existing_pres_dly_us;
	uint32_t *computed_pres_dly_us;
	uint32_t *existing_pres_dly_us_check;
	uint16_t *existing_trans_lat_ms;
	bool *group_reconfig_needed;
	bool existing_pres_dly_already_in_range;
};

static bool stream_check_pd(struct bt_cap_stream *existing_stream, void *user_data)
{
	struct foreach_stream_data *ctx = (struct foreach_stream_data *)user_data;

	if (existing_stream->bap_stream.group != ctx->incoming_stream->group) {
		/* The existing stream is not in the same group as the incoming stream */
		LOG_ERR("Existing stream group (%p) not same as incoming stream group (%p)",
			(void *)existing_stream->bap_stream.group,
			(void *)ctx->incoming_stream->group);
		ctx->ret = -EINVAL;
		return true;
	}

	if (pres_dly_stream_ignore(&(existing_stream->bap_stream), ctx->incoming_stream)) {
		/* Do not consider this stream. Continue parsing */
		LOG_DBG("Ignoring existing stream");
		return false;
	}

	/* All already running streams in the same direction and in the
	 * same group (CIG) shall have the same presentation delay.
	 */
	*ctx->existing_pres_dly_us = existing_stream->bap_stream.qos->pd;

	if (*ctx->existing_pres_dly_us_check == UINT32_MAX) {
		ctx->existing_pres_dly_us_check = ctx->existing_pres_dly_us;
	} else if (*ctx->existing_pres_dly_us_check != *ctx->existing_pres_dly_us) {
		LOG_ERR("Illegal value. Pres delays do not match: %u != %u",
			*ctx->existing_pres_dly_us_check, *ctx->existing_pres_dly_us);
		ctx->ret = -EINVAL;
		return true;
	}

	if (*ctx->existing_pres_dly_us == 0) {
		LOG_ERR("Existing presentation delay is zero");
		ctx->ret = -EINVAL;
		return true;
	}

	if (IN_RANGE(*ctx->existing_pres_dly_us, ctx->incoming_qos->pd_min,
		     ctx->incoming_qos->pd_max)) {
		*ctx->computed_pres_dly_us = *ctx->existing_pres_dly_us;
		LOG_INF("Existing pres delay is within the incoming stream QoS range");
		ctx->existing_pres_dly_already_in_range = true;
		return true;
	}

	*ctx->group_reconfig_needed = true;
	(*ctx->streams_checked)++;

	ctx->ret = pres_delay_compute(ctx->common_qos, &existing_stream->bap_stream.ep->qos_pref);
	if (ctx->ret) {
		return true;
	}

	/* Continue iteration */
	return false;
}

/* Find the presentation delay. Needs to be the same within a CIG for a given direction */
int srv_store_pres_dly_find(struct bt_bap_stream *stream, uint32_t *computed_pres_dly_us,
			    uint32_t *existing_pres_dly_us,
			    struct bt_bap_qos_cfg_pref const *server_qos_pref,
			    bool *group_reconfig_needed, struct bt_cap_unicast_group *unicast_group)
{
	valid_entry_check(__func__);

	int ret;

	if (stream == NULL || computed_pres_dly_us == NULL || existing_pres_dly_us == NULL ||
	    server_qos_pref == NULL || group_reconfig_needed == NULL) {
		LOG_ERR("NULL parameter");

		return -EINVAL;
	}

	*existing_pres_dly_us = 0;
	*group_reconfig_needed = false;
	*computed_pres_dly_us = UINT32_MAX;
	uint32_t existing_pres_dly_us_check = UINT32_MAX;
	uint8_t existing_streams_checked = 0;

	if (stream->group == NULL) {
		LOG_ERR("The incoming stream %p has no group", (void *)stream);
		*computed_pres_dly_us = UINT32_MAX;

		return -EINVAL;
	}

	if (server_qos_pref->pd_min == 0 || server_qos_pref->pd_max == 0) {
		LOG_ERR("Incoming pd_min or pd_max is zero");
		*computed_pres_dly_us = UINT32_MAX;

		return -EINVAL;
	}

	stream_print(server_qos_pref, true, "Incoming");

	/* Common QoS with most permissive values */
	struct bt_bap_qos_cfg_pref common_qos = {
		.pd_min = 0,
		.pref_pd_min = 0,
		.pref_pd_max = UINT32_MAX,
		.pd_max = UINT32_MAX,
	};

	ret = pres_delay_compute(&common_qos, server_qos_pref);
	if (ret) {
		LOG_ERR("Failed to find initial common presentation delay: %d", ret);
		*computed_pres_dly_us = UINT32_MAX;

		return ret;
	}

	struct bt_bap_ep_info ep_info;

	ret = bt_bap_ep_get_info(stream->ep, &ep_info);
	if (ret) {
		LOG_ERR("Failed to get ep! info: %d", ret);
		*computed_pres_dly_us = UINT32_MAX;

		return ret;
	}

	struct foreach_stream_data foreach_data = {
		.ret = 0,
		.incoming_ep_info = &ep_info,
		.incoming_qos = server_qos_pref,
		.common_qos = &common_qos,
		.incoming_stream = stream,
		.streams_checked = &existing_streams_checked,
		.group_reconfig_needed = group_reconfig_needed,
		.existing_pres_dly_us = existing_pres_dly_us,
		.existing_pres_dly_us_check = &existing_pres_dly_us_check,
		.computed_pres_dly_us = computed_pres_dly_us,
		.existing_pres_dly_already_in_range = false,
	};

	ret = bt_cap_unicast_group_foreach_stream(unicast_group, stream_check_pd,
						  (void *)&foreach_data);
	if (foreach_data.ret) {
		LOG_ERR("Failed to compute presentation delay");
		*computed_pres_dly_us = UINT32_MAX;

		return foreach_data.ret;
	}

	if (ret != 0 && ret != -ECANCELED) {
		LOG_ERR("Failed to iterate streams in group: %d", ret);
		*computed_pres_dly_us = UINT32_MAX;

		return ret;
	}

	LOG_DBG("Checked %d stream(s) in the same group", existing_streams_checked);

	if (foreach_data.existing_pres_dly_already_in_range) {
		LOG_DBG("Existing pres delay already in range, no reconfig needed");

		ret = 0;
		goto print_and_return;
	}

	if (existing_streams_checked == 0) {
		LOG_DBG("No other streams in the same group found");

		/* No other streams in the same group found */
		if (server_qos_pref->pref_pd_min == 0) {
			*computed_pres_dly_us = common_qos.pd_min;
		} else {
			*computed_pres_dly_us = common_qos.pref_pd_min;
		}

		ret = 0;
		goto print_and_return;
	}

	if (!group_reconfig_needed) {
		ret = 0;
		goto print_and_return;
	}

	if (common_qos.pd_min > common_qos.pd_max) {
		LOG_ERR("No common ground for pd_min %u and pd_max %u", common_qos.pd_min,
			common_qos.pd_max);
		*computed_pres_dly_us = UINT32_MAX;

		ret = -ESPIPE;
		goto print_and_return;
	}

	/* No streams have a preferred min */
	if (common_qos.pref_pd_min == 0) {
		*computed_pres_dly_us = common_qos.pd_min;

		ret = 0;
		goto print_and_return;
	}

	if (common_qos.pref_pd_min < common_qos.pd_min) {
		/* Preferred min is lower than min, use min */
		LOG_ERR("pref PD min is lower than min. Using min");
		*computed_pres_dly_us = common_qos.pd_min;
	} else if (common_qos.pref_pd_min > common_qos.pd_min &&
		   common_qos.pref_pd_min <= common_qos.pd_max) {
		/* Preferred min is in range, use pref min */
		*computed_pres_dly_us = common_qos.pref_pd_min;
	} else {
		*computed_pres_dly_us = common_qos.pd_min;
	}

print_and_return:
	done_print(existing_streams_checked, &common_qos, *computed_pres_dly_us,
		   *existing_pres_dly_us);
	return ret;
}

static bool stream_check_max_trans_lat(struct bt_cap_stream *existing_stream, void *user_data)
{
	struct foreach_stream_data *ctx = (struct foreach_stream_data *)user_data;

	LOG_INF("Incoming stream %p Existing stream %p", (void *)ctx->incoming_stream,
		(void *)&existing_stream->bap_stream);

	if (existing_stream->bap_stream.group != ctx->incoming_stream->group) {
		/* The existing stream is not in the same group as the incoming stream */
		LOG_ERR("Existing stream group (%p) not same as incoming stream group (%p)",
			(void *)existing_stream->bap_stream.group,
			(void *)ctx->incoming_stream->group);
		ctx->ret = -EINVAL;
		return true;
	}

	if (ctx->incoming_stream == &existing_stream->bap_stream) {
		/* The existing stream is the same as the incoming stream */
		LOG_DBG("Existing stream is the incoming stream, skipping");
		return false;
	}

	if (existing_stream->bap_stream.ep == NULL) {
		/* The existing stream we are comparing agains has not yet been through
		 * the ep configured callback
		 * This means that this A<->B comparison will be done later, when stream B
		 * has been through the ep configured callback
		 */
		LOG_DBG("Existing stream has no ep set yet.");
		return false;
	}

	int existing_dir = le_audio_stream_dir_get(&existing_stream->bap_stream);
	int incoming_dir = le_audio_stream_dir_get(ctx->incoming_stream);

	if (existing_dir < 0) {
		LOG_ERR("Failed to get existing bap stream (%p) direction",
			(void *)&existing_stream->bap_stream);
		ctx->ret = -EINVAL;
		return true;
	}

	if (incoming_dir < 0) {
		LOG_ERR("Failed to get incoming stream direction");
		ctx->ret = -EINVAL;
		return true;
	}

	if (existing_dir != incoming_dir) {
		LOG_DBG("Existing stream not in same direction as incoming stream");
		return false;
	}

	(*ctx->streams_checked)++;
	LOG_INF("Stream checked %d", *ctx->streams_checked);

	if (*ctx->existing_trans_lat_ms == UINT16_MAX) {
		/* All of the existing_stream->bap_stream.qos->latency will be updated if the new
		 * value is lower.
		 */
		*ctx->existing_trans_lat_ms = existing_stream->bap_stream.qos->latency;
		LOG_INF("First trans lat set: %u ms", ctx->incoming_qos->latency);
	} else if (*ctx->existing_trans_lat_ms != existing_stream->bap_stream.qos->latency) {
		LOG_ERR("Illegal value. Max transport latencies do not match: %u != %u",
			*ctx->existing_trans_lat_ms, existing_stream->bap_stream.qos->latency);
		return true;
	}

	return false;
}

int srv_store_max_trans_lat_find(struct bt_bap_stream const *const stream,
				 struct bt_bap_qos_cfg_pref const *const server_qos_pref,
				 uint16_t *new_max_trans_lat_ms,
				 uint16_t *existing_max_trans_lat_ms, bool *group_reconfig_needed,
				 struct bt_cap_unicast_group *unicast_group)
{
	int ret;

	valid_entry_check(__func__);

	if (stream == NULL || new_max_trans_lat_ms == NULL || server_qos_pref == NULL ||
	    group_reconfig_needed == NULL || existing_max_trans_lat_ms == NULL) {
		LOG_ERR("NULL parameter!");

		return -EINVAL;
	}

	*existing_max_trans_lat_ms = UINT16_MAX;

	/* All streams of a given direction within the CIG must have the same
	 * transport latency. See BAP spec (BAP_v1.0.2 section 7.2.1)
	 */

	uint8_t existing_streams_checked = 0;

	struct foreach_stream_data foreach_data = {
		.ret = 0,
		.incoming_qos = server_qos_pref,
		.incoming_stream = stream,
		.streams_checked = &existing_streams_checked,
		.group_reconfig_needed = group_reconfig_needed,
		.existing_trans_lat_ms = existing_max_trans_lat_ms,
	};

	ret = bt_cap_unicast_group_foreach_stream(unicast_group, stream_check_max_trans_lat,
						  (void *)&foreach_data);
	if (ret != 0 && ret != -ECANCELED) {
		/* There shall already be at least one server added at this point */
		LOG_ERR("Failed to iterate streams in group: %d", ret);
		return ret;
	}

	if (foreach_data.ret) {
		LOG_ERR("Failed to compute max transport latency");
		return foreach_data.ret;
	}

	LOG_INF("Checked %d stream(s) in the same group", existing_streams_checked);
	if (existing_streams_checked == 0) {
		/* No other streams in CIG, use server_qos_pref latency*/
		LOG_INF("No existing trans lat, setting to new: %u ms", server_qos_pref->latency);
		*new_max_trans_lat_ms = server_qos_pref->latency;
		*group_reconfig_needed = false;
	} else if (*existing_max_trans_lat_ms < server_qos_pref->latency) {
		/* Existing max transport latency is less than the new one, need to configure the
		 * new/incoming stream
		 */
		LOG_INF("Existing max transport latency %u ms < incoming qos_pref %d ms",
			*existing_max_trans_lat_ms, server_qos_pref->latency);
		*new_max_trans_lat_ms = *existing_max_trans_lat_ms;
		*group_reconfig_needed = false;
	} else if (*existing_max_trans_lat_ms > server_qos_pref->latency) {
		/* Existing latency is larger than the new one, need to re-configure
		 * existing streams
		 */
		LOG_INF("Existing max transport latency %u ms > incoming qos_pref %d ms",
			*existing_max_trans_lat_ms, server_qos_pref->latency);
		*new_max_trans_lat_ms = server_qos_pref->latency;
		*group_reconfig_needed = true;
	} else if (*existing_max_trans_lat_ms == server_qos_pref->latency) {
		/* Existing max transport latency is equal to the pref of the new stream, no
		 * re-config
		 */
		LOG_INF("Max transport latency unchanged at %d ms", *existing_max_trans_lat_ms);
		*new_max_trans_lat_ms = *existing_max_trans_lat_ms;
		*group_reconfig_needed = false;
	} else {
		LOG_ERR("Unreachable");
		return -EINVAL;
	}

	return 0;
}

struct foreach_trans_lat_store {
	enum bt_audio_dir dir;
	uint16_t max_trans_lat_ms;
	uint8_t streams_checked;
};

static bool stream_trans_lat_set(struct bt_cap_stream *existing_stream, void *user_data)
{
	struct foreach_trans_lat_store *ctx = (struct foreach_trans_lat_store *)user_data;

	if (existing_stream->bap_stream.ep == NULL) {
		/* Another stream may not yet have been through the stream_configured_cb
		 * This is no problem, as the last stream to go through will set the
		 * latency for the other streams.
		 */
		LOG_DBG("Existing stream has no ep set yet.");
		return false;
	}

	int existing_dir = le_audio_stream_dir_get(&existing_stream->bap_stream);
	if (existing_dir < 0) {
		LOG_ERR("Failed stream direction for BAP stream %p",
			(void *)&existing_stream->bap_stream);
		return false;
	}

	if (existing_dir != ctx->dir) {
		LOG_WRN("Existing stream not in same direction as incoming stream");
		return false;
	}

	ctx->streams_checked++;

	existing_stream->bap_stream.qos->latency = ctx->max_trans_lat_ms;
	LOG_INF("Max trans lat %d ms set for BAP stream %p checked %d other streams",
		existing_stream->bap_stream.qos->latency, (void *)&existing_stream->bap_stream,
		ctx->streams_checked);

	return false;
}

int srv_store_max_trans_lat_set(struct bt_cap_unicast_group *unicast_group, enum bt_audio_dir dir,
				uint16_t max_trans_lat_ms)
{
	valid_entry_check(__func__);

	if (unicast_group == NULL) {
		LOG_ERR("No valid unicast group pointer received");
		return -EINVAL;
	}

	int ret;

	struct foreach_trans_lat_store trans_lat_store = {
		.dir = dir, .max_trans_lat_ms = max_trans_lat_ms, .streams_checked = 0};

	ret = bt_cap_unicast_group_foreach_stream(unicast_group, stream_trans_lat_set,
						  (void *)&trans_lat_store);

	if (ret != 0 && ret != -ECANCELED) {
		LOG_ERR("Failed to iterate streams in group: %d", ret);
		return ret;
	}

	if (trans_lat_store.streams_checked == 0) {
		LOG_ERR("No streams to set");
		return -ECHILD;
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

	ret = srv_store_from_conn_get(conn, &server);
	if (ret) {
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

	if (conn == NULL || valid_codec_caps == NULL) {
		LOG_ERR("Invalid parameters: conn or valid_codec_caps is NULL");
		return -EINVAL;
	}

	*valid_codec_caps = 0;
	struct server_store *server = NULL;

	ret = srv_store_from_conn_get(conn, &server);
	if (ret) {
		LOG_ERR("Unknown connection, should not reach here");
		return ret;
	}

	/* Only the sampling frequency is checked */
	if (dir == BT_AUDIO_DIR_SINK) {
		LOG_DBG("Discovered %d sink endpoint(s) for server", server->snk.num_eps);

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

			LOG_DBG("Valid codec capabilities found for server, sink EP %d", i);
			*valid_codec_caps |= 1 << i;

			if (srv_store_preset_validated(
				    &(preset.codec_cfg), &(server->snk.lc3_preset[0].codec_cfg),
				    CONFIG_BT_AUDIO_PREF_SINK_SAMPLE_RATE_VALUE)) {
				memcpy(&server->snk.lc3_preset[0], &preset,
				       sizeof(struct bt_bap_lc3_preset));
			}
		}
	} else if (dir == BT_AUDIO_DIR_SOURCE) {
		LOG_DBG("Discovered %d source endpoint(s) for server", server->src.num_eps);

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

			LOG_DBG("Valid codec capabilities found for server, source EP %d", i);
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

	if (stream == NULL || server == NULL) {
		LOG_ERR("Invalid parameters: stream or server is NULL");
		return -EINVAL;
	}

	*server = NULL;

	for (int srv_idx = 0; srv_idx < MAX_SERVERS; srv_idx++) {
		tmp_server = &servers[srv_idx];
		if (bt_addr_le_eq(&tmp_server->addr, NO_ADDR)) {
			continue;
		}
		for (int i = 0; i < CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT; i++) {

			LOG_DBG("Checking server %d, sink stream %d %p %p", srv_idx, i,
				&tmp_server->snk.cap_streams[i].bap_stream, stream);

			if (&tmp_server->snk.cap_streams[i].bap_stream == stream) {
				*server = tmp_server;
				LOG_DBG("Found server for sink stream %p at index %d", stream,
					srv_idx);
				matches++;
			}
		}

		for (int i = 0; i < CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT; i++) {
			LOG_DBG("Checking server %d, source stream %d", srv_idx, i);
			if (&tmp_server->src.cap_streams[i].bap_stream == stream) {
				*server = tmp_server;
				LOG_DBG("Found server for source stream %p at index %d", stream,
					srv_idx);
				matches++;
			}
		}
	}

	if (matches == 0) {
		LOG_ERR("No server found for the given stream");
		*server = NULL;

		return -ENOENT;
	} else if (matches > 1) {
		LOG_ERR("Multiple servers found for the same stream. Should not happen");
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

	if (conn == NULL) {
		LOG_ERR("NULL parameter");
		return -EINVAL;
	}

	ret = srv_store_from_conn_get(conn, &server);
	if (ret) {
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

	for (int srv_idx = 0; srv_idx < MAX_SERVERS; srv_idx++) {
		server = &servers[srv_idx];
		if (bt_addr_le_eq(&server->addr, NO_ADDR)) {
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
	if (ret) {
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

	if (conn == NULL || codec == NULL) {
		LOG_ERR("NULL parameter");
		return -EINVAL;
	}

	if (codec->data_len > CONFIG_BT_AUDIO_CODEC_CAP_MAX_DATA_SIZE) {
		LOG_ERR("Codec data length exceeds maximum size: %d", codec->data_len);
		return -ENOMEM;
	}

	struct server_store *server = NULL;

	ret = srv_store_from_conn_get(conn, &server);
	if (ret) {
		return ret;
	}

	if (codec->data_len > CONFIG_BT_AUDIO_CODEC_CAP_MAX_DATA_SIZE) {
		LOG_ERR("Codec data length exceeds maximum size: %d", codec->data_len);
		return -EINVAL;
	}

	if (dir == BT_AUDIO_DIR_SINK) {
		/* num_codec_caps is an increasing index that starts at 0 */
		if (server->snk.num_codec_caps >= ARRAY_SIZE(server->snk.codec_caps)) {
			LOG_WRN("No more space (%d) for sink codec capabilities, increase "
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
			LOG_WRN("No more space for source codec capabilities, increase "
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

	if (addr == NULL || server == NULL) {
		LOG_ERR("NULL parameter");
		return -EINVAL;
	}

	for (int i = 0; i < MAX_SERVERS; i++) {
		if (bt_addr_le_eq(&servers[i].addr, addr)) {
			*server = &servers[i];
			return 0;
		}
	}

	*server = NULL;
	return -ENOENT;
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

	ret = srv_store_from_addr_get(addr, &temp_server);
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

	if (conn == NULL || server == NULL) {
		LOG_ERR("NULL parameter");
		return -EINVAL;
	}

	ret = srv_store_from_addr_get(bt_conn_get_dst(conn), server);
	if (ret) {
		return ret;
	}

	if ((*server)->conn != conn) {
		LOG_DBG("Conn %p does not match stored %p", conn, (*server)->conn);
		return -ENOTCONN;
	}

	return 0;
}

int srv_store_num_get(void)
{
	valid_entry_check(__func__);
	int num_servers = 0;

	for (int i = 0; i < MAX_SERVERS; i++) {
		if (!bt_addr_le_eq(&servers[i].addr, NO_ADDR)) {
			num_servers++;
		}
	}

	return num_servers;
}

int srv_store_add_by_conn(struct bt_conn *conn)
{
	valid_entry_check(__func__);

	int ret;
	struct server_store *temp_server = NULL;

	if (conn == NULL) {
		LOG_ERR("NULL parameter");
		return -EINVAL;
	}

	const bt_addr_le_t *peer_addr = bt_conn_get_dst(conn);
	char peer_str[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(peer_addr, peer_str, BT_ADDR_LE_STR_LEN);

	LOG_DBG("Adding server by conn for peer: %s", peer_str);

	/* Check if server already exists */
	ret = srv_store_from_conn_get(conn, &temp_server);
	if (ret == 0) {
		/* Server already exists, no need to add again, but we update the conn
		 * pointer */
		temp_server->conn = conn;
		LOG_DBG("Server already exists for conn: %p", (void *)conn);
		return -EALREADY;
	}

	/* Clear the temporary add_server before populating it */
	add_server.conn = NULL;

	ret = server_remove(&add_server, false);
	if (ret) {
		return ret;
	}

	add_server.conn = conn;
	bt_addr_le_copy(&add_server.addr, peer_addr);

	return server_add(&add_server);
}

int srv_store_add_by_addr(const bt_addr_le_t *addr)
{
	valid_entry_check(__func__);

	int ret;
	char peer_str[BT_ADDR_LE_STR_LEN];
	struct server_store *temp_server = NULL;

	if (addr == NULL) {
		LOG_ERR("NULL parameter");
		return -EINVAL;
	}

	ret = srv_store_from_addr_get(addr, &temp_server);
	if (ret == 0) {
		bt_addr_le_to_str(addr, peer_str, BT_ADDR_LE_STR_LEN);
		LOG_DBG("Server already exists for addr: %s", peer_str);
		return -EALREADY;
	}

	/* Clear the temporary add_server before populating it */
	add_server.conn = NULL;

	ret = server_remove(&add_server, false);
	if (ret) {
		return ret;
	}

	bt_addr_le_copy(&add_server.addr, addr);

	bt_addr_le_to_str(addr, peer_str, BT_ADDR_LE_STR_LEN);
	LOG_DBG("Adding server for addr: %s", peer_str);

	return server_add(&add_server);
}

int srv_store_conn_update(struct bt_conn *conn, bt_addr_le_t const *const addr)
{
	valid_entry_check(__func__);
	int ret;

	struct server_store *server = NULL;

	if (conn == NULL || addr == NULL) {
		LOG_ERR("NULL parameter");
		return -EINVAL;
	}

	ret = srv_store_from_addr_get(addr, &server);
	if (ret) {
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

int srv_store_clear_by_conn(struct bt_conn const *const conn)
{
	valid_entry_check(__func__);

	int ret;

	if (conn == NULL) {
		LOG_ERR("NULL parameter");
		return -EINVAL;
	}

	struct server_store *server = NULL;

	ret = srv_store_from_conn_get(conn, &server);

	if (ret == -ENOENT) {
		return 0;
	} else if (ret == -ENOTCONN) {
		LOG_DBG("Server exists different conn pointer, clearing anyway");
	} else if (ret) {
		return ret;
	}

	/* Address is not cleared */
	server->name = "NOT_SET";
	server->conn = NULL;
	server->member = NULL;

	server->snk.waiting_for_disc = false;
	server->src.waiting_for_disc = false;
	server->snk.locations = 0;
	server->src.locations = 0;
	memset(&server->snk.lc3_preset, 0, sizeof(server->snk.lc3_preset));
	memset(&server->src.lc3_preset, 0, sizeof(server->src.lc3_preset));
	memset(&server->snk.codec_caps, 0, sizeof(server->snk.codec_caps));
	memset(&server->src.codec_caps, 0, sizeof(server->src.codec_caps));
	server->snk.num_codec_caps = 0;
	server->src.num_codec_caps = 0;
	memset(&server->snk.eps, 0, sizeof(server->snk.eps));
	memset(&server->src.eps, 0, sizeof(server->src.eps));
	server->snk.num_eps = 0;
	server->src.num_eps = 0;
	server->snk.supported_ctx = 0;
	server->src.supported_ctx = 0;
	server->snk.available_ctx = 0;
	server->src.available_ctx = 0;
	/* Streams are not touched as these are handled by the stack.
	 * Clearing them here may lead to faults as they are needed beyond ACL disconnection
	 */

	return 0;
}

int srv_store_remove_by_addr(bt_addr_le_t const *const addr)
{
	valid_entry_check(__func__);
	int ret;

	struct server_store *server = NULL;

	ret = srv_store_from_addr_get(addr, &server);
	if (ret) {
		return ret;
	}

	return server_remove(server, false);
}

int srv_store_remove_by_conn(struct bt_conn const *const conn)
{
	valid_entry_check(__func__);
	int ret;

	if (conn == NULL) {
		LOG_ERR("NULL parameter");
		return -EINVAL;
	}

	struct server_store *server = NULL;

	ret = srv_store_from_conn_get(conn, &server);
	if (ret == -ENOENT) {
		LOG_WRN("Server does not exist");
		return ret;
	} else if (ret) {
		return ret;
	}

	return server_remove(server, true);
}

int srv_store_remove_all(bool force)
{
	valid_entry_check(__func__);
	int ret;

	for (int i = 0; i < MAX_SERVERS; i++) {
		ret = server_remove(&servers[i], force);
		if (ret) {
			LOG_ERR("Failed to remove server at index %d, error: %d", i, ret);
			return ret;
		}
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

	return srv_store_remove_all(false);
}

struct foreach_stream_pres_dly {
	int ret;
	struct bt_cap_unicast_group *unicast_group;
	struct bt_bap_qos_cfg_pref *common_qos_snk;
	struct bt_bap_qos_cfg_pref *common_qos_src;
	uint8_t streams_checked_snk;
	uint8_t streams_checked_src;
	uint32_t existing_pres_dly_us_snk;
	uint32_t existing_pres_dly_us_src;
};

static bool streams_calc_pres_dly(struct bt_cap_stream *stream, void *user_data)
{
	int ret;
	struct foreach_stream_pres_dly *ctx = (struct foreach_stream_pres_dly *)user_data;

	if (stream->bap_stream.group != ctx->unicast_group) {
		/* The existing stream is not in the same group as we are checking.
		This is not an error as such, but the system for now supports a single group.*/
		LOG_ERR("Existing stream group (%p) not same as incoming stream group (%p)",
			(void *)ctx->unicast_group, (void *)ctx->unicast_group);
		ctx->ret = -EINVAL;
		return true;
	}

	if (stream->bap_stream.ep == NULL) {
		LOG_ERR("Existing stream has no ep set yet.");
		ctx->ret = -EINVAL;
		return true;
	}

	if (!le_audio_ep_state_check(stream->bap_stream.ep, BT_BAP_EP_STATE_CODEC_CONFIGURED) &&
	    !le_audio_ep_state_check(stream->bap_stream.ep, BT_BAP_EP_STATE_QOS_CONFIGURED) &&
	    !le_audio_ep_state_check(stream->bap_stream.ep, BT_BAP_EP_STATE_ENABLING) &&
	    !le_audio_ep_state_check(stream->bap_stream.ep, BT_BAP_EP_STATE_STREAMING)) {
		LOG_DBG("Existing stream not in codec configured, QoS configured, enabling or "
			"streaming state");
		return true;
	}

	struct bt_bap_ep_info ep_info;

	ret = bt_bap_ep_get_info(stream->bap_stream.ep, &ep_info);
	if (ret != 0) {
		LOG_ERR("Failed to get existing stream ep info: %d", ret);
		return true;
	}

	if (ep_info.dir == BT_AUDIO_DIR_SINK) {
		/* Sink stream */
		ctx->streams_checked_snk++;
		if (ctx->existing_pres_dly_us_snk == UINT32_MAX) {
			ctx->existing_pres_dly_us_snk = stream->bap_stream.qos->pd;
		} else if (ctx->existing_pres_dly_us_snk != stream->bap_stream.qos->pd) {
			LOG_ERR("Existing sink streams have differing presentation delays: %u us "
				"and %u us",
				ctx->existing_pres_dly_us_snk, stream->bap_stream.qos->pd);
			ctx->ret = -EINVAL;
			return true;
		}

		if (ctx->existing_pres_dly_us_snk == 0) {
			LOG_ERR("Existing presentation delay is zero");
			ctx->ret = -EINVAL;
			return true;
		}

		ctx->ret =
			pres_delay_compute(ctx->common_qos_snk, &stream->bap_stream.ep->qos_pref);
		if (ctx->ret) {
			return true;
		}

	} else if (ep_info.dir == BT_AUDIO_DIR_SOURCE) {
		/* Source stream */
		ctx->streams_checked_src++;
		if (ctx->existing_pres_dly_us_src == UINT32_MAX) {
			ctx->existing_pres_dly_us_src = stream->bap_stream.qos->pd;
		} else if (ctx->existing_pres_dly_us_src != stream->bap_stream.qos->pd) {
			LOG_ERR("Existing sink streams have differing presentation delays: %u us "
				"and %u us",
				ctx->existing_pres_dly_us_src, stream->bap_stream.qos->pd);
			ctx->ret = -EINVAL;
			return true;
		}

		if (ctx->existing_pres_dly_us_snk == 0) {
			LOG_ERR("Existing presentation delay is zero");
			ctx->ret = -EINVAL;
			return true;
		}

		ctx->ret =
			pres_delay_compute(ctx->common_qos_src, &stream->bap_stream.ep->qos_pref);
		if (ctx->ret) {
			return true;
		}
	} else {
		LOG_ERR("Unknown stream direction");
		ctx->ret = -EINVAL;
		return true;
	}

	/* Continue iteration */
	return false;
}

static int parse_common_qos_for_pres_dly(struct bt_bap_qos_cfg_pref *common_qos,
					 uint32_t *computed_pres_dly_us)
{

	if (common_qos->pd_min > common_qos->pd_max) {
		LOG_ERR("No common ground for pd_min %u and pd_max %u", common_qos->pd_min,
			common_qos->pd_max);
		*computed_pres_dly_us = UINT32_MAX;

		return -ESPIPE;
	}

	/* No streams have a preferred min */
	if (common_qos->pref_pd_min == 0) {
		*computed_pres_dly_us = common_qos->pd_min;

		return 0;
	}

	if (common_qos->pref_pd_min < common_qos->pd_min) {
		/* Preferred min is lower than min, use min */
		LOG_ERR("pref PD min is lower than min. Using min");
		*computed_pres_dly_us = common_qos->pd_min;
	} else if (common_qos->pref_pd_min > common_qos->pd_min &&
		   common_qos->pref_pd_min <= common_qos->pd_max) {
		/* Preferred min is in range, use pref min */
		*computed_pres_dly_us = common_qos->pref_pd_min;
	} else {
		*computed_pres_dly_us = common_qos->pd_min;
	}

	return 0;
}

static void pd_print(struct bt_bap_qos_cfg_pref pref, uint32_t existing_pd_us,
		     uint32_t calculated_pd_us)
{
	char existing_pd_buf[20] = {0};
	char calculated_pd_buf[20] = {0};

	if (existing_pd_us != UINT32_MAX) {
		snprintf(existing_pd_buf, sizeof(existing_pd_buf), "%u", existing_pd_us);
	} else {
		strcpy(existing_pd_buf, "N/A");
	}

	if (calculated_pd_us != UINT32_MAX) {
		snprintf(calculated_pd_buf, sizeof(calculated_pd_buf), "%u", calculated_pd_us);
	} else {
		strcpy(calculated_pd_buf, "N/A");
	}

	LOG_INF("PD: \t\t abs min: %05u pref min: %05u pref max: %05u  abs max: %05u\r\n"
		"\t selected: %s us, existing: %s us",
		pref.pd_min, pref.pref_pd_min, pref.pref_pd_max, pref.pd_max, calculated_pd_buf,
		existing_pd_buf);
}

// This needs to be called only once, after all streams have been through the configured cb
int srv_store_pres_delay_get(struct bt_cap_unicast_group *unicast_group, uint32_t *pres_dly_snk_us,
			     uint32_t *pres_dly_src_us)
{
	valid_entry_check(__func__);
	int ret;

	if (unicast_group == NULL || pres_dly_snk_us == NULL || pres_dly_src_us == NULL) {
		LOG_ERR("NULL parameter");
		return -EINVAL;
	}

	/* Common QoS with most permissive values */
	struct bt_bap_qos_cfg_pref common_qos_snk = {
		.pd_min = 0,
		.pref_pd_min = 0,
		.pref_pd_max = UINT32_MAX,
		.pd_max = UINT32_MAX,
	};

	struct bt_bap_qos_cfg_pref common_qos_src = {
		.pd_min = 0,
		.pref_pd_min = 0,
		.pref_pd_max = UINT32_MAX,
		.pd_max = UINT32_MAX,
	};

	struct foreach_stream_pres_dly foreach_data = {
		.ret = 0,
		.unicast_group = unicast_group,
		.common_qos_snk = &common_qos_snk,
		.common_qos_src = &common_qos_src,
		.streams_checked_snk = 0,
		.streams_checked_src = 0,
		.existing_pres_dly_us_snk = UINT32_MAX,
		.existing_pres_dly_us_src = UINT32_MAX,
	};

	ret = bt_cap_unicast_group_foreach_stream(unicast_group, streams_calc_pres_dly,
						  (void *)&foreach_data);
	if (ret) {
		LOG_ERR("Failed to iterate streams in group: %d", ret);
		return ret;
	}

	if (foreach_data.ret) {
		LOG_ERR("Failed to compute presentation delay");
		return foreach_data.ret;
	}

	/* All steams have been iterated over. Parse results */
	if (foreach_data.streams_checked_snk == 0) {
		/* No sink streams checked, we don't have a presentation delay computed */
		*pres_dly_snk_us = UINT32_MAX;
	} else {
		ret = parse_common_qos_for_pres_dly(&common_qos_snk, pres_dly_snk_us);
		if (ret) {
			LOG_ERR("Failed to parse common sink QoS for pres delay: %d", ret);
			return ret;
		}
		LOG_INF("Source PD: (%d streams checked)", foreach_data.streams_checked_snk);
		pd_print(common_qos_src, foreach_data.existing_pres_dly_us_src, *pres_dly_src_us);
	}

	if (foreach_data.streams_checked_src == 0) {
		/* No source streams checked, we don't have a presentation delay computed */
		*pres_dly_src_us = UINT32_MAX;
	} else {
		ret = parse_common_qos_for_pres_dly(&common_qos_src, pres_dly_src_us);
		if (ret) {
			LOG_ERR("Failed to parse common source QoS for pres delay: %d", ret);
			return ret;
		}
		LOG_INF("Sink PD: (%d streams checked)", foreach_data.streams_checked_src);
		pd_print(common_qos_src, foreach_data.existing_pres_dly_us_src, *pres_dly_src_us);
	}

	return 0;
}

struct new_pres_delays {
	enum group_action_req action;
	uint32_t snk_us;
	uint32_t src_us;
};

/* Set the new action required. Only set if the new action is more severe */
static void group_action_set(enum group_action_req *action, enum group_action_req new_action)
{
	if (*action < new_action) {
		*action = new_action;
	}
}

static void stream_state_check(struct bt_cap_stream *stream,
			       struct new_pres_delays *new_pres_delays)
{
	if (le_audio_ep_state_check(stream->bap_stream.ep, BT_BAP_EP_STATE_QOS_CONFIGURED)) {
		/* Stream needs to be QoS configured again to update the presentation delay */
		group_action_set(&new_pres_delays->action, GROUP_ACTION_REQ_QOS_RECONFIG);
	} else if (le_audio_ep_state_check(stream->bap_stream.ep, BT_BAP_EP_STATE_ENABLING) ||
		   le_audio_ep_state_check(stream->bap_stream.ep, BT_BAP_EP_STATE_STREAMING)) {
		/* Group must be restarted to update the presentation delay */
		group_action_set(&new_pres_delays->action, GROUP_ACTION_REQ_RESTART);
	} else {
		group_action_set(&new_pres_delays->action, GROUP_ACTION_REQ_NONE);
	}
}

static bool new_pres_dly_us_set(struct bt_cap_stream *stream, void *user_data)
{
	struct new_pres_delays *new_pres_delays = (struct new_pres_delays *)user_data;

	if (stream->bap_stream.ep == NULL) {
		LOG_ERR("Stream has no endpoint assigned yet");
		return false;
	}

	if (new_pres_delays->snk_us != UINT32_MAX &&
	    stream->bap_stream.ep->dir == BT_AUDIO_DIR_SINK) {
		if (stream->bap_stream.qos->pd != new_pres_delays->snk_us) {
			stream->bap_stream.qos->pd = new_pres_delays->snk_us;
			stream_state_check(stream, new_pres_delays);
		}

	} else if (new_pres_delays->src_us != UINT32_MAX &&
		   stream->bap_stream.ep->dir == BT_AUDIO_DIR_SOURCE) {
		if (stream->bap_stream.qos->pd != new_pres_delays->src_us) {
			stream->bap_stream.qos->pd = new_pres_delays->src_us;
			stream_state_check(stream, new_pres_delays);
		}
	} else {
		/* No update needed for this stream */
		return false;
	}
	return false;
}

// This needs to be called only once, after all streams have been through the configured cb
int srv_store_pres_delay_set(struct bt_cap_unicast_group *unicast_group, uint32_t pres_dly_snk_us,
			     uint32_t pres_dly_src_us, enum group_action_req *group_action_required)
{

	int ret;
	struct new_pres_delays new_pres_delays = {
		.action = GROUP_ACTION_REQ_NONE,
		.snk_us = pres_dly_snk_us,
		.src_us = pres_dly_src_us,
	};

	ret = bt_cap_unicast_group_foreach_stream(unicast_group, new_pres_dly_us_set,
						  &new_pres_delays);

	*group_action_required = new_pres_delays.action;
	return ret;
}

// This needs to be called only once, after all streams have been through the configured cb
int srv_store_max_transp_lat_get(struct bt_cap_unicast_group *unicast_group,
				 uint16_t *new_max_trans_lat_snk_ms,
				 uint16_t *new_max_trans_lat_src_ms)
{
	return 0;
}

int srv_store_max_transp_lat_set(struct bt_cap_unicast_group *unicast_group,
				 uint16_t new_max_trans_lat_snk_ms,
				 uint16_t new_max_trans_lat_src_ms, bool *group_reconfig_needed)
{
	return 0;
}