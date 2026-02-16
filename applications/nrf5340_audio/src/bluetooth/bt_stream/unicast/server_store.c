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

#include "server_store.h"
#include "macros_common.h"
#include "le_audio.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(srvstore, CONFIG_SERVER_STORE_LOG_LEVEL);

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
		/* Server already exists, no need to add again, but we update the conn pointer*/
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
