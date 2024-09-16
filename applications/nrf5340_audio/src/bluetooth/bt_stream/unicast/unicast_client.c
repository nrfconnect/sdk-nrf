/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "unicast_client.h"

#include <zephyr/sys/byteorder.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/csip.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>
#include <../subsys/bluetooth/audio/bap_iso.h>

/* TODO: Remove when a qos_pref_get function has been added in host */
/* https://github.com/zephyrproject-rtos/zephyr/issues/72359 */
#include <../subsys/bluetooth/audio/bap_endpoint.h>

#include "macros_common.h"
#include "zbus_common.h"
#include "bt_le_audio_tx.h"
#include "le_audio.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(unicast_client, CONFIG_UNICAST_CLIENT_LOG_LEVEL);

ZBUS_CHAN_DEFINE(le_audio_chan, struct le_audio_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
		 ZBUS_MSG_INIT(0));

#define CAP_PROCED_MUTEX_WAIT_TIME_MS K_MSEC(500)

struct le_audio_unicast_server {
	char *ch_name;
	struct bt_conn *device_conn;
	enum bt_audio_location location;
	bool qos_reconfigure;
	uint32_t reconfigure_pd;
	/* Sink variables */
	bool waiting_for_sink_disc;
	struct bt_audio_codec_cap sink_codec_cap[CONFIG_CODEC_CAP_COUNT_MAX];
	uint8_t num_sink_eps;
	struct bt_bap_ep *sink_ep;
	struct bt_cap_stream cap_sink_stream;
	/* Source variables */
	bool waiting_for_source_disc;
	struct bt_audio_codec_cap source_codec_cap[CONFIG_CODEC_CAP_COUNT_MAX];
	uint8_t num_source_eps;
	struct bt_bap_ep *source_ep;
	struct bt_cap_stream cap_source_stream;
	const struct bt_csip_set_coordinator_set_member *member;
};

struct discover_dir {
	struct bt_conn *conn;
	bool sink;
	bool source;
};

struct temp_cap_storage {
	struct bt_conn *conn;
	uint8_t num_caps;
	/* Must be the same size as sink_codec_cap and source_codec_cap */
	struct bt_audio_codec_cap codec[CONFIG_CODEC_CAP_COUNT_MAX];
};

/* Since there is no subgroups for CIG we will use 1 as a hard coded value */
static struct le_audio_unicast_server unicast_servers[CONFIG_BT_ISO_MAX_CIG][1][CONFIG_BT_MAX_CONN];

K_MSGQ_DEFINE(cap_start_msgq, sizeof(struct stream_index), CONFIG_BT_ISO_MAX_CHAN,
	      sizeof(uint32_t));

static struct temp_cap_storage temp_cap[CONFIG_BT_ISO_MAX_CHAN];

/* Make sure that we have at least one unicast_server per CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK */
BUILD_ASSERT(ARRAY_SIZE(unicast_servers[0][0]) >= CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT,
	     "We need to have at least one unicast_server per ASE SINK");

/* Make sure that we have at least one unicast_server per CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC */
BUILD_ASSERT(ARRAY_SIZE(unicast_servers[0][0]) >= CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT,
	     "We need to have at least one unicast_server per ASE SOURCE");

BUILD_ASSERT(CONFIG_BT_ISO_MAX_CIG == 1, "Only one CIG is supported");

static le_audio_receive_cb receive_cb;

static struct bt_bap_unicast_group *unicast_group;
static bool unicast_group_created;

static struct bt_bap_lc3_preset lc3_preset_sink = BT_BAP_LC3_UNICAST_PRESET_NRF5340_AUDIO_SINK;
static struct bt_bap_lc3_preset lc3_preset_sink_48_4_1 =
	BT_BAP_LC3_UNICAST_PRESET_48_4_1(BT_AUDIO_LOCATION_ANY, (BT_AUDIO_CONTEXT_TYPE_ANY));
static struct bt_bap_lc3_preset lc3_preset_sink_24_2_1 =
	BT_BAP_LC3_UNICAST_PRESET_24_2_1(BT_AUDIO_LOCATION_ANY, (BT_AUDIO_CONTEXT_TYPE_ANY));
static struct bt_bap_lc3_preset lc3_preset_sink_16_2_1 =
	BT_BAP_LC3_UNICAST_PRESET_16_2_1(BT_AUDIO_LOCATION_ANY, (BT_AUDIO_CONTEXT_TYPE_ANY));

static struct bt_bap_lc3_preset lc3_preset_source = BT_BAP_LC3_UNICAST_PRESET_NRF5340_AUDIO_SOURCE;
static struct bt_bap_lc3_preset lc3_preset_source_48_4_1 =
	BT_BAP_LC3_UNICAST_PRESET_48_4_1(BT_AUDIO_LOCATION_ANY, BT_AUDIO_CONTEXT_TYPE_ANY);
static struct bt_bap_lc3_preset lc3_preset_source_24_2_1 =
	BT_BAP_LC3_UNICAST_PRESET_24_2_1(BT_AUDIO_LOCATION_ANY, BT_AUDIO_CONTEXT_TYPE_ANY);
static struct bt_bap_lc3_preset lc3_preset_source_16_2_1 =
	BT_BAP_LC3_UNICAST_PRESET_16_2_1(BT_AUDIO_LOCATION_ANY, BT_AUDIO_CONTEXT_TYPE_ANY);

static bool playing_state = true;

static void le_audio_event_publish(enum le_audio_evt_type event, struct bt_conn *conn,
				   enum bt_audio_dir dir)
{
	int ret;
	struct le_audio_msg msg;

	msg.event = event;
	msg.conn = conn;
	msg.dir = dir;

	ret = zbus_chan_pub(&le_audio_chan, &msg, LE_AUDIO_ZBUS_EVENT_WAIT_TIME);
	ERR_CHK(ret);
}

K_MUTEX_DEFINE(mtx_cap_procedure_proceed);

static void cap_start_worker(struct k_work *work)
{
	int ret;
	struct stream_index idx;
	int device_iterator = 0;
	int stream_iterator = 0;

	/* Check msgq for a pending start procedure */
	ret = k_msgq_get(&cap_start_msgq, &idx, K_NO_WAIT);
	if (ret) {
		LOG_ERR("Failed to get device index for pending cap start procedure: %d", ret);
		return;
	}

	struct le_audio_unicast_server *unicast_server =
		&unicast_servers[idx.lvl1][idx.lvl2][idx.lvl3];

	if (unicast_server->sink_ep == NULL && unicast_server->source_ep == NULL) {
		LOG_ERR("No sink or source endpoint found for device");
		return;
	}

	if (unicast_group_created == false) {
		uint8_t cig_index = idx.lvl1;
		struct bt_bap_unicast_group_stream_pair_param
			pair_params[ARRAY_SIZE(unicast_servers[cig_index][0])];
		/* 2 streams (one sink and one source stream) for each unicast_server */
		struct bt_bap_unicast_group_stream_param
			group_stream_params[(ARRAY_SIZE(unicast_servers[cig_index][0]) * 2)];
		struct bt_bap_unicast_group_param group_param;

		for (int i = 0; i < ARRAY_SIZE(group_stream_params); i++) {
			/* Every other stream should be sink or source */
			if ((i % 2) == 0) {
				group_stream_params[i].qos = &lc3_preset_sink.qos;
				group_stream_params[i].stream =
					&unicast_servers[cig_index][0][device_iterator]
						 .cap_sink_stream.bap_stream;
			} else {
				group_stream_params[i].qos = &lc3_preset_source.qos;
				group_stream_params[i].stream =
					&unicast_servers[cig_index][0][device_iterator]
						 .cap_source_stream.bap_stream;
				device_iterator++;
			}
		}

		for (int i = 0; i < ARRAY_SIZE(pair_params); i++) {
			if (unicast_server->sink_ep) {
				pair_params[i].tx_param = &group_stream_params[stream_iterator];
			} else {
				pair_params[i].tx_param = NULL;
			}
			stream_iterator++;

			if (unicast_server->source_ep) {
				pair_params[i].rx_param = &group_stream_params[stream_iterator];
			} else {
				pair_params[i].rx_param = NULL;
			}

			stream_iterator++;
		}

		group_param.params = pair_params;
		group_param.params_count = ARRAY_SIZE(pair_params);

		if (IS_ENABLED(CONFIG_BT_AUDIO_PACKING_INTERLEAVED)) {
			group_param.packing = BT_ISO_PACKING_INTERLEAVED;
		} else {
			group_param.packing = BT_ISO_PACKING_SEQUENTIAL;
		}

		ret = bt_bap_unicast_group_create(&group_param, &unicast_group);
		if (ret) {
			LOG_ERR("Failed to create unicast group: %d", ret);
		} else {
			unicast_group_created = true;
		}
	}

	ret = k_mutex_lock(&mtx_cap_procedure_proceed, CAP_PROCED_MUTEX_WAIT_TIME_MS);
	if (ret == -EAGAIN) {
		LOG_ERR("CAP procedure lock timeout");
	}

	struct bt_cap_unicast_audio_start_stream_param
		cap_stream_params[CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT +
				  CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT];

	struct bt_cap_unicast_audio_start_param param;

	param.stream_params = cap_stream_params;
	param.count = 0;
	param.type = BT_CAP_SET_TYPE_AD_HOC;

	if (unicast_server->sink_ep) {
		cap_stream_params[param.count].member.member = unicast_server->device_conn;
		cap_stream_params[param.count].stream = &unicast_server->cap_sink_stream;
		cap_stream_params[param.count].ep = unicast_server->sink_ep;
		cap_stream_params[param.count].codec_cfg = &lc3_preset_sink.codec_cfg;
		param.count++;
	}

	if (unicast_server->source_ep) {
		cap_stream_params[param.count].member.member = unicast_server->device_conn;
		cap_stream_params[param.count].stream = &unicast_server->cap_source_stream;
		cap_stream_params[param.count].ep = unicast_server->source_ep;
		cap_stream_params[param.count].codec_cfg = &lc3_preset_source.codec_cfg;
		param.count++;
	}

	ret = bt_cap_initiator_unicast_audio_start(&param);
	if (ret == -EBUSY) {
		/* Try again once the ongoing start procedure is completed */
		ret = k_msgq_put(&cap_start_msgq, &idx, K_NO_WAIT);
		if (ret) {
			LOG_ERR("Failed to put device_index on the queue: %d", ret);
		}
	} else if (ret) {
		LOG_ERR("Failed to start unicast sink audio: %d", ret);
	}
}
K_WORK_DEFINE(cap_start_work, cap_start_worker);

/**
 * @brief  Get the common presentation delay for all unicast_servers.
 *
 * @param[in]	index		The index of the device to test against.
 * @param[out]	pres_dly_us	Pointer to where the presentation delay will be stored.
 *
 * @retval	0	Operation successful.
 * @retval	-EINVAL	Any error.
 */
static int device_pres_delay_find(struct stream_index idx, uint32_t *pres_dly_us)
{
	struct le_audio_unicast_server *unicast_server =
		&unicast_servers[idx.lvl1][idx.lvl2][idx.lvl3];

	uint32_t pd_min = unicast_server->sink_ep->qos_pref.pd_min;
	uint32_t pd_max = unicast_server->sink_ep->qos_pref.pd_max;
	uint32_t pref_pd_min = unicast_server->sink_ep->qos_pref.pref_pd_min;
	uint32_t pref_pd_max = unicast_server->sink_ep->qos_pref.pref_pd_max;

	LOG_DBG("Index: %d, Pref min: %d, pref max: %d, pres_min: %d, pres_max: %d", idx.lvl3,
		pref_pd_min, pref_pd_max, pd_min, pd_max);

	*pres_dly_us = 0;

	for (int i = 0; i < ARRAY_SIZE(unicast_servers[idx.lvl1][idx.lvl2]); i++) {
		if (le_audio_ep_qos_configured(unicast_servers[idx.lvl1][idx.lvl2][i].sink_ep)) {
			LOG_DBG("i: %d, Pref min: %d, pref max: %d, pres_min: %d, pres_max: %d", i,
				unicast_servers[idx.lvl1][idx.lvl2][i]
					.sink_ep->qos_pref.pref_pd_min,
				unicast_servers[idx.lvl1][idx.lvl2][i]
					.sink_ep->qos_pref.pref_pd_max,
				unicast_servers[idx.lvl1][idx.lvl2][i].sink_ep->qos_pref.pd_min,
				unicast_servers[idx.lvl1][idx.lvl2][i].sink_ep->qos_pref.pd_max);

			pd_min = MAX(
				pd_min,
				unicast_servers[idx.lvl1][idx.lvl2][i].sink_ep->qos_pref.pd_min);
			pref_pd_min = MAX(pref_pd_min, unicast_servers[idx.lvl1][idx.lvl2][i]
							       .sink_ep->qos_pref.pref_pd_min);

			if (unicast_servers[idx.lvl1][idx.lvl2][i].sink_ep->qos_pref.pd_max) {
				pd_max = MIN(pd_max, unicast_servers[idx.lvl1][idx.lvl2][i]
							     .sink_ep->qos_pref.pd_max);
			}

			if (unicast_servers[idx.lvl1][idx.lvl2][i].sink_ep->qos_pref.pref_pd_max) {
				pref_pd_max =
					MIN(pref_pd_max, unicast_servers[idx.lvl1][idx.lvl2][i]
								 .sink_ep->qos_pref.pref_pd_max);
			}
		}
	}

	if (IS_ENABLED(CONFIG_BT_AUDIO_PRES_DELAY_SRCH_MIN)) {
		*pres_dly_us = pd_min;

		return 0;
	}

	if (IS_ENABLED(CONFIG_BT_AUDIO_PRES_DELAY_SRCH_MAX)) {
		*pres_dly_us = pd_max;

		return 0;
	}

	if (IS_ENABLED(CONFIG_BT_AUDIO_PRES_DELAY_SRCH_PREF_MIN)) {
		if (pref_pd_min == 0) {
			*pres_dly_us = pd_min;
		} else if (pref_pd_min < pd_min) {
			*pres_dly_us = pd_min;
			LOG_WRN("pref_pd_min < pd_min (%d < %d), pres delay set to %d", pref_pd_min,
				pd_min, *pres_dly_us);
		} else if (pref_pd_min <= pd_max) {
			*pres_dly_us = pref_pd_min;
		} else {
			*pres_dly_us = pd_max;
			LOG_WRN("pref_pd_min > pd_max (%d > %d), pres delay set to %d", pref_pd_min,
				pd_max, *pres_dly_us);
		}

		return 0;
	}

	if (IS_ENABLED(CONFIG_BT_AUDIO_PRES_DELAY_SRCH_PREF_MAX)) {
		if (pref_pd_max == 0) {
			*pres_dly_us = pd_max;
		} else if (pref_pd_max > pd_max) {
			*pres_dly_us = pd_max;
			LOG_WRN("pref_pd_max > pd_max (%d > %d), pres delay set to %d", pref_pd_max,
				pd_max, *pres_dly_us);
		} else if (pref_pd_max >= pd_min) {
			*pres_dly_us = pref_pd_max;
		} else {
			*pres_dly_us = pd_min;
			LOG_WRN("pref_pd_max < pd_min (%d < %d), pres delay set to %d", pref_pd_max,
				pd_min, *pres_dly_us);
		}

		return 0;
	}

	LOG_ERR("Trying to use unrecognized search mode");

	return -EINVAL;
}

/**
 * @brief	Get device index based on connection.
 *
 * @param[in]	conn	The connection to search for.
 * @param[out]	index	The device index.
 *
 * @retval	0	Operation successful.
 * @retval	-EINVAL	There is no match.
 */
static int device_index_get(const struct bt_conn *conn, struct stream_index *idx)
{
	if (conn == NULL) {
		LOG_ERR("No connection provided");
		return -EINVAL;
	}

	for (int i = 0; i < CONFIG_BT_ISO_MAX_CIG; i++) {
		for (int j = 0; j < ARRAY_SIZE(unicast_servers[i][0]); j++) {
			if (unicast_servers[i][0][j].device_conn == conn) {
				idx->lvl1 = i;
				idx->lvl2 = 0;
				idx->lvl3 = j;
				return 0;
			}
		}
	}

	return -EINVAL;
}

static int device_index_vacant_get(const struct bt_conn *conn, struct stream_index *idx)
{

	if (device_index_get(conn, idx) == 0) {
		LOG_WRN("Device has already been discovered");
		return -EALREADY;
	}

	for (int i = 0; i < CONFIG_BT_ISO_MAX_CIG; i++) {
		for (int j = 0; j < ARRAY_SIZE(unicast_servers[i][0]); j++) {
			if (unicast_servers[i][0][j].device_conn == NULL) {
				idx->lvl1 = i;
				idx->lvl2 = 0;
				idx->lvl3 = j;
				return 0;
			}
		}
	}

	LOG_WRN("No more room in device list");
	return -ENOSPC;
}

static void supported_sample_rates_print(uint16_t supported_sample_rates, enum bt_audio_dir dir)
{
	char supported_str[20] = "";

	if (supported_sample_rates & BT_AUDIO_CODEC_CAP_FREQ_48KHZ) {
		strcat(supported_str, "48, ");
	}

	if (supported_sample_rates & BT_AUDIO_CODEC_CAP_FREQ_24KHZ) {
		strcat(supported_str, "24, ");
	}

	if (supported_sample_rates & BT_AUDIO_CODEC_CAP_FREQ_32KHZ) {
		strcat(supported_str, "32, ");
	}

	if (supported_sample_rates & BT_AUDIO_CODEC_CAP_FREQ_16KHZ) {
		strcat(supported_str, "16, ");
	}

	if (dir == BT_AUDIO_DIR_SINK) {
		LOG_DBG("Unicast_server supports: %s kHz in sink direction", supported_str);
	} else if (dir == BT_AUDIO_DIR_SOURCE) {
		LOG_DBG("Unicast_server supports: %s kHz in source direction", supported_str);
	}
}

static bool sink_parse_cb(struct bt_data *data, void *user_data)
{
	if (data->type == BT_AUDIO_CODEC_CAP_TYPE_FREQ) {
		uint16_t lc3_freq_bit = sys_get_le16(data->data);

		supported_sample_rates_print(lc3_freq_bit, BT_AUDIO_DIR_SINK);

		/* Try with the preferred sample rate first */
		switch (CONFIG_BT_AUDIO_PREF_SAMPLE_RATE_VALUE) {
		case BT_AUDIO_CODEC_CFG_FREQ_48KHZ:
			if (lc3_freq_bit & BT_AUDIO_CODEC_CAP_FREQ_48KHZ) {
				lc3_preset_sink = lc3_preset_sink_48_4_1;
				*(bool *)user_data = true;
				/* Found what we were looking for, stop parsing LTV */
				return false;
			}

			break;

		case BT_AUDIO_CODEC_CFG_FREQ_24KHZ:
			if (lc3_freq_bit & BT_AUDIO_CODEC_CAP_FREQ_24KHZ) {
				lc3_preset_sink = lc3_preset_sink_24_2_1;
				*(bool *)user_data = true;
				/* Found what we were looking for, stop parsing LTV */
				return false;
			}

			break;

		case BT_AUDIO_CODEC_CFG_FREQ_16KHZ:
			if (lc3_freq_bit & BT_AUDIO_CODEC_CAP_FREQ_16KHZ) {
				lc3_preset_sink = lc3_preset_sink_16_2_1;
				*(bool *)user_data = true;
				/* Found what we were looking for, stop parsing LTV */
				return false;
			}

			break;
		}

		/* If no match with the preferred, revert to trying highest first */
		if (lc3_freq_bit & BT_AUDIO_CODEC_CAP_FREQ_48KHZ) {
			lc3_preset_sink = lc3_preset_sink_48_4_1;
			*(bool *)user_data = true;
		} else if (lc3_freq_bit & BT_AUDIO_CODEC_CAP_FREQ_24KHZ) {
			lc3_preset_sink = lc3_preset_sink_24_2_1;
			*(bool *)user_data = true;
		} else if (lc3_freq_bit & BT_AUDIO_CODEC_CAP_FREQ_16KHZ) {
			lc3_preset_sink = lc3_preset_sink_16_2_1;
			*(bool *)user_data = true;
		}

		/* Found what we were looking for, stop parsing LTV */
		return false;
	}

	/* Did not find what we were looking for, continue parsing LTV */
	return true;
}

static void set_color_if_supported(char *str, uint16_t bitfield, uint16_t mask)
{
	if (bitfield & mask) {
		strcat(str, COLOR_GREEN);
	} else {
		strcat(str, COLOR_RED);
	}
}

static bool caps_print_cb(struct bt_data *data, void *user_data)
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

		LOG_INF("\tFrequencies kHz: %s", supported_freq);
	}

	if (data->type == BT_AUDIO_CODEC_CAP_TYPE_DURATION) {
		uint16_t dur_bit = sys_get_le16(data->data);
		char supported_dur[30] = "";

		set_color_if_supported(supported_dur, dur_bit, BT_AUDIO_CODEC_CAP_DURATION_7_5);
		strcat(supported_dur, "7.5, ");
		set_color_if_supported(supported_dur, dur_bit, BT_AUDIO_CODEC_CAP_DURATION_10);
		strcat(supported_dur, "10");

		LOG_INF("\tFrame duration ms: %s", supported_dur);
	}

	if (data->type == BT_AUDIO_CODEC_CAP_TYPE_CHAN_COUNT) {
		uint16_t chan_bit = sys_get_le16(data->data);
		char supported_chan[120] = "";

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

static bool source_parse_cb(struct bt_data *data, void *user_data)
{
	if (data->type == BT_AUDIO_CODEC_CAP_TYPE_FREQ) {
		uint16_t lc3_freq_bit = sys_get_le16(data->data);

		supported_sample_rates_print(lc3_freq_bit, BT_AUDIO_DIR_SOURCE);

		/* Try with the preferred sample rate first */
		switch (CONFIG_BT_AUDIO_PREF_SAMPLE_RATE_VALUE) {
		case BT_AUDIO_CODEC_CFG_FREQ_48KHZ:
			if (lc3_freq_bit & BT_AUDIO_CODEC_CAP_FREQ_48KHZ) {
				lc3_preset_source = lc3_preset_source_48_4_1;
				*(bool *)user_data = true;
				/* Found what we were looking for, stop parsing LTV */
				return false;
			}

			break;

		case BT_AUDIO_CODEC_CFG_FREQ_24KHZ:
			if (lc3_freq_bit & BT_AUDIO_CODEC_CAP_FREQ_24KHZ) {
				lc3_preset_source = lc3_preset_source_24_2_1;
				*(bool *)user_data = true;
				/* Found what we were looking for, stop parsing LTV */
				return false;
			}

			break;

		case BT_AUDIO_CODEC_CFG_FREQ_16KHZ:
			if (lc3_freq_bit & BT_AUDIO_CODEC_CAP_FREQ_16KHZ) {
				lc3_preset_source = lc3_preset_source_16_2_1;
				*(bool *)user_data = true;
				/* Found what we were looking for, stop parsing LTV */
				return false;
			}

			break;
		}

		/* If no match with the preferred, revert to trying highest first */
		if (lc3_freq_bit & BT_AUDIO_CODEC_CAP_FREQ_48KHZ) {
			lc3_preset_source = lc3_preset_source_48_4_1;
			*(bool *)user_data = true;
		} else if (lc3_freq_bit & BT_AUDIO_CODEC_CAP_FREQ_24KHZ) {
			lc3_preset_source = lc3_preset_source_24_2_1;
			*(bool *)user_data = true;
		} else if (lc3_freq_bit & BT_AUDIO_CODEC_CAP_FREQ_16KHZ) {
			lc3_preset_source = lc3_preset_source_16_2_1;
			*(bool *)user_data = true;
		}

		/* Found what we were looking for, stop parsing LTV */
		return false;
	}

	/* Did not find what we were looking for, continue parsing LTV */
	return true;
}

/**
 * @brief	Check if the gateway can support the codec capabilities.
 *
 * @note	Currently only the sampling frequency is checked.
 *
 * @param[in]	cap_array	The array of pointers to codec capabilities.
 * @param[in]	num_caps	The size of cap_array.
 * @param[in]	dir		Direction of the capabilities to check.
 * @param[in]	index		Device index.
 *
 * @return	True if valid codec capability found, false otherwise.
 */
static bool valid_codec_cap_check(struct bt_audio_codec_cap cap_array[], uint8_t num_caps,
				  enum bt_audio_dir dir, uint8_t index)
{
	bool valid_result = false;

	/* Only the sampling frequency is checked */
	if (dir == BT_AUDIO_DIR_SINK) {
		LOG_INF("Discovered %d sink endpoint(s) for device %d", num_caps, index);
		for (int i = 0; i < num_caps; i++) {
			if (IS_ENABLED(CONFIG_BT_AUDIO_EP_PRINT)) {
				LOG_INF("");
				LOG_INF("Dev: %d Sink EP %d", index, i);
				(void)bt_audio_data_parse(cap_array[i].data, cap_array[i].data_len,
							  caps_print_cb, NULL);
				LOG_INF("__________________________");
			}

			(void)bt_audio_data_parse(cap_array[i].data, cap_array[i].data_len,
						  sink_parse_cb, &valid_result);
		}
	} else if (dir == BT_AUDIO_DIR_SOURCE) {
		LOG_INF("Discovered %d source endpoint(s) for device %d", num_caps, index);
		for (int i = 0; i < num_caps; i++) {
			if (IS_ENABLED(CONFIG_BT_AUDIO_EP_PRINT)) {
				LOG_INF("");
				LOG_INF("Dev: %d Source EP %d", index, i);
				(void)bt_audio_data_parse(cap_array[i].data, cap_array[i].data_len,
							  caps_print_cb, NULL);
				LOG_INF("__________________________");
			}

			(void)bt_audio_data_parse(cap_array[i].data, cap_array[i].data_len,
						  source_parse_cb, &valid_result);
		}
	}

	return valid_result;
}

/**
 * @brief Set the allocation to a preset codec configuration.
 *
 * @param codec The preset codec configuration.
 * @param loc   Location bitmask setting.
 *
 */
static void bt_audio_codec_allocation_set(struct bt_audio_codec_cfg *codec_cfg,
					  enum bt_audio_location loc)
{
	for (size_t i = 0U; i < codec_cfg->data_len;) {
		const uint8_t len = codec_cfg->data[i++];
		const uint8_t type = codec_cfg->data[i++];
		uint8_t *value = &codec_cfg->data[i];
		const uint8_t value_len = len - sizeof(type);

		if (type == BT_AUDIO_CODEC_CFG_CHAN_ALLOC) {
			const uint32_t loc_32 = loc;

			sys_put_le32(loc_32, value);

			return;
		}
		i += value_len;
	}
}

static int update_cap_sink_stream_qos(struct le_audio_unicast_server *unicast_server,
				      uint32_t pres_delay_us)
{
	int ret;

	if (unicast_server->cap_sink_stream.bap_stream.ep == NULL) {
		return -ESRCH;
	}

	if (unicast_server->cap_sink_stream.bap_stream.qos == NULL) {
		LOG_WRN("No QoS found for %p", (void *)&unicast_server->cap_sink_stream.bap_stream);
		return -ENXIO;
	}

	if (unicast_server->cap_sink_stream.bap_stream.qos->pd != pres_delay_us) {
		struct bt_cap_unicast_audio_stop_param param;
		struct bt_cap_stream *streams[2];

		LOG_DBG("Current preset PD = %d us, target PD = %d us",
			unicast_server->cap_sink_stream.bap_stream.qos->pd, pres_delay_us);

		param.streams = streams;
		param.count = 0;
		param.type = BT_CAP_SET_TYPE_AD_HOC;

		if (playing_state &&
		    le_audio_ep_state_check(unicast_server->cap_sink_stream.bap_stream.ep,
					    BT_BAP_EP_STATE_STREAMING)) {
			LOG_DBG("Update streaming %s unicast_server, connection %p, stream %p",
				unicast_server->ch_name, (void *)&unicast_server->device_conn,
				(void *)&unicast_server->cap_sink_stream.bap_stream);

			unicast_server->qos_reconfigure = true;
			unicast_server->reconfigure_pd = pres_delay_us;

			streams[param.count] = &unicast_server->cap_sink_stream;
			param.count++;
		} else {
			LOG_DBG("Reset %s unicast_server, connection %p, stream %p",
				unicast_server->ch_name, (void *)&unicast_server->device_conn,
				(void *)&unicast_server->cap_sink_stream.bap_stream);
			unicast_server->cap_sink_stream.bap_stream.qos->pd = pres_delay_us;
		}

		if (playing_state &&
		    le_audio_ep_state_check(unicast_server->cap_source_stream.bap_stream.ep,
					    BT_BAP_EP_STATE_STREAMING)) {
			unicast_server->qos_reconfigure = true;
			unicast_server->reconfigure_pd = pres_delay_us;

			streams[param.count] = &unicast_server->cap_source_stream;
			param.count++;
		}

		if (param.count > 0) {
			ret = bt_cap_initiator_unicast_audio_stop(&param);
			if (ret) {
				LOG_WRN("Failed to stop streams: %d, use default PD in preset",
					ret);
				return ret;
			}
		}
	}

	return 0;
}

static void unicast_client_location_cb(struct bt_conn *conn, enum bt_audio_dir dir,
				       enum bt_audio_location loc)
{
	int ret;
	struct stream_index idx;

	ret = device_index_get(conn, &idx);
	if (ret) {
		LOG_ERR("Device index not found");
		return;
	}

	struct le_audio_unicast_server *unicast_server =
		&unicast_servers[idx.lvl1][idx.lvl2][idx.lvl3];

	if ((loc & BT_AUDIO_LOCATION_FRONT_LEFT) || (loc & BT_AUDIO_LOCATION_SIDE_LEFT) ||
	    (loc == BT_AUDIO_LOCATION_MONO_AUDIO)) {
		unicast_server->location = BT_AUDIO_LOCATION_FRONT_LEFT;
		unicast_server->ch_name = "LEFT";

	} else if ((loc & BT_AUDIO_LOCATION_FRONT_RIGHT) || (loc & BT_AUDIO_LOCATION_SIDE_RIGHT)) {
		unicast_server->location = BT_AUDIO_LOCATION_FRONT_RIGHT;
		unicast_server->ch_name = "RIGHT";
	} else {
		LOG_WRN("Channel location not supported: %d", loc);
		le_audio_event_publish(LE_AUDIO_EVT_NO_VALID_CFG, conn, dir);
	}
}

static void available_contexts_cb(struct bt_conn *conn, enum bt_audio_context snk_ctx,
				  enum bt_audio_context src_ctx)
{
	char addr[BT_ADDR_LE_STR_LEN];

	(void)bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_DBG("conn: %s, snk ctx %d src ctx %d", addr, snk_ctx, src_ctx);
}

static int temp_cap_index_get(struct bt_conn *conn, uint8_t *index)
{
	if (conn == NULL) {
		LOG_ERR("No conn provided");
		return -EINVAL;
	}

	for (int i = 0; i < ARRAY_SIZE(temp_cap); i++) {
		if (temp_cap[i].conn == conn) {
			*index = i;
			return 0;
		}
	}

	/* Connection not found in temp_cap, searching for empty slot */
	for (int i = 0; i < ARRAY_SIZE(temp_cap); i++) {
		if (temp_cap[i].conn == NULL) {
			temp_cap[i].conn = conn;
			*index = i;
			return 0;
		}
	}

	LOG_WRN("No more space in temp_cap");

	return -ECANCELED;
}

static void pac_record_cb(struct bt_conn *conn, enum bt_audio_dir dir,
			  const struct bt_audio_codec_cap *codec)
{
	int ret;
	uint8_t temp_cap_index;

	ret = temp_cap_index_get(conn, &temp_cap_index);
	if (ret) {
		LOG_ERR("Could not get temporary CAP storage index");
		return;
	}

	if (codec->id != BT_HCI_CODING_FORMAT_LC3) {
		LOG_DBG("Only the LC3 codec is supported");
		return;
	}

	/* num_caps is an increasing index that starts at 0 */
	if (temp_cap[temp_cap_index].num_caps < ARRAY_SIZE(temp_cap[temp_cap_index].codec)) {
		struct bt_audio_codec_cap *codec_loc =
			&temp_cap[temp_cap_index].codec[temp_cap[temp_cap_index].num_caps];

		memcpy(codec_loc, codec, sizeof(struct bt_audio_codec_cap));

		temp_cap[temp_cap_index].num_caps++;
	} else {
		LOG_WRN("No more space. Increase CONFIG_CODEC_CAP_COUNT_MAX");
	}
}

static void endpoint_cb(struct bt_conn *conn, enum bt_audio_dir dir, struct bt_bap_ep *ep)
{
	int ret;
	struct stream_index idx;

	ret = device_index_get(conn, &idx);
	if (ret) {
		LOG_ERR("Unknown connection, should not reach here");
		return;
	}

	struct le_audio_unicast_server *unicast_server =
		&unicast_servers[idx.lvl1][idx.lvl2][idx.lvl3];

	if (dir == BT_AUDIO_DIR_SINK) {
		if (ep != NULL) {
			if (unicast_server->num_sink_eps > 0) {
				LOG_WRN("More than one sink endpoint found, idx 0 is used "
					"by default");
				return;
			}

			unicast_server->sink_ep = ep;
			unicast_server->num_sink_eps++;
			return;
		}

		if (unicast_server->sink_ep == NULL) {
			LOG_WRN("No sink endpoints found");
		}

		return;
	} else if (dir == BT_AUDIO_DIR_SOURCE) {
		if (ep != NULL) {
			if (unicast_server->num_source_eps > 0) {
				LOG_WRN("More than one source endpoint found, idx 0 is "
					"used by default");
				return;
			}

			unicast_server->source_ep = ep;
			unicast_server->num_source_eps++;
			return;
		}

		if (unicast_server->source_ep == NULL) {
			LOG_WRN("No source endpoints found");
		}

		return;
	} else {
		LOG_WRN("Endpoint direction not recognized: %d", dir);
	}
}

static void discover_cb(struct bt_conn *conn, int err, enum bt_audio_dir dir)
{
	int ret;
	uint8_t temp_cap_index;

	struct stream_index idx;

	ret = device_index_get(conn, &idx);
	if (ret) {
		LOG_ERR("Unknown connection, should not reach here");
		return;
	}

	struct le_audio_unicast_server *unicast_server =
		&unicast_servers[idx.lvl1][idx.lvl2][idx.lvl3];

	if (err == BT_ATT_ERR_ATTRIBUTE_NOT_FOUND) {
		if (dir == BT_AUDIO_DIR_SINK) {
			LOG_WRN("No sinks found");
			unicast_server->waiting_for_sink_disc = false;
		} else if (dir == BT_AUDIO_DIR_SOURCE) {
			LOG_WRN("No sources found");
			unicast_server->waiting_for_source_disc = false;
		}
	} else if (err) {
		LOG_ERR("Discovery failed: %d", err);
		return;
	}

	ret = temp_cap_index_get(conn, &temp_cap_index);
	if (ret) {
		LOG_ERR("Could not get temporary CAP storage index");
		return;
	}

	for (int i = 0; i < CONFIG_CODEC_CAP_COUNT_MAX; i++) {
		if (dir == BT_AUDIO_DIR_SINK && !err) {
			memcpy(&unicast_server->sink_codec_cap[i],
			       &temp_cap[temp_cap_index].codec[i],
			       sizeof(struct bt_audio_codec_cap));
		} else if (dir == BT_AUDIO_DIR_SOURCE && !err) {
			memcpy(&unicast_server->source_codec_cap[i],
			       &temp_cap[temp_cap_index].codec[i],
			       sizeof(struct bt_audio_codec_cap));
		}
	}

	if (dir == BT_AUDIO_DIR_SINK && !err) {
		if (valid_codec_cap_check(unicast_server->sink_codec_cap,
					  temp_cap[temp_cap_index].num_caps, BT_AUDIO_DIR_SINK,
					  idx.lvl3)) {
			bt_audio_codec_allocation_set(&lc3_preset_sink.codec_cfg,
						      unicast_server->location);
		} else {
			/* NOTE: The string below is used by the Nordic CI system */
			LOG_WRN("No valid codec capability found for %s sink",
				unicast_server->ch_name);
			unicast_server->sink_ep = NULL;
		}
	} else if (dir == BT_AUDIO_DIR_SOURCE && !err) {
		if (valid_codec_cap_check(unicast_server->source_codec_cap,
					  temp_cap[temp_cap_index].num_caps, BT_AUDIO_DIR_SOURCE,
					  idx.lvl3)) {
			bt_audio_codec_allocation_set(&lc3_preset_source.codec_cfg,
						      unicast_server->location);
		} else {
			LOG_WRN("No valid codec capability found for %s source",
				unicast_server->ch_name);
			unicast_server->source_ep = NULL;
		}
	}

	/* Free up the slot in temp_cap */
	memset(temp_cap[temp_cap_index].codec, 0, sizeof(temp_cap[temp_cap_index].codec));
	temp_cap[temp_cap_index].conn = NULL;
	temp_cap[temp_cap_index].num_caps = 0;

	if (dir == BT_AUDIO_DIR_SINK) {
		unicast_server->waiting_for_sink_disc = false;

		if (unicast_server->waiting_for_source_disc) {
			ret = bt_bap_unicast_client_discover(conn, BT_AUDIO_DIR_SOURCE);
			if (ret) {
				LOG_WRN("Failed to discover source: %d", ret);
			}

			return;
		}
	} else if (dir == BT_AUDIO_DIR_SOURCE) {
		unicast_server->waiting_for_source_disc = false;
	}

	if (!playing_state) {
		/* Since we are not in a playing state we return before starting the new streams */
		return;
	}

	ret = k_msgq_put(&cap_start_msgq, &idx, K_NO_WAIT);
	if (ret) {
		LOG_ERR("Failed to put device_index on the queue: %d", ret);
		return;
	}

	k_work_submit(&cap_start_work);
}

#if (CONFIG_BT_AUDIO_TX)
static void stream_sent_cb(struct bt_bap_stream *stream)
{
	int ret;
	struct stream_index idx;
	uint8_t state;

	ret = le_audio_ep_state_get(stream->ep, &state);
	if (ret) {
		return;
	}

	if (state == BT_BAP_EP_STATE_STREAMING) {

		ret = device_index_get(stream->conn, &idx);
		if (ret) {
			LOG_ERR("Device index not found");
		} else {
			ERR_CHK(bt_le_audio_tx_stream_sent(idx));
		}
	} else {
		LOG_WRN("Not in streaming state: %d", state);
	}
}
#endif /* CONFIG_BT_AUDIO_TX */

static void check_and_update_pd_in_group(struct stream_index idx, uint32_t new_pres_dly_us)
{
	int ret;

	for (int i = 0; i < ARRAY_SIZE(unicast_servers[idx.lvl1][idx.lvl2]); i++) {
		if (i != idx.lvl3 && unicast_servers[idx.lvl1][idx.lvl2][i].device_conn != NULL) {
			ret = update_cap_sink_stream_qos(&unicast_servers[idx.lvl1][idx.lvl2][i],
							 new_pres_dly_us);
			if (ret && ret != -ESRCH) {
				/* TODO: Fix OCT-3111 and then turn the WRN to ERR */
				LOG_WRN("Presentation delay not set for %s "
					"device: %d",
					unicast_servers[idx.lvl1][idx.lvl2][i].ch_name, ret);
			}
		}
	}

	LOG_DBG("Set %s, connection %p, stream %p",
		unicast_servers[idx.lvl1][idx.lvl2][idx.lvl3].ch_name,
		(void *)&unicast_servers[idx.lvl1][idx.lvl2][idx.lvl3].device_conn,
		(void *)&unicast_servers[idx.lvl1][idx.lvl2][idx.lvl3].cap_sink_stream.bap_stream);

	unicast_servers[idx.lvl1][idx.lvl2][idx.lvl3].cap_sink_stream.bap_stream.qos->pd =
		new_pres_dly_us;
}

static void stream_configured_cb(struct bt_bap_stream *stream,
				 const struct bt_audio_codec_qos_pref *pref)
{
	int ret;
	uint32_t new_pres_dly_us;
	enum bt_audio_dir dir;
	struct stream_index idx;

	ret = device_index_get(stream->conn, &idx);
	if (ret) {
		LOG_ERR("Unknown connection, should not reach here");
		return;
	}

	struct le_audio_unicast_server *unicast_server =
		&unicast_servers[idx.lvl1][idx.lvl2][idx.lvl3];

	dir = le_audio_stream_dir_get(stream);
	if (dir <= 0) {
		LOG_ERR("Failed to get dir of stream %p", (void *)stream);
		return;
	}

	if (dir == BT_AUDIO_DIR_SINK) {
		/* NOTE: The string below is used by the Nordic CI system */
		LOG_INF("%s sink stream configured", unicast_server->ch_name);
		le_audio_print_codec(unicast_server->cap_sink_stream.bap_stream.codec_cfg, dir);
	} else if (dir == BT_AUDIO_DIR_SOURCE) {
		LOG_INF("%s source stream configured", unicast_server->ch_name);
		le_audio_print_codec(unicast_server->cap_source_stream.bap_stream.codec_cfg, dir);
	} else {
		LOG_ERR("Endpoint direction not recognized: %d", dir);
		return;
	}
	LOG_DBG("Configured Stream info: %s, %p, dir %d", unicast_server->ch_name, (void *)stream,
		dir);

	ret = device_pres_delay_find(idx, &new_pres_dly_us);
	if (ret) {
		LOG_ERR("Cannot get a valid presentation delay");
		return;
	}

	if (unicast_server->waiting_for_source_disc) {
		return;
	}

	if (le_audio_ep_state_check(unicast_server->cap_sink_stream.bap_stream.ep,
				    BT_BAP_EP_STATE_CODEC_CONFIGURED)) {
		check_and_update_pd_in_group(idx, new_pres_dly_us);
	}

	le_audio_event_publish(LE_AUDIO_EVT_CONFIG_RECEIVED, stream->conn, dir);

	/* Make sure both sink and source ep (if both are discovered) are configured before
	 * QoS
	 */
	if ((unicast_server->sink_ep != NULL &&
	     !le_audio_ep_state_check(unicast_server->cap_sink_stream.bap_stream.ep,
				      BT_BAP_EP_STATE_CODEC_CONFIGURED)) ||
	    (unicast_server->source_ep != NULL &&
	     !le_audio_ep_state_check(unicast_server->cap_source_stream.bap_stream.ep,
				      BT_BAP_EP_STATE_CODEC_CONFIGURED))) {
		return;
	}
}

static void stream_qos_set_cb(struct bt_bap_stream *stream)
{
	int ret;
	struct stream_index idx;

	LOG_DBG("QoS set cb");

	ret = device_index_get(stream->conn, &idx);
	if (ret) {
		LOG_ERR("Unknown connection, should not reach here");
		return;
	}

	struct le_audio_unicast_server *unicast_server =
		&unicast_servers[idx.lvl1][idx.lvl2][idx.lvl3];

	if (unicast_server->qos_reconfigure) {
		LOG_DBG("Reconfiguring: %s to PD: %d", unicast_server->ch_name,
			unicast_server->reconfigure_pd);

		unicast_server->qos_reconfigure = false;
		unicast_server->cap_sink_stream.bap_stream.qos->pd = unicast_server->reconfigure_pd;
	} else {
		LOG_DBG("Set %s to PD: %d", unicast_server->ch_name, stream->qos->pd);
	}
}

static void stream_enabled_cb(struct bt_bap_stream *stream)
{
	LOG_DBG("Stream enabled: %p", (void *)stream);
}

static void stream_started_cb(struct bt_bap_stream *stream)
{
	int ret;
	enum bt_audio_dir dir;

	dir = le_audio_stream_dir_get(stream);
	if (dir <= 0) {
		LOG_ERR("Failed to get dir of stream %p", (void *)stream);
		return;
	}

	if (IS_ENABLED(CONFIG_BT_AUDIO_TX)) {
		struct stream_index idx;

		ret = device_index_get(stream->conn, &idx);
		if (ret) {
			LOG_ERR("Device index not found");
		} else {
			ERR_CHK(bt_le_audio_tx_stream_started(idx));
		}
	}

	/* NOTE: The string below is used by the Nordic CI system */
	LOG_INF("Stream %p started", (void *)stream);

	le_audio_event_publish(LE_AUDIO_EVT_STREAMING, stream->conn, dir);
}

static void stream_metadata_updated_cb(struct bt_bap_stream *stream)
{
	LOG_DBG("Audio Stream %p metadata updated", (void *)stream);
}

static void stream_disabled_cb(struct bt_bap_stream *stream)
{
	LOG_DBG("Audio Stream %p disabled", (void *)stream);
}

static void stream_stopped_cb(struct bt_bap_stream *stream, uint8_t reason)
{
	/* NOTE: The string below is used by the Nordic CI system */
	LOG_INF("Stream %p stopped. Reason %d", (void *)stream, reason);

	/* Check if the other streams are streaming, send event if not */
	for (int i = 0; i < CONFIG_BT_ISO_MAX_CIG; i++) {
		for (int j = 0; j < ARRAY_SIZE(unicast_servers[i][0]); j++) {
			if (le_audio_ep_state_check(
				    unicast_servers[i][0][j].cap_sink_stream.bap_stream.ep,
				    BT_BAP_EP_STATE_STREAMING) ||
			    le_audio_ep_state_check(
				    unicast_servers[i][0][j].cap_source_stream.bap_stream.ep,
				    BT_BAP_EP_STATE_STREAMING)) {
				return;
			}
		}
	}

	le_audio_event_publish(LE_AUDIO_EVT_NOT_STREAMING, stream->conn, BT_AUDIO_DIR_SINK);
}

static void stream_released_cb(struct bt_bap_stream *stream)
{
	LOG_DBG("Audio Stream %p released", (void *)stream);

	/* Check if the other streams are streaming, send event if not */
	for (int i = 0; i < CONFIG_BT_ISO_MAX_CIG; i++) {
		for (int j = 0; j < ARRAY_SIZE(unicast_servers[i][0]); j++) {
			if (le_audio_ep_state_check(
				    unicast_servers[i][0][j].cap_sink_stream.bap_stream.ep,
				    BT_BAP_EP_STATE_STREAMING) ||
			    le_audio_ep_state_check(
				    unicast_servers[i][0][j].cap_source_stream.bap_stream.ep,
				    BT_BAP_EP_STATE_STREAMING)) {
				return;
			}
		}
	}

	le_audio_event_publish(LE_AUDIO_EVT_NOT_STREAMING, stream->conn, BT_AUDIO_DIR_SINK);
}

#if (CONFIG_BT_AUDIO_RX)
static void stream_recv_cb(struct bt_bap_stream *stream, const struct bt_iso_recv_info *info,
			   struct net_buf *buf)
{
	int ret;
	bool bad_frame = false;
	struct stream_index idx;

	if (receive_cb == NULL) {
		LOG_ERR("The RX callback has not been set");
		return;
	}

	if (!(info->flags & BT_ISO_FLAGS_VALID)) {
		bad_frame = true;
	}

	ret = device_index_get(stream->conn, &idx);
	if (ret) {
		LOG_ERR("Device index not found");
		return;
	}

	receive_cb(buf->data, buf->len, bad_frame, info->ts, idx.lvl3,
		   bt_audio_codec_cfg_get_octets_per_frame(stream->codec_cfg));
}
#endif /* (CONFIG_BT_AUDIO_RX) */

static struct bt_bap_stream_ops stream_ops = {
	.configured = stream_configured_cb,
	.qos_set = stream_qos_set_cb,
	.enabled = stream_enabled_cb,
	.started = stream_started_cb,
	.metadata_updated = stream_metadata_updated_cb,
	.disabled = stream_disabled_cb,
	.stopped = stream_stopped_cb,
	.released = stream_released_cb,
#if (CONFIG_BT_AUDIO_RX)
	.recv = stream_recv_cb,
#endif /* (CONFIG_BT_AUDIO_RX) */
#if (CONFIG_BT_AUDIO_TX)
	.sent = stream_sent_cb,
#endif /* (CONFIG_BT_AUDIO_TX) */
};

static struct bt_bap_unicast_client_cb unicast_client_cbs = {
	.location = unicast_client_location_cb,
	.available_contexts = available_contexts_cb,
	.pac_record = pac_record_cb,
	.endpoint = endpoint_cb,
	.discover = discover_cb,
};

static void disconnected_cleanup(struct stream_index idx)
{
	struct le_audio_unicast_server *unicast_server =
		&unicast_servers[idx.lvl1][idx.lvl2][idx.lvl3];

	unicast_server->device_conn = NULL;
	unicast_server->sink_ep = NULL;
	memset(unicast_server->sink_codec_cap, 0, sizeof(unicast_server->sink_codec_cap));
	unicast_server->source_ep = NULL;
	memset(unicast_server->source_codec_cap, 0, sizeof(unicast_server->source_codec_cap));

	unicast_server->num_sink_eps = 0;
	unicast_server->num_source_eps = 0;
}

static void unicast_discovery_complete_cb(struct bt_conn *conn, int err,
					  const struct bt_csip_set_coordinator_set_member *member,
					  const struct bt_csip_set_coordinator_csis_inst *csis_inst)
{
	int ret;
	struct le_audio_msg msg;
	struct stream_index idx;

	ret = device_index_get(conn, &idx);
	if (ret) {
		LOG_ERR("Device not found");
		return;
	}

	if (err || csis_inst == NULL) {
		LOG_WRN("Got err: %d from conn: %p", err, (void *)conn);
		msg.set_size = 0;
		msg.sirk = NULL;
	} else {
		LOG_DBG("\tErr: %d, set_size: %d", err, csis_inst->info.set_size);
		LOG_HEXDUMP_DBG(csis_inst->info.sirk, BT_CSIP_SIRK_SIZE, "\tSIRK:");

		unicast_servers[idx.lvl1][idx.lvl2][idx.lvl3].member = member;
		msg.set_size = csis_inst->info.set_size;
		msg.sirk = csis_inst->info.sirk;
	}

	LOG_DBG("Unicast discovery complete cb");

	msg.event = LE_AUDIO_EVT_COORD_SET_DISCOVERED;
	msg.conn = conn;

	ret = zbus_chan_pub(&le_audio_chan, &msg, LE_AUDIO_ZBUS_EVENT_WAIT_TIME);
	ERR_CHK(ret);
}

static void unicast_start_complete_cb(int err, struct bt_conn *conn)
{
	int ret;
	struct stream_index idx;

	k_mutex_unlock(&mtx_cap_procedure_proceed);

	if (err) {
		LOG_WRN("Failed start_complete for conn: %p, err: %d", (void *)conn, err);
	}

	LOG_DBG("Unicast start complete cb");
	ret = k_msgq_peek(&cap_start_msgq, &idx);
	if (ret == 0) {
		/* Pending start procedure found, call k_work */
		k_work_submit(&cap_start_work);
	} else {
		LOG_DBG("No outstanding work");
	}
}

static void unicast_update_complete_cb(int err, struct bt_conn *conn)
{
	if (err) {
		LOG_WRN("Failed update_complete for conn: %p, err: %d", (void *)conn, err);
	}

	LOG_DBG("Unicast update complete cb");
}

static void unicast_stop_complete_cb(int err, struct bt_conn *conn)
{
	int ret;

	if (err) {
		LOG_WRN("Failed stop_complete for conn: %p, err: %d", (void *)conn, err);
	}

	LOG_DBG("Unicast stop complete cb");

	/* Check for reconfigurable sink streams */
	for (int i = 0; i < CONFIG_BT_ISO_MAX_CIG; i++) {
		for (int j = 0; j < ARRAY_SIZE(unicast_servers[i][0]); j++) {
			if (unicast_servers[i][0][j].qos_reconfigure && playing_state) {
				struct stream_index idx = {
					.lvl1 = i,
					.lvl2 = 0,
					.lvl3 = j,
				};
				ret = k_msgq_put(&cap_start_msgq, &idx, K_NO_WAIT);
				if (ret) {
					LOG_ERR("Failed to put device_index %d on the queue: %d", j,
						ret);
				}
				k_work_submit(&cap_start_work);
			}
		}
	}
}

static struct bt_cap_initiator_cb cap_cbs = {
	.unicast_discovery_complete = unicast_discovery_complete_cb,
	.unicast_start_complete = unicast_start_complete_cb,
	.unicast_update_complete = unicast_update_complete_cb,
	.unicast_stop_complete = unicast_stop_complete_cb,
};

int unicast_client_config_get(struct bt_conn *conn, enum bt_audio_dir dir, uint32_t *bitrate,
			      uint32_t *sampling_rate_hz)
{
	int ret;
	struct stream_index idx;

	if (conn == NULL) {
		LOG_ERR("No valid connection pointer received");
		return -EINVAL;
	}

	if (bitrate == NULL && sampling_rate_hz == NULL) {
		LOG_ERR("No valid pointers received");
		return -ENXIO;
	}

	ret = device_index_get(conn, &idx);
	if (ret) {
		LOG_WRN("No configured streams found");
		return ret;
	}

	struct le_audio_unicast_server *unicast_server =
		&unicast_servers[idx.lvl1][idx.lvl2][idx.lvl3];

	if (dir == BT_AUDIO_DIR_SINK) {
		if (unicast_server->cap_sink_stream.bap_stream.codec_cfg == NULL) {
			LOG_ERR("No codec found for the stream");

			return -ENXIO;
		}

		if (sampling_rate_hz != NULL) {
			ret = le_audio_freq_hz_get(
				unicast_server->cap_sink_stream.bap_stream.codec_cfg,
				sampling_rate_hz);
			if (ret) {
				LOG_ERR("Invalid sampling frequency: %d", ret);
				return -ENXIO;
			}
		}

		if (bitrate != NULL) {
			ret = le_audio_bitrate_get(
				unicast_server->cap_sink_stream.bap_stream.codec_cfg, bitrate);
			if (ret) {
				LOG_ERR("Unable to calculate bitrate: %d", ret);
				return -ENXIO;
			}
		}
	} else if (dir == BT_AUDIO_DIR_SOURCE) {
		if (unicast_server->cap_source_stream.bap_stream.codec_cfg == NULL) {
			LOG_ERR("No codec found for the stream");
			return -ENXIO;
		}

		if (sampling_rate_hz != NULL) {
			ret = le_audio_freq_hz_get(
				unicast_server->cap_source_stream.bap_stream.codec_cfg,
				sampling_rate_hz);
			if (ret) {
				LOG_ERR("Invalid sampling frequency: %d", ret);
				return -ENXIO;
			}
		}

		if (bitrate != NULL) {
			ret = le_audio_bitrate_get(
				unicast_server->cap_source_stream.bap_stream.codec_cfg, bitrate);
			if (ret) {
				LOG_ERR("Unable to calculate bitrate: %d", ret);
				return -ENXIO;
			}
		}
	}

	return 0;
}

void unicast_client_conn_disconnected(struct bt_conn *conn)
{
	int ret;
	struct stream_index idx;

	ret = device_index_get(conn, &idx);
	if (ret) {
		LOG_WRN("Unknown connection disconnected");
	} else {
		disconnected_cleanup(idx);
	}
}

int unicast_client_discover(struct bt_conn *conn, enum unicast_discover_dir dir)
{
	int ret;
	struct stream_index idx;

	ret = device_index_vacant_get(conn, &idx);
	if (ret) {
		return ret;
	}

	unicast_servers[idx.lvl1][idx.lvl2][idx.lvl3].device_conn = conn;

	ret = bt_cap_initiator_unicast_discover(conn);
	if (ret) {
		LOG_WRN("Failed to start cap discover: %d", ret);
		return ret;
	}

	if (dir & BT_AUDIO_DIR_SOURCE) {
		unicast_servers[idx.lvl1][idx.lvl2][idx.lvl3].waiting_for_source_disc = true;
	}

	if (dir & BT_AUDIO_DIR_SINK) {
		unicast_servers[idx.lvl1][idx.lvl2][idx.lvl3].waiting_for_sink_disc = true;
	}

	if (dir == UNICAST_SERVER_BIDIR) {
		/* If we need to discover both source and sink, do sink first */
		ret = bt_bap_unicast_client_discover(conn, BT_AUDIO_DIR_SINK);
		return ret;
	}

	ret = bt_bap_unicast_client_discover(conn, dir);
	return ret;
}

int unicast_client_start(uint8_t cig_index)
{
	int ret;
	struct bt_cap_unicast_audio_start_stream_param
		cap_stream_params[CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT +
				  CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT];
	static struct bt_cap_unicast_audio_start_param param;

	if (cig_index >= CONFIG_BT_ISO_MAX_CIG) {
		LOG_ERR("Trying to start CIG %d out of %d", cig_index, CONFIG_BT_ISO_MAX_CIG);
		return -EINVAL;
	}

	param.stream_params = cap_stream_params;
	param.count = 0;
	param.type = BT_CAP_SET_TYPE_AD_HOC;

	for (int i = 0; i < ARRAY_SIZE(unicast_servers[cig_index][0]); i++) {
		uint8_t state;

		ret = le_audio_ep_state_get(unicast_servers[cig_index][0][i].sink_ep, &state);
		if (ret) {
			continue;
		}

		if (state == BT_BAP_EP_STATE_IDLE) {
			/* Start all streams in the configured state */
			cap_stream_params[param.count].member.member =
				unicast_servers[cig_index][0][i].device_conn;
			cap_stream_params[param.count].stream =
				&unicast_servers[cig_index][0][i].cap_sink_stream;
			cap_stream_params[param.count].ep =
				unicast_servers[cig_index][0][i].sink_ep;
			cap_stream_params[param.count].codec_cfg = &lc3_preset_sink.codec_cfg;
			param.count++;
		} else {
			LOG_WRN("Found unicast_server[%d] with an endpoint not in IDLE state: %d",
				i, state);
		}

		ret = le_audio_ep_state_get(unicast_servers[cig_index][0][i].source_ep, &state);
		if (ret) {
			continue;
		}

		if (state == BT_BAP_EP_STATE_IDLE) {
			/* Start all streams in the configured state */
			cap_stream_params[param.count].member.member =
				unicast_servers[cig_index][0][i].device_conn;
			cap_stream_params[param.count].stream =
				&unicast_servers[cig_index][0][i].cap_source_stream;
			cap_stream_params[param.count].ep =
				unicast_servers[cig_index][0][i].source_ep;
			cap_stream_params[param.count].codec_cfg = &lc3_preset_source.codec_cfg;
			param.count++;
		} else {
			LOG_WRN("Found unicast_server[%d] with an endpoint not in IDLE state: %d",
				i, state);
		}
	}

	if (param.count > 0) {
		ret = bt_cap_initiator_unicast_audio_start(&param);
		if (ret) {
			LOG_ERR("Failed to start unicast audio: %d", ret);
			return ret;
		}

		playing_state = true;
	} else {
		return -EIO;
	}

	return 0;
}

int unicast_client_stop(uint8_t cig_index)
{
	int ret;
	struct bt_cap_stream *streams[CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT +
				      CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT];
	static struct bt_cap_unicast_audio_stop_param param;

	if (cig_index >= CONFIG_BT_ISO_MAX_CIG) {
		LOG_ERR("Trying to stop CIG %d out of %d", cig_index, CONFIG_BT_ISO_MAX_CIG);
		return -EINVAL;
	}

	param.streams = streams;
	param.count = 0;
	param.type = BT_CAP_SET_TYPE_AD_HOC;

	le_audio_event_publish(LE_AUDIO_EVT_NOT_STREAMING, NULL, 0);

	for (int i = 0; i < ARRAY_SIZE(unicast_servers[cig_index][0]); i++) {
		if (le_audio_ep_state_check(unicast_servers[cig_index][0][i].sink_ep,
					    BT_BAP_EP_STATE_STREAMING)) {
			/* Stop all sink streams currently in a streaming state */
			streams[param.count++] = &unicast_servers[cig_index][0][i].cap_sink_stream;
		}

		if (le_audio_ep_state_check(unicast_servers[cig_index][0][i].source_ep,
					    BT_BAP_EP_STATE_STREAMING)) {
			/* Stop all source streams currently in a streaming state */
			streams[param.count++] =
				&unicast_servers[cig_index][0][i].cap_source_stream;
		}
	}

	if (param.count > 0) {
		ret = bt_cap_initiator_unicast_audio_stop(&param);
		if (ret) {
			LOG_ERR("Failed to stop unicast audio: %d", ret);
			return ret;
		}

		playing_state = false;
	} else {
		return -EIO;
	}

	return 0;
}

int unicast_client_send(uint8_t cig_index, struct le_audio_encoded_audio enc_audio)
{
#if (CONFIG_BT_AUDIO_TX)
	int ret;
	uint8_t num_active_streams = 0;

	if (cig_index >= CONFIG_BT_ISO_MAX_CIG) {
		LOG_ERR("Trying to send to CIG %d out of %d", cig_index, CONFIG_BT_ISO_MAX_CIG);
		return -EINVAL;
	}

	struct le_audio_tx_info tx[ARRAY_SIZE(unicast_servers[cig_index][0])];

	for (int i = 0; i < ARRAY_SIZE(unicast_servers[cig_index][0]); i++) {
		/* Skip unicast_servers not in a streaming state */
		if (!le_audio_ep_state_check(
			    unicast_servers[cig_index][0][i].cap_sink_stream.bap_stream.ep,
			    BT_BAP_EP_STATE_STREAMING)) {
			continue;
		}

		/* Set cap stream pointer */
		tx[num_active_streams].cap_stream =
			&unicast_servers[cig_index][0][i].cap_sink_stream;

		/* Set index */
		tx[num_active_streams].idx.lvl1 = cig_index;
		tx[num_active_streams].idx.lvl2 = 0;
		tx[num_active_streams].idx.lvl3 = i;

		/* Set channel location */
		/* Both mono and left unicast_servers will receive left channel */
		tx[num_active_streams].audio_channel =
			(unicast_servers[cig_index][0][i].location == BT_AUDIO_LOCATION_FRONT_RIGHT)
				? AUDIO_CH_R
				: AUDIO_CH_L;

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
#endif /* (CONFIG_BT_AUDIO_TX) */

	return 0;
}

int unicast_client_disable(uint8_t cig_index)
{
	ARG_UNUSED(cig_index);

	return -ENOTSUP;
}

int unicast_client_enable(uint8_t cig_index, le_audio_receive_cb recv_cb)
{
	int ret;
	static bool initialized;

	if (cig_index >= CONFIG_BT_ISO_MAX_CIG) {
		LOG_ERR("Trying to enable CIG %d out of %d", cig_index, CONFIG_BT_ISO_MAX_CIG);
		return -EINVAL;
	}

	if (initialized) {
		LOG_WRN("Already initialized");
		return -EALREADY;
	}

	if (recv_cb == NULL) {
		LOG_ERR("Receive callback is NULL");
		return -EINVAL;
	}

	receive_cb = recv_cb;

	for (int i = 0; i < ARRAY_SIZE(unicast_servers[cig_index][0]); i++) {
		bt_cap_stream_ops_register(&unicast_servers[cig_index][0][i].cap_sink_stream,
					   &stream_ops);
		bt_cap_stream_ops_register(&unicast_servers[cig_index][0][i].cap_source_stream,
					   &stream_ops);
	}

	ret = bt_bap_unicast_client_register_cb(&unicast_client_cbs);
	if (ret) {
		LOG_ERR("Failed to register client callbacks: %d", ret);
		return ret;
	}

	ret = bt_cap_initiator_register_cb(&cap_cbs);
	if (ret) {
		LOG_ERR("Failed to register cap callbacks: %d", ret);
		return ret;
	}

	if (IS_ENABLED(CONFIG_BT_AUDIO_TX)) {
		bt_le_audio_tx_init();
	}

	initialized = true;

	return 0;
}
