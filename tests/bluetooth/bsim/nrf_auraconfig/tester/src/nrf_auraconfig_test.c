/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "nac_test.h"

#include <ctype.h>
#include <stdlib.h>
#include <strings.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/pacs.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/audio/pbp.h>
#include <zephyr/shell/shell.h>
#include <bstests.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, CONFIG_MAIN_LOG_LEVEL);

#define SEM_TIMEOUT K_SECONDS(60)

#define COLOR_GREEN "\x1B[0;32m"
#define COLOR_RED   "\x1B[0;31m"
#define COLOR_RESET "\x1b[0m"

#define INVALID_BROADCAST_ID		  (BT_AUDIO_BROADCAST_ID_MAX + 1)
#define PA_SYNC_INTERVAL_TO_TIMEOUT_RATIO 20 /* Set the timeout relative to interval */
#define PA_SYNC_SKIP			  5
#define PROGRAM_INFO_MAX_LEN		  255

static K_SEM_DEFINE(sem_broadcast_sink_stopped, 0U, 1U);
static K_SEM_DEFINE(sem_connected, 0U, 1U);
static K_SEM_DEFINE(sem_disconnected, 0U, 1U);
static K_SEM_DEFINE(sem_broadcaster_found, 0U, 1U);
static K_SEM_DEFINE(sem_pa_synced, 0U, 1U);
static K_SEM_DEFINE(sem_base_received, 0U, 1U);
static K_SEM_DEFINE(sem_syncable, 0U, 1U);
static K_SEM_DEFINE(sem_pa_sync_lost, 0U, 1U);
static K_SEM_DEFINE(sem_broadcast_code_received, 0U, 1U);
static K_SEM_DEFINE(sem_pa_request, 0U, 1U);
static K_SEM_DEFINE(sem_past_request, 0U, 1U);
static K_SEM_DEFINE(sem_bis_sync_requested, 0U, 1U);
static K_SEM_DEFINE(sem_stream_connected, 0U, CONFIG_BT_BAP_BROADCAST_SNK_STREAM_COUNT);
static K_SEM_DEFINE(sem_stream_started, 0U, CONFIG_BT_BAP_BROADCAST_SNK_STREAM_COUNT);
static K_SEM_DEFINE(sem_big_synced, 0U, 1U);

/* Sample assumes that we only have a single Scan Delegator receive state */
static struct bt_bap_broadcast_sink *broadcast_sink;
static struct bt_le_scan_recv_info broadcaster_info;
static bt_addr_le_t broadcaster_addr;
static struct bt_le_per_adv_sync *pa_sync;
static uint32_t broadcaster_broadcast_id;
static struct broadcast_sink_stream {
	struct bt_bap_stream stream;
} streams[CONFIG_BT_BAP_BROADCAST_SNK_STREAM_COUNT];

static struct bt_bap_stream *streams_p[ARRAY_SIZE(streams)];
static volatile bool big_synced;
static volatile bool base_received;
static volatile uint8_t stream_count;

extern enum bst_result_t bst_result;

static struct nac_test_values_subgroup
	subgroups_expected[CONFIG_BT_BAP_BROADCAST_SNK_SUBGROUP_COUNT];
static struct nac_test_values_subgroup subgroups_dut[CONFIG_BT_BAP_BROADCAST_SNK_SUBGROUP_COUNT];

static struct nac_test_values_big expected = {
	.subgroups = subgroups_expected,
};
static struct nac_test_values_big dut = {
	.subgroups = subgroups_dut,
};

static const struct bt_audio_codec_cap codec_cap = BT_AUDIO_CODEC_CAP_LC3(
	BT_AUDIO_CODEC_CAP_FREQ_16KHZ | BT_AUDIO_CODEC_CAP_FREQ_24KHZ |
		BT_AUDIO_CODEC_CAP_FREQ_48KHZ,
	BT_AUDIO_CODEC_CAP_DURATION_7_5 | BT_AUDIO_CODEC_CAP_DURATION_10,
	BT_AUDIO_CODEC_CAP_CHAN_COUNT_SUPPORT(1), 40u, 120u, 1u, BT_AUDIO_CONTEXT_TYPE_ANY);

/* Create a mask for the maximum BIS we can sync to using the number of streams
 * we have. We add an additional 1 since the bis indexes start from 1 and not
 * 0.
 */
static const uint32_t bis_index_mask = BIT_MASK(ARRAY_SIZE(streams) + 1U);
static uint32_t requested_bis_sync;
static uint32_t bis_index_bitfield;
static uint8_t sink_broadcast_code[BT_ISO_BROADCAST_CODE_SIZE];

static char broadcast_name_target[ADV_NAME_MAX + 1];

/* Find the most significant 1 in bits, bits will not be 0. */
static int8_t context_msb_one(uint16_t bits)
{
	uint8_t pos = 0;

	while (bits >>= 1) {
		pos++;
	}

	return pos;
}

static void stream_connected_cb(struct bt_bap_stream *stream)
{
	LOG_DBG("Stream connected");

	k_sem_give(&sem_stream_connected);
}

static void stream_disconnected_cb(struct bt_bap_stream *stream, uint8_t reason)
{
	int err;

	LOG_INF("Stream disconnected with reason 0x%02X", reason);

	err = k_sem_take(&sem_stream_connected, K_NO_WAIT);
	if (err != 0) {
		LOG_DBG("Failed to take sem_stream_connected: %d", err);
	}
}

static void stream_started_cb(struct bt_bap_stream *stream)
{
	LOG_DBG("Stream started");

	k_sem_give(&sem_stream_started);
	if (k_sem_count_get(&sem_stream_started) == stream_count) {
		big_synced = true;
		k_sem_give(&sem_big_synced);
	}
}

static void stream_stopped_cb(struct bt_bap_stream *stream, uint8_t reason)
{
	int err;

	LOG_DBG("Stream stopped with reason 0x%02X", reason);

	err = k_sem_take(&sem_stream_started, K_NO_WAIT);
	if (err != 0) {
		LOG_DBG("Failed to take sem_stream_started: %d", err);
	}

	if (k_sem_count_get(&sem_stream_started) != stream_count) {
		big_synced = false;
	}
}

static void stream_recv_cb(struct bt_bap_stream *stream, const struct bt_iso_recv_info *info,
			   struct net_buf *buf)
{
	LOG_DBG("Stream received %d bytes", buf->len);
}

static struct bt_bap_stream_ops stream_ops = {
	.connected = stream_connected_cb,
	.disconnected = stream_disconnected_cb,
	.started = stream_started_cb,
	.stopped = stream_stopped_cb,
	.recv = stream_recv_cb,
};

struct stream_info {
	struct bt_bap_base_codec_id codec_id;
	uint8_t sub_index;
	uint8_t bis_index;
};

static bool print_base_subgroup_bis_cb(const struct bt_bap_base_subgroup_bis *bis, void *user_data)
{
	int ret;
	struct stream_info *info = user_data;
	enum bt_audio_location chan_allocation;

	struct bt_audio_codec_cfg codec_cfg = {
		.id = info->codec_id.id,
		.cid = info->codec_id.cid,
		.vid = info->codec_id.vid,
	};

	ret = bt_bap_base_subgroup_bis_codec_to_codec_cfg(bis, &codec_cfg);
	if (ret < 0) {
		LOG_WRN("Failed to convert codec to codec_cfg: %d", ret);
		return false;
	}

	ret = bt_audio_codec_cfg_get_chan_allocation(&codec_cfg, &chan_allocation, false);
	if (ret < 0) {
		LOG_WRN("Failed to get channel allocation: %d", ret);
		return false;
	}

	dut.subgroups[info->sub_index].locations[info->bis_index] = chan_allocation;

	if (chan_allocation == BT_AUDIO_LOCATION_MONO_AUDIO) {
		LOG_DBG("\t\t\tBIS %d: Mono", info->bis_index);
	} else {
		for (size_t i = 0; i < 32; i++) {
			const uint8_t bit_val = BIT(i);

			if (chan_allocation & bit_val) {
				LOG_DBG("\t\t\tBIS %d: %s", info->bis_index,
					bt_audio_location_bit_to_str(bit_val));
			}
		}
	}

	info->bis_index += 1;

	return true;
}

static bool print_base_subgroup_cb(const struct bt_bap_base_subgroup *subgroup, void *user_data)
{
	int ret;
	struct bt_bap_base_codec_id codec_id;
	struct bt_audio_codec_cfg codec_cfg;
	uint8_t *sub_index = user_data;

	LOG_DBG("Subgroup: %d", *sub_index);

	ret = bt_bap_base_get_subgroup_codec_id(subgroup, &codec_id);
	if (ret < 0) {
		return false;
	}

	struct stream_info info = {
		.codec_id = codec_id,
		.sub_index = *sub_index,
		.bis_index = 0,
	};

	ret = bt_bap_base_subgroup_codec_to_codec_cfg(subgroup, &codec_cfg);
	if (ret != 0) {
		LOG_WRN("Failed to convert codec to codec_cfg: %d", ret);
		return false;
	}

	LOG_DBG("\tCodec specific configuration:");

	if (codec_cfg.data_len == 0U) {
		LOG_DBG("\t\tNone");
	} else if (codec_cfg.id == BT_HCI_CODING_FORMAT_LC3) {
		int ret;

		ret = bt_audio_codec_cfg_get_freq(&codec_cfg);
		if (ret >= 0) {
			dut.subgroups[info.sub_index].sampling_rate_hz =
				bt_audio_codec_cfg_freq_to_freq_hz(ret);
			LOG_DBG("\t\tSampling rate: %u Hz",
				dut.subgroups[info.sub_index].sampling_rate_hz);
		}

		ret = bt_audio_codec_cfg_get_frame_dur(&codec_cfg);
		if (ret >= 0) {
			dut.subgroups[info.sub_index].frame_duration_us =
				bt_audio_codec_cfg_frame_dur_to_frame_dur_us(ret);
			LOG_DBG("\t\tFrame duration: %u us",
				dut.subgroups[info.sub_index].frame_duration_us);
		}

		ret = bt_audio_codec_cfg_get_octets_per_frame(&codec_cfg);
		if (ret >= 0) {
			dut.subgroups[info.sub_index].octets_per_frame = (uint16_t)ret;
			LOG_DBG("\t\tOctets per frame: %u",
				dut.subgroups[info.sub_index].octets_per_frame);
		}

		ret = bt_bap_base_get_subgroup_bis_count(subgroup);
		if (ret >= 0) {
			dut.subgroups[info.sub_index].num_bis = (uint8_t)ret;
			LOG_DBG("\t\tNumber of BISes: %u", dut.subgroups[info.sub_index].num_bis);
		}

		LOG_DBG("\t\tLocation:");
		ret = bt_bap_base_subgroup_foreach_bis(subgroup, print_base_subgroup_bis_cb, &info);
		if (ret < 0) {
			return false;
		}

	} else {
		LOG_DBG("\t\tUnsupported codec");
	}

	LOG_DBG("\tCodec specific metadata:");

	if (codec_cfg.meta_len == 0U) {
		LOG_DBG("\tNone");
	} else {
		const uint8_t *data;

		ret = bt_audio_codec_cfg_meta_get_stream_context(&codec_cfg);
		if (ret >= 0) {
			dut.subgroups[info.sub_index].context = ret;
			/* Iterate through the context bits */
			for (size_t i = 0U; i < 16; i++) {
				const uint16_t bit_val = BIT(i);

				if (ret & bit_val) {
					LOG_DBG("\t\tContext: %s",
						bt_audio_context_bit_to_str(bit_val));
				}
			}
		}

		ret = bt_audio_codec_cfg_meta_get_program_info(&codec_cfg, &data);
		if (ret >= 0) {
			char program_info[PROGRAM_INFO_MAX_LEN] = {'\0'};
			int meta_len = ret;

			if (meta_len > PROGRAM_INFO_MAX_LEN) {
				meta_len = PROGRAM_INFO_MAX_LEN - 1;
			}

			memcpy(program_info, data, meta_len);
			LOG_DBG("\t\tProgram info: %s", program_info);
			memcpy(dut.subgroups[info.sub_index].program_info, program_info,
			       sizeof(program_info));
		}

		const uint8_t *lang;

		ret = bt_audio_codec_cfg_meta_get_lang(&codec_cfg, &lang);
		if (ret == 0) {
			LOG_DBG("\t\tLanguage: %c%c%c", (char)lang[0], (char)lang[1],
				(char)lang[2]);
			memcpy(dut.subgroups[info.sub_index].language, lang, 3);
		}

		ret = bt_audio_codec_cfg_meta_get_parental_rating(&codec_cfg);
		if (ret >= 0) {
			LOG_DBG("\t\tParental rating: %s", bt_audio_parental_rating_to_str(ret));
		}

		ret = bt_audio_codec_cfg_meta_get_audio_active_state(&codec_cfg);
		if (ret >= 0) {
			LOG_DBG("\t\tAudio active state: %s",
				ret == BT_AUDIO_ACTIVE_STATE_ENABLED ? "enabled" : "disabled");
		}

		ret = bt_audio_codec_cfg_meta_get_bcast_audio_immediate_rend_flag(&codec_cfg);
		if (ret >= 0) {
			LOG_DBG("\t\tImmediate rendering flag: %s",
				ret == 0 ? "enabled" : "disabled");
			dut.subgroups[info.sub_index].immediate_rend_flag = ret == 0 ? true : false;
		}
	}

	*sub_index += 1;

	return true;
}

static void base_print(const struct bt_bap_base *base)
{
	int err;

	dut.pd_us = bt_bap_base_get_pres_delay(base);
	LOG_DBG("Presentation delay: %d us", dut.pd_us);

	dut.num_subgroups = bt_bap_base_get_subgroup_count(base);

	uint8_t sub_index = 0;

	err = bt_bap_base_foreach_subgroup(base, print_base_subgroup_cb, &sub_index);
	if (err < 0) {
		LOG_DBG("Invalid BASE: %d", err);
	}
}

static void base_recv_cb(struct bt_bap_broadcast_sink *sink, const struct bt_bap_base *base,
			 size_t base_size)
{
	int err;
	uint32_t base_bis_index_bitfield = 0U;

	if (base_received) {
		return;
	}

	LOG_DBG("Received BASE with %d subgroups from broadcast sink",
		bt_bap_base_get_subgroup_count(base));

	err = bt_bap_base_get_bis_indexes(base, &base_bis_index_bitfield);
	if (err != 0) {
		LOG_DBG("Failed to BIS indexes: %d", err);
		return;
	}

	bis_index_bitfield = base_bis_index_bitfield & bis_index_mask;

	LOG_DBG("bis_index_bitfield = 0x%08x", bis_index_bitfield);

	requested_bis_sync = BT_BAP_BIS_SYNC_NO_PREF;
	k_sem_give(&sem_bis_sync_requested);

	base_received = true;
	base_print(base);
	k_sem_give(&sem_base_received);
}

static void syncable_cb(struct bt_bap_broadcast_sink *sink, const struct bt_iso_biginfo *biginfo)
{
	LOG_DBG("Broadcast sink is syncable, BIG %s",
		biginfo->encryption ? "encrypted" : "not encrypted");

	k_sem_give(&sem_syncable);

	/* Give semaphore if no encryption or broadcast code is received */
	if (!biginfo->encryption || sink_broadcast_code[0] != 0) {
		/* Use the semaphore as a boolean */
		k_sem_reset(&sem_broadcast_code_received);
		k_sem_give(&sem_broadcast_code_received);
	}
}

static struct bt_bap_broadcast_sink_cb broadcast_sink_cbs = {
	.base_recv = base_recv_cb,
	.syncable = syncable_cb,
};

static struct bt_pacs_cap cap = {
	.codec_cap = &codec_cap,
};

static bool scan_check_and_sync_broadcast(struct bt_data *data, void *user_data)
{
	enum bt_pbp_announcement_feature source_features;
	const struct bt_le_scan_recv_info *info = user_data;
	struct bt_uuid_16 adv_uuid;
	uint8_t *tmp_meta = NULL;
	int ret;

	if (data->type != BT_DATA_SVC_DATA16) {
		return true;
	}

	if (!bt_uuid_create(&adv_uuid.uuid, data->data, BT_UUID_SIZE_16)) {
		return true;
	}

	if (!bt_uuid_cmp(&adv_uuid.uuid, BT_UUID_BROADCAST_AUDIO)) {
		broadcaster_broadcast_id = sys_get_le24(data->data + BT_UUID_SIZE_16);
		return true;
	}

	ret = bt_pbp_parse_announcement(data, &source_features, &tmp_meta);
	if (ret >= 0) {
		if (source_features & BT_PBP_ANNOUNCEMENT_FEATURE_ENCRYPTION) {
			dut.encryption = true;
		}

		if (source_features & BT_PBP_ANNOUNCEMENT_FEATURE_STANDARD_QUALITY) {
			dut.std_quality = true;
		}

		if (source_features & BT_PBP_ANNOUNCEMENT_FEATURE_HIGH_QUALITY) {
			dut.high_quality = true;
		}

		/* parse metadata */
		if (tmp_meta == NULL) {
			LOG_DBG("\tNo metadata");
			return true;
		}

		/* Store info for PA sync parameters */
		memcpy(&broadcaster_info, info, sizeof(broadcaster_info));
		bt_addr_le_copy(&broadcaster_addr, info->addr);
		dut.broadcast_id = broadcaster_broadcast_id;
		dut.phy = info->secondary_phy;
		k_sem_give(&sem_broadcaster_found);

		return false;
	}

	return true;
}

static bool is_substring(const char *substr, const char *str)
{
	const size_t str_len = strlen(str);
	const size_t sub_str_len = strlen(substr);

	if (sub_str_len > str_len) {
		return false;
	}

	for (size_t pos = 0; pos < str_len; pos++) {
		if (pos + sub_str_len > str_len) {
			return false;
		}

		if (strncasecmp(substr, &str[pos], sub_str_len) == 0) {
			return true;
		}
	}

	return false;
}

static bool names_store(struct bt_data *data, void *user_data)
{
	switch (data->type) {
	case BT_DATA_NAME_SHORTENED:
	case BT_DATA_NAME_COMPLETE:
		if (data->data_len <= ADV_NAME_MAX && data->type == BT_DATA_NAME_COMPLETE) {
			memset(dut.adv_name, '\0', sizeof(dut.adv_name));
			memcpy(dut.adv_name, data->data, data->data_len);
		} else {
			LOG_DBG("Adv name too long");
		}

	case BT_DATA_BROADCAST_NAME:
		if (data->data_len <= ADV_NAME_MAX && data->type == BT_DATA_BROADCAST_NAME) {
			memset(dut.broadcast_name, '\0', sizeof(dut.broadcast_name));
			memcpy(dut.broadcast_name, data->data, data->data_len);
		} else {
			LOG_DBG("Broadcast name too long");
		}

		return true;
	default:
		return true;
	}
}

static bool data_cb(struct bt_data *data, void *user_data)
{
	bool *device_found = user_data;
	char name[ADV_NAME_MAX] = {0};

	switch (data->type) {
	case BT_DATA_NAME_SHORTENED:
	case BT_DATA_NAME_COMPLETE:
		if (data->data_len <= ADV_NAME_MAX) {
			memset(dut.adv_name, '\0', sizeof(dut.adv_name));
			memcpy(dut.adv_name, data->data, data->data_len);
		} else {
			LOG_DBG("Adv name too long");
		}

	case BT_DATA_BROADCAST_NAME:
		memcpy(name, data->data, MIN(data->data_len, ADV_NAME_MAX - 1));

		if ((is_substring(broadcast_name_target, name))) {
			*device_found = true;
			return false;
		}
		return true;
	default:
		return true;
	}
}

static void broadcast_scan_recv(const struct bt_le_scan_recv_info *info, struct net_buf_simple *ad)
{
	if (info->interval == 0U) {
		return;
	}
	/* call to bt_data_parse consumes netbufs so shallow clone for verbose
	 * output
	 */

	if (strlen(broadcast_name_target) > 0U) {
		bool device_found = false;
		struct net_buf_simple buf_copy_0;
		struct net_buf_simple buf_copy_1;

		net_buf_simple_clone(ad, &buf_copy_0);
		net_buf_simple_clone(ad, &buf_copy_1);
		bt_data_parse(&buf_copy_0, data_cb, &device_found);

		if (!device_found) {
			return;
		}

		bt_data_parse(&buf_copy_1, names_store, NULL);
	}

	bt_data_parse(ad, scan_check_and_sync_broadcast, (void *)info);
}

static struct bt_le_scan_cb bap_scan_cb = {
	.recv = broadcast_scan_recv,
};

static void bap_pa_sync_synced_cb(struct bt_le_per_adv_sync *sync,
				  struct bt_le_per_adv_sync_synced_info *info)
{
	if (sync != pa_sync) {
		return;
	}

	LOG_DBG("PA sync synced for broadcast sink with broadcast ID 0x%06X",
		broadcaster_broadcast_id);

	if (pa_sync == NULL) {
		pa_sync = sync;
	}

	k_sem_give(&sem_pa_synced);
}

static void bap_pa_sync_terminated_cb(struct bt_le_per_adv_sync *sync,
				      const struct bt_le_per_adv_sync_term_info *info)
{
	if (sync == pa_sync) {
		return;
	}

	LOG_DBG("PA sync lost with reason %u", info->reason);
	pa_sync = NULL;

	k_sem_give(&sem_pa_sync_lost);
}

static struct bt_le_per_adv_sync_cb bap_pa_sync_cb = {
	.synced = bap_pa_sync_synced_cb,
	.term = bap_pa_sync_terminated_cb,
};

static int init(void)
{
	int err;

	memset(broadcast_name_target, '\0', sizeof(broadcast_name_target));

	err = bt_enable(NULL);
	if (err) {
		LOG_DBG("Bluetooth enable failed (err %d)", err);
		return err;
	}

	LOG_DBG("Bluetooth initialized");

	const struct bt_pacs_register_param pacs_param = {
		.snk_pac = true,
		.snk_loc = true,
	};

	err = bt_pacs_register(&pacs_param);
	if (err) {
		LOG_ERR("Could not register PACS (err %d)\n", err);
		return err;
	}

	err = bt_pacs_cap_register(BT_AUDIO_DIR_SINK, &cap);
	if (err) {
		LOG_DBG("Capability register failed (err %d)", err);
		return err;
	}

	bt_bap_broadcast_sink_register_cb(&broadcast_sink_cbs);
	bt_le_per_adv_sync_cb_register(&bap_pa_sync_cb);
	bt_le_scan_cb_register(&bap_scan_cb);

	for (size_t i = 0U; i < ARRAY_SIZE(streams); i++) {
		streams[i].stream.ops = &stream_ops;
	}

	return 0;
}

static int reset(void)
{
	int err;

	LOG_DBG("Reset");

	if (broadcast_sink != NULL) {
		err = bt_bap_broadcast_sink_delete(broadcast_sink);
		if (err) {
			LOG_DBG("Deleting broadcast sink failed (err %d)", err);

			return err;
		}

		broadcast_sink = NULL;
	}

	if (pa_sync != NULL) {
		bt_le_per_adv_sync_delete(pa_sync);
		if (err) {
			LOG_DBG("Deleting PA sync failed (err %d)", err);

			return err;
		}

		pa_sync = NULL;
	}

	memset(broadcast_name_target, '\0', sizeof(broadcast_name_target));
	bis_index_bitfield = 0U;
	requested_bis_sync = 0U;
	big_synced = false;
	base_received = false;
	stream_count = 0U;
	(void)memset(sink_broadcast_code, 0, sizeof(sink_broadcast_code));
	(void)memset(&broadcaster_info, 0, sizeof(broadcaster_info));
	(void)memset(&broadcaster_addr, 0, sizeof(broadcaster_addr));
	broadcaster_broadcast_id = INVALID_BROADCAST_ID;

	k_sem_reset(&sem_broadcaster_found);
	k_sem_reset(&sem_pa_synced);
	k_sem_reset(&sem_base_received);
	k_sem_reset(&sem_syncable);
	k_sem_reset(&sem_pa_sync_lost);
	k_sem_reset(&sem_broadcast_code_received);
	k_sem_reset(&sem_bis_sync_requested);
	k_sem_reset(&sem_stream_connected);
	k_sem_reset(&sem_stream_started);
	k_sem_reset(&sem_broadcast_sink_stopped);

	return 0;
}

static uint16_t interval_to_sync_timeout(uint16_t interval)
{
	uint32_t interval_ms;
	uint32_t timeout;

	/* Add retries and convert to unit in 10's of ms */
	interval_ms = BT_GAP_PER_ADV_INTERVAL_TO_MS(interval);
	timeout = (interval_ms * PA_SYNC_INTERVAL_TO_TIMEOUT_RATIO) / 10;

	/* Enforce restraints */
	timeout = CLAMP(timeout, BT_GAP_PER_ADV_MIN_TIMEOUT, BT_GAP_PER_ADV_MAX_TIMEOUT);

	return (uint16_t)timeout;
}

static int pa_sync_create(void)
{
	int err;
	struct bt_le_local_features feature;
	struct bt_le_per_adv_sync_param create_params = {0};

	bt_addr_le_copy(&create_params.addr, &broadcaster_addr);

	err = bt_le_get_local_features(&feature);
	if (err < 0) {
		LOG_WRN("Failed to get local le features (err %d)", err);
		return err;
	}

	if (BT_FEAT_LE_PER_ADV_ADI_SUPP(feature.features)) {
		create_params.options = BT_LE_PER_ADV_SYNC_OPT_FILTER_DUPLICATE;
	} else {
		create_params.options = BT_LE_PER_ADV_SYNC_OPT_NONE;
		LOG_WRN("Not supported: LE Per Adv ADI");
	}

	create_params.sid = broadcaster_info.sid;
	create_params.skip = PA_SYNC_SKIP;
	create_params.timeout = interval_to_sync_timeout(broadcaster_info.interval);

	return bt_le_per_adv_sync_create(&create_params, &pa_sync);
}

static void values_compare(void)
{
	bool passing = true;

	LOG_DBG("Results: ");
	if (strcmp(expected.adv_name, dut.adv_name) != 0) {
		LOG_WRN("\tExpected adv_name: %s, received: %s", expected.adv_name, dut.adv_name);
		passing = false;
	} else {
		LOG_DBG("\tAdv_name: " COLOR_GREEN "Success" COLOR_RESET);
	}

	if (strcmp(expected.broadcast_name, dut.broadcast_name) != 0) {
		LOG_WRN("\tExpected broadcast_name: %s, received: %s", expected.broadcast_name,
			dut.broadcast_name);
		passing = false;
	} else {
		LOG_DBG("\tBroadcast_name: " COLOR_GREEN "Success" COLOR_RESET);
	}

	if (expected.broadcast_id != dut.broadcast_id && expected.broadcast_id != 0x00) {
		LOG_WRN("\tExpected broadcast_id: 0x%06X, received: 0x%06X", expected.broadcast_id,
			dut.broadcast_id);
		passing = false;
	} else {
		LOG_DBG("\tBroadcast_id: " COLOR_GREEN "Success" COLOR_RESET);
	}

	if (expected.phy != dut.phy) {
		LOG_WRN("\tExpected phy: %d, received: %d", expected.phy, dut.phy);
		passing = false;
	} else {
		LOG_DBG("\tPhy: " COLOR_GREEN "Success" COLOR_RESET);
	}

	if (expected.encryption != dut.encryption) {
		LOG_WRN("\tExpected encryption: %d, received: %d", expected.encryption,
			dut.encryption);
		passing = false;
	} else {
		LOG_DBG("\tEncryption: " COLOR_GREEN "Success" COLOR_RESET);
	}

	if (expected.num_subgroups != dut.num_subgroups) {
		LOG_WRN("\tExpected num_subgroups: %d, received: %d", expected.num_subgroups,
			dut.num_subgroups);
		passing = false;
	} else {
		LOG_DBG("\tNum_subgroups: " COLOR_GREEN "Success" COLOR_RESET);
	}

	for (size_t i = 0U; i < dut.num_subgroups; i++) {
		if (expected.subgroups[i].sampling_rate_hz != dut.subgroups[i].sampling_rate_hz) {
			LOG_WRN("\tExpected sampling rate: %u Hz, received: %u Hz",
				expected.subgroups[i].sampling_rate_hz,
				dut.subgroups[i].sampling_rate_hz);
			passing = false;
		} else {
			LOG_DBG("\tSampling rate: " COLOR_GREEN "Success" COLOR_RESET);
		}

		if (expected.subgroups[i].frame_duration_us != dut.subgroups[i].frame_duration_us) {
			LOG_WRN("\tExpected frame duration: %u us, received: %u us",
				expected.subgroups[i].frame_duration_us,
				dut.subgroups[i].frame_duration_us);
			passing = false;
		} else {
			LOG_DBG("\tFrame duration: " COLOR_GREEN "Success" COLOR_RESET);
		}

		if (expected.subgroups[i].octets_per_frame != dut.subgroups[i].octets_per_frame) {
			LOG_WRN("\tExpected octets per frame: %u, received: %u",
				expected.subgroups[i].octets_per_frame,
				dut.subgroups[i].octets_per_frame);
			passing = false;
		} else {
			LOG_DBG("\tOctets per frame: " COLOR_GREEN "Success" COLOR_RESET);
		}

		if (strcmp(expected.subgroups[i].language, dut.subgroups[i].language) != 0) {
			LOG_WRN("\tExpected language: %s, received: %s",
				expected.subgroups[i].language, dut.subgroups[i].language);
			passing = false;
		} else {
			LOG_DBG("\tLanguage: " COLOR_GREEN "Success" COLOR_RESET);
		}

		if (expected.subgroups[i].immediate_rend_flag !=
		    dut.subgroups[i].immediate_rend_flag) {
			LOG_WRN("\tExpected immediate_rend_flag: %d, received: %d",
				expected.subgroups[i].immediate_rend_flag,
				dut.subgroups[i].immediate_rend_flag);
			passing = false;
		} else {
			LOG_DBG("\tImmediate_rend_flag: " COLOR_GREEN "Success" COLOR_RESET);
		}

		if (expected.subgroups[i].context != dut.subgroups[i].context) {
			LOG_WRN("\tExpected context: 0x%04X, received: 0x%04X",
				expected.subgroups[i].context, dut.subgroups[i].context);
			passing = false;
		} else {
			LOG_DBG("\tContext: " COLOR_GREEN "Success" COLOR_RESET);
		}

		if (expected.subgroups[i].immediate_rend_flag !=
		    dut.subgroups[i].immediate_rend_flag) {
			LOG_WRN("\tExpected immediate_rend_flag: %d, received: %d",
				expected.subgroups[i].immediate_rend_flag,
				dut.subgroups[i].immediate_rend_flag);
			passing = false;
		} else {
			LOG_DBG("\tImmediate_rend_flag: " COLOR_GREEN "Success" COLOR_RESET);
		}

		if (strcmp(expected.subgroups[i].program_info, dut.subgroups[i].program_info) !=
		    0) {
			LOG_WRN("\tExpected program info: %s, received: %s",
				expected.subgroups[i].program_info, dut.subgroups[i].program_info);
			passing = false;
		} else {
			LOG_DBG("\tProgram info: " COLOR_GREEN "Success" COLOR_RESET);
		}

		if (expected.subgroups[i].num_bis != dut.subgroups[i].num_bis) {
			LOG_WRN("\tExpected num_bis: %d, received: %d",
				expected.subgroups[i].num_bis, dut.subgroups[i].num_bis);
			passing = false;
		} else {
			LOG_DBG("\tNum_bis: " COLOR_GREEN "Success" COLOR_RESET);
		}

		for (size_t j = 0U; j < dut.subgroups[i].num_bis; j++) {
			if (expected.subgroups[i].locations[j] != dut.subgroups[i].locations[j]) {
				LOG_WRN("\tExpected location: %d, received: %d",
					expected.subgroups[i].locations[j],
					dut.subgroups[i].locations[j]);
				passing = false;
			} else {
				LOG_DBG("\tLocation: " COLOR_GREEN "Success" COLOR_RESET);
			}
		}
	}

	if (passing == true) {
		bst_result = Passed;
		LOG_INF("Verdict: " COLOR_GREEN "SUCCESS" COLOR_RESET);
	} else {
		bst_result = Failed;
		LOG_WRN("Verdict: " COLOR_RED "FAIL" COLOR_RESET);
	}
}

void nac_test_loop(void)
{
	int err;
	uint32_t sync_bitfield;

	err = k_sem_take(&sem_broadcaster_found, K_FOREVER);
	if (err != 0) {
		LOG_WRN("sem_broadcaster_found timed out, resetting");
	}

	err = bt_le_scan_stop();
	if (err != 0) {
		LOG_WRN("bt_le_scan_stop failed with %d, resetting", err);
	}

	LOG_DBG("Attempting to PA sync to the broadcaster with id 0x%06X",
		broadcaster_broadcast_id);

	err = pa_sync_create();
	if (err != 0) {
		LOG_WRN("Could not create Broadcast PA sync: %d, resetting", err);
	}

	LOG_DBG("Waiting for PA synced");

	err = k_sem_take(&sem_pa_synced, SEM_TIMEOUT);
	if (err != 0) {
		LOG_DBG("sem_pa_synced timed out, resetting");
	}

	LOG_DBG("Broadcast source PA synced, creating Broadcast Sink");

	err = bt_bap_broadcast_sink_create(pa_sync, broadcaster_broadcast_id, &broadcast_sink);
	if (err != 0) {
		LOG_DBG("Failed to create broadcast sink: %d", err);
	}

	LOG_DBG("Broadcast Sink created, waiting for BASE");

	err = k_sem_take(&sem_base_received, SEM_TIMEOUT);
	if (err != 0) {
		LOG_DBG("sem_base_received timed out, resetting");
	}

	LOG_DBG("BASE received, waiting for syncable");

	err = k_sem_take(&sem_syncable, SEM_TIMEOUT);
	if (err != 0) {
		LOG_DBG("sem_syncable timed out, resetting");
	}

	/* sem_broadcast_code_received is also given if the
	 * broadcast is not encrypted
	 */
	LOG_DBG("Waiting for broadcast code");

	err = k_sem_take(&sem_broadcast_code_received, SEM_TIMEOUT);
	if (err != 0) {
		LOG_DBG("sem_broadcast_code_received timed out, resetting");
	}

	values_compare();

	LOG_DBG("Waiting for BIS sync request");

	err = k_sem_take(&sem_bis_sync_requested, SEM_TIMEOUT);
	if (err != 0) {
		LOG_DBG("sem_bis_sync_requested timed out, resetting");
	}

	sync_bitfield = bis_index_bitfield & requested_bis_sync;
	stream_count = 0;
	for (int i = 1; i < BT_ISO_MAX_GROUP_ISO_COUNT; i++) {
		if ((sync_bitfield & BIT(i)) != 0) {
			stream_count++;
		}
	}

	LOG_DBG("Syncing to broadcast with bitfield: 0x%08x = 0x%08x "
		"(bis_index) & "
		"0x%08x "
		"(req_bis_sync), stream_count = %u",
		sync_bitfield, bis_index_bitfield, requested_bis_sync, stream_count);

	err = bt_bap_broadcast_sink_sync(broadcast_sink, sync_bitfield, streams_p,
					 sink_broadcast_code);
	if (err != 0) {
		LOG_DBG("Unable to sync to broadcast source: %d", err);
	}

	LOG_DBG("Waiting for stream(s) started");

	err = k_sem_take(&sem_big_synced, SEM_TIMEOUT);
	if (err != 0) {
		LOG_DBG("sem_big_synced timed out, resetting");
	}

	LOG_DBG("Waiting for PA disconnected");
	k_sem_take(&sem_pa_sync_lost, K_FOREVER);

	LOG_DBG("Waiting for sink to stop");

	err = k_sem_take(&sem_broadcast_sink_stopped, SEM_TIMEOUT);
	if (err != 0) {
		LOG_DBG("sem_broadcast_sink_stopped timed out, resetting");
	}
}

int nac_test_main(void)
{
	int err;

	err = init();
	if (err) {
		LOG_DBG("Init failed (err %d)", err);
		return err;
	}

	for (size_t i = 0U; i < ARRAY_SIZE(streams_p); i++) {
		streams_p[i] = &streams[i].stream;
	}

	err = reset();
	if (err != 0) {
		LOG_DBG("Resetting failed: %d - Aborting", err);

		return err;
	}

	return 0;
}

static int cmd_big_input(const struct shell *shell, size_t argc, char **argv)
{
	if (argc < 9) {
		shell_error(shell, "<adv_name (string)> <broadcast_name (string)> <broadcast_id (6 "
				   "octets)> <phy (1 (M), 2 (M), or 4 (coded))> <pd_us (uint32_t)> "
				   "<encryption "
				   "(0/1)> <broadcast_code (16 chars)> <num_subgroups (uint8_t)> "
				   "<std_quality (0/1)> <high_quality (0/1)>");
		return -EINVAL;
	}

	memset(expected.adv_name, '\0', sizeof(expected.adv_name));
	memcpy(expected.adv_name, argv[1], strlen(argv[1]));

	memset(broadcast_name_target, '\0', sizeof(broadcast_name_target));
	memcpy(broadcast_name_target, argv[2], strlen(argv[2]));

	memset(expected.broadcast_name, '\0', sizeof(expected.broadcast_name));
	memcpy(expected.broadcast_name, argv[2], strlen(argv[2]));

	expected.broadcast_id = (uint32_t)strtol(argv[3], NULL, 16);
	expected.phy = strtol(argv[4], NULL, 10);
	expected.pd_us = strtol(argv[5], NULL, 10);
	expected.encryption = strtol(argv[6], NULL, 10);

	memset(expected.broadcast_code, '\0', sizeof(expected.broadcast_code));
	memcpy(expected.broadcast_code, argv[7], strlen(argv[7]));

	expected.num_subgroups = strtol(argv[8], NULL, 10);
	expected.std_quality = strtol(argv[9], NULL, 10);
	expected.high_quality = strtol(argv[10], NULL, 10);

	return 0;
}

static int cmd_adv_name(const struct shell *shell, size_t argc, char **argv)
{
	if (argc < 2) {
		shell_error(shell, "<adv_name (string)>");
		return -EINVAL;
	}

	memset(expected.adv_name, '\0', sizeof(expected.adv_name));
	memcpy(expected.adv_name, argv[1], strlen(argv[1]));

	return 0;
}

static int cmd_broadcast_name(const struct shell *shell, size_t argc, char **argv)
{
	if (argc < 2) {
		shell_error(shell, "<broadcast_name (string)>");
		return -EINVAL;
	}

	memset(broadcast_name_target, '\0', sizeof(broadcast_name_target));
	memcpy(broadcast_name_target, argv[1], strlen(argv[1]));

	memset(expected.broadcast_name, '\0', sizeof(expected.broadcast_name));
	memcpy(expected.broadcast_name, argv[1], strlen(argv[1]));

	return 0;
}

static int cmd_broadcast_id(const struct shell *shell, size_t argc, char **argv)
{
	if (argc < 2) {
		shell_error(shell, "<broadcast_id (6 octets)>");
		return -EINVAL;
	}

	expected.broadcast_id = (uint32_t)strtol(argv[1], NULL, 16);

	return 0;
}

static int cmd_phy(const struct shell *shell, size_t argc, char **argv)
{
	if (argc < 2) {
		shell_error(shell, "<phy (1, 2, or 4 (coded))>");
		return -EINVAL;
	}

	expected.phy = strtol(argv[1], NULL, 10);

	return 0;
}

static int cmd_pd_us(const struct shell *shell, size_t argc, char **argv)
{
	if (argc < 2) {
		shell_error(shell, "<pd_us (uint32_t)>");
		return -EINVAL;
	}

	expected.pd_us = strtol(argv[1], NULL, 10);

	return 0;
}

static int cmd_encryption(const struct shell *shell, size_t argc, char **argv)
{
	if (argc < 2) {
		shell_error(shell, "<encryption (0/1)>");
		return -EINVAL;
	}

	expected.encryption = strtol(argv[1], NULL, 10);

	return 0;
}

static int cmd_broadcast_code(const struct shell *shell, size_t argc, char **argv)
{
	if (argc < 2) {
		shell_error(shell, "<broadcast_code (16 chars)>");
		return -EINVAL;
	}

	memset(expected.broadcast_code, '\0', sizeof(expected.broadcast_code));
	memcpy(expected.broadcast_code, argv[1], strlen(argv[1]));
	memcpy(sink_broadcast_code, argv[1], strlen(argv[1]));

	return 0;
}

static int cmd_num_subgroups(const struct shell *shell, size_t argc, char **argv)
{
	if (argc < 2) {
		shell_error(shell, "<num_subgroups (uint8_t)>");
		return -EINVAL;
	}

	expected.num_subgroups = strtol(argv[1], NULL, 10);

	return 0;
}

static int cmd_std_quality(const struct shell *shell, size_t argc, char **argv)
{
	if (argc < 2) {
		shell_error(shell, "<std_quality (0/1)>");
		return -EINVAL;
	}

	expected.std_quality = strtol(argv[1], NULL, 10);

	return 0;
}

static int cmd_high_quality(const struct shell *shell, size_t argc, char **argv)
{
	if (argc < 2) {
		shell_error(shell, "<high_quality (0/1)>");
		return -EINVAL;
	}

	expected.high_quality = strtol(argv[1], NULL, 10);

	return 0;
}

static int cmd_sub_input(const struct shell *shell, size_t argc, char **argv)
{
	if (argc < 2) {
		shell_error(shell, "<subgroup_index> <sampling_rate_hz (uint32_t)> "
				   "<frame_duration_us "
				   "(uint32_t)> "
				   "<octets_per_frame (uint8_t)> <language (3 chars)> "
				   "<immediate_rend_flag (0/1)> <context (uint16_t)> "
				   "<program_info (string)> <num_bis (uint8_t)>");
		return -EINVAL;
	}

	int subgroup_index = strtol(argv[1], NULL, 10);

	if (subgroup_index >= CONFIG_BT_BAP_BROADCAST_SNK_SUBGROUP_COUNT) {
		shell_error(shell, "Invalid subgroup index");
		return -EINVAL;
	}

	expected.subgroups[subgroup_index].sampling_rate_hz = strtol(argv[2], NULL, 10);
	expected.subgroups[subgroup_index].frame_duration_us = strtol(argv[3], NULL, 10);
	expected.subgroups[subgroup_index].octets_per_frame = strtol(argv[4], NULL, 10);

	memcpy(expected.subgroups[subgroup_index].language, argv[5], 3);

	expected.subgroups[subgroup_index].immediate_rend_flag = strtol(argv[6], NULL, 10);
	expected.subgroups[subgroup_index].context = strtol(argv[7], NULL, 10);

	memset(expected.subgroups[subgroup_index].program_info, '\0',
	       sizeof(expected.subgroups[subgroup_index].program_info));
	memcpy(expected.subgroups[subgroup_index].program_info, argv[8], strlen(argv[8]));

	expected.subgroups[subgroup_index].num_bis = strtol(argv[9], NULL, 10);

	return 0;
}

static int cmd_sampling_rate(const struct shell *shell, size_t argc, char **argv)
{
	if (argc < 3) {
		shell_error(shell, "<sampling_rate_hz (uint32_t)> <subgroup_index>");
		return -EINVAL;
	}

	int subgroup_index = strtol(argv[2], NULL, 10);

	if (subgroup_index >= CONFIG_BT_BAP_BROADCAST_SNK_SUBGROUP_COUNT) {
		shell_error(shell, "Invalid subgroup index");
		return -EINVAL;
	}

	expected.subgroups[subgroup_index].sampling_rate_hz = strtol(argv[1], NULL, 10);

	return 0;
}

static int cmd_frame_duration(const struct shell *shell, size_t argc, char **argv)
{
	if (argc < 3) {
		shell_error(shell, "<frame_duration_us (uint32_t)> <subgroup_index>");
		return -EINVAL;
	}

	int subgroup_index = strtol(argv[2], NULL, 10);

	if (subgroup_index >= CONFIG_BT_BAP_BROADCAST_SNK_SUBGROUP_COUNT) {
		shell_error(shell, "Invalid subgroup index");
		return -EINVAL;
	}

	expected.subgroups[subgroup_index].frame_duration_us = strtol(argv[1], NULL, 10);

	return 0;
}

static int cmd_octets_per_frame(const struct shell *shell, size_t argc, char **argv)
{
	if (argc < 3) {
		shell_error(shell, "<octets_per_frame (uint8_t)> <subgroup_index>");
		return -EINVAL;
	}

	int subgroup_index = strtol(argv[2], NULL, 10);

	if (subgroup_index >= CONFIG_BT_BAP_BROADCAST_SNK_SUBGROUP_COUNT) {
		shell_error(shell, "Invalid subgroup index");
		return -EINVAL;
	}

	expected.subgroups[subgroup_index].octets_per_frame = strtol(argv[1], NULL, 10);

	return 0;
}

static int cmd_lang(const struct shell *shell, size_t argc, char **argv)
{
	if (argc < 3) {
		shell_error(shell, "<lang (3 chars)> <subgroup_index>");
		return -EINVAL;
	}

	int subgroup_index = strtol(argv[2], NULL, 10);

	if (subgroup_index >= CONFIG_BT_BAP_BROADCAST_SNK_SUBGROUP_COUNT) {
		shell_error(shell, "Invalid subgroup index");
		return -EINVAL;
	}

	memcpy(expected.subgroups[subgroup_index].language, argv[1], 3);

	return 0;
}

static int cmd_immediate(const struct shell *shell, size_t argc, char **argv)
{
	if (argc < 3) {
		shell_error(shell, "<immediate_rend_flag (0/1)> <subgroup_index> ");
		return -EINVAL;
	}

	int subgroup_index = strtol(argv[2], NULL, 10);

	if (subgroup_index >= CONFIG_BT_BAP_BROADCAST_SNK_SUBGROUP_COUNT) {
		shell_error(shell, "Invalid subgroup index");
		return -EINVAL;
	}

	expected.subgroups[subgroup_index].immediate_rend_flag = strtol(argv[1], NULL, 10);

	return 0;
}

static int cmd_context(const struct shell *shell, size_t argc, char **argv)
{
	uint16_t context = 0;

	if (argc < 3) {
		shell_error(shell, "<context (uint16_t)> <subgroup_index>");
		return -EINVAL;
	}

	int subgroup_index = strtol(argv[2], NULL, 10);

	if (subgroup_index >= CONFIG_BT_BAP_BROADCAST_SNK_SUBGROUP_COUNT) {
		shell_error(shell, "Invalid subgroup index");
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
		LOG_ERR("Context not found");
		return -EINVAL;
	}

	expected.subgroups[subgroup_index].context = context;

	return 0;
}

static int cmd_program_info(const struct shell *shell, size_t argc, char **argv)
{
	if (argc < 3) {
		shell_error(shell, "<program_info (string)> <subgroup_index>");
		return -EINVAL;
	}

	int subgroup_index = strtol(argv[2], NULL, 10);

	if (subgroup_index >= CONFIG_BT_BAP_BROADCAST_SNK_SUBGROUP_COUNT) {
		shell_error(shell, "Invalid subgroup index");
		return -EINVAL;
	}

	memset(expected.subgroups[subgroup_index].program_info, '\0',
	       sizeof(expected.subgroups[subgroup_index].program_info));
	memcpy(expected.subgroups[subgroup_index].program_info, argv[1], strlen(argv[1]));

	return 0;
}

static int cmd_num_bis(const struct shell *shell, size_t argc, char **argv)
{
	if (argc < 3) {
		shell_error(shell, "<num_bis (uint8_t)> <subgroup_index>");
		return -EINVAL;
	}

	int subgroup_index = strtol(argv[2], NULL, 10);

	if (subgroup_index >= CONFIG_BT_BAP_BROADCAST_SNK_SUBGROUP_COUNT) {
		shell_error(shell, "Invalid subgroup index");
		return -EINVAL;
	}

	expected.subgroups[subgroup_index].num_bis = strtol(argv[1], NULL, 10);

	return 0;
}

static int cmd_location(const struct shell *shell, size_t argc, char **argv)
{
	if (argc < 4) {
		shell_error(shell, "<location (uint32_t)> <subgroup_index> <bis_index>");
		return -EINVAL;
	}

	int subgroup_index = strtol(argv[2], NULL, 10);
	int bis_index = strtol(argv[3], NULL, 10);

	if (subgroup_index >= CONFIG_BT_BAP_BROADCAST_SNK_SUBGROUP_COUNT) {
		shell_error(shell, "Invalid subgroup index");
		return -EINVAL;
	}

	if (bis_index >= CONFIG_BT_BAP_BROADCAST_SNK_STREAM_COUNT) {
		shell_error(shell, "Invalid BIS index");
		return -EINVAL;
	}

	expected.subgroups[subgroup_index].locations[bis_index] = strtol(argv[1], NULL, 16);

	return 0;
}

static int cmd_run(const struct shell *shell, size_t argc, char **argv)
{
	int err;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (broadcast_name_target[0] == '\0') {
		LOG_DBG("No broadcast name target set");
		return -EINVAL;
	}

	/* Start scanning */
	err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, NULL);
	if (err != 0 && err != -EALREADY) {
		LOG_DBG("Unable to start scan for broadcast sources: %d", err);
	}

	LOG_INF("Started scan for: %s", broadcast_name_target);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	expected_cmd,
	SHELL_COND_CMD(CONFIG_SHELL, big_input, NULL, "Expected BIG values", cmd_big_input),
	SHELL_COND_CMD(CONFIG_SHELL, sub_input, NULL, "Expected subgroup values", cmd_sub_input),
	SHELL_COND_CMD(CONFIG_SHELL, adv_name, NULL, "Expected adv name", cmd_adv_name),
	SHELL_COND_CMD(CONFIG_SHELL, broadcast_name, NULL, "Expected broadcast name",
		       cmd_broadcast_name),
	SHELL_COND_CMD(CONFIG_SHELL, broadcast_id, NULL, "Expected broadcast ID", cmd_broadcast_id),
	SHELL_COND_CMD(CONFIG_SHELL, phy, NULL, "Expected PHY", cmd_phy),
	SHELL_COND_CMD(CONFIG_SHELL, pd_us, NULL, "Expected PD US", cmd_pd_us),
	SHELL_COND_CMD(CONFIG_SHELL, encryption, NULL, "Expected encryption", cmd_encryption),
	SHELL_COND_CMD(CONFIG_SHELL, broadcast_code, NULL, "Expected broadcast code",
		       cmd_broadcast_code),
	SHELL_COND_CMD(CONFIG_SHELL, num_subgroups, NULL, "Expected num subgroups",
		       cmd_num_subgroups),
	SHELL_COND_CMD(CONFIG_SHELL, std_quality, NULL, "Expected std quality", cmd_std_quality),
	SHELL_COND_CMD(CONFIG_SHELL, high_quality, NULL, "Expected high quality", cmd_high_quality),
	SHELL_COND_CMD(CONFIG_SHELL, sampling_rate, NULL, "Expected sampling rate",
		       cmd_sampling_rate),
	SHELL_COND_CMD(CONFIG_SHELL, frame_duration_us, NULL, "Expected frame duration",
		       cmd_frame_duration),
	SHELL_COND_CMD(CONFIG_SHELL, octets_per_frame, NULL, "Expected octets per frame",
		       cmd_octets_per_frame),
	SHELL_COND_CMD(CONFIG_SHELL, lang, NULL, "Expected language", cmd_lang),
	SHELL_COND_CMD(CONFIG_SHELL, immediate, NULL, "Expected immediate rend flag",
		       cmd_immediate),
	SHELL_COND_CMD(CONFIG_SHELL, context, NULL, "Expected context", cmd_context),
	SHELL_COND_CMD(CONFIG_SHELL, program_info, NULL, "Expected program info", cmd_program_info),
	SHELL_COND_CMD(CONFIG_SHELL, num_bis, NULL, "Expected num BIS", cmd_num_bis),
	SHELL_COND_CMD(CONFIG_SHELL, location, NULL, "Expected BIS location", cmd_location),
	SHELL_COND_CMD(CONFIG_SHELL, run_test, NULL, "Run test", cmd_run), SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(nac_test, &expected_cmd, "nrf Auraconfig tester", NULL);
