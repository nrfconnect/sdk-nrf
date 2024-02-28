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
#include <zephyr/bluetooth/audio/pacs.h>
#include <zephyr/bluetooth/audio/csip.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>
#include <../subsys/bluetooth/audio/bap_iso.h>

/* TODO: Remove when a get_info function is implemented in host */
#include <../subsys/bluetooth/audio/bap_endpoint.h>

#include "macros_common.h"
#include "nrf5340_audio_common.h"
#include "bt_le_audio_tx.h"
#include "le_audio.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(unicast_client, CONFIG_UNICAST_CLIENT_LOG_LEVEL);

ZBUS_CHAN_DEFINE(le_audio_chan, struct le_audio_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
		 ZBUS_MSG_INIT(0));

#define HCI_ISO_BUF_ALLOC_PER_CHAN 2
#define CIS_CONN_RETRY_TIMES	   5
#define CIS_CONN_RETRY_DELAY_MS	   500

struct le_audio_headset {
	char *ch_name;
	uint8_t num_sink_eps;
	uint8_t num_source_eps;
	struct bt_bap_stream sink_stream;
	bool waiting_for_sink_disc;
	struct bt_bap_ep *sink_ep;
	struct bt_audio_codec_cap sink_codec_cap[CONFIG_CODEC_CAP_COUNT_MAX];
	struct bt_bap_stream source_stream;
	bool waiting_for_source_disc;
	struct bt_bap_ep *source_ep;
	struct bt_audio_codec_cap source_codec_cap[CONFIG_CODEC_CAP_COUNT_MAX];
	struct bt_conn *headset_conn;
	struct k_work_delayable stream_start_sink_work;
	struct k_work_delayable stream_start_source_work;
	bool qos_reconfigure;
	uint32_t reconfigure_pd;
};

struct discover_dir {
	struct bt_conn *conn;
	bool source;
	bool sink;
};

struct worker_data {
	uint8_t channel_index;
	enum bt_audio_dir dir;
	uint8_t retries;
} __aligned(4);

struct temp_cap_storage {
	struct bt_conn *conn;
	uint8_t num_caps;
	/* Must be the same size as sink_codec_cap and source_codec_cap */
	struct bt_audio_codec_cap codec[CONFIG_CODEC_CAP_COUNT_MAX];
};

static struct le_audio_headset headsets[CONFIG_BT_MAX_CONN];
static struct discover_dir discover_list[CONFIG_BT_MAX_CONN];

K_MSGQ_DEFINE(kwork_msgq, sizeof(struct worker_data),
	      2 * (CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT +
		   CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT),
	      sizeof(uint32_t));

static struct temp_cap_storage temp_cap[CONFIG_BT_MAX_CONN];

/* Make sure that we have at least one headset device per CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK */
BUILD_ASSERT(ARRAY_SIZE(headsets) >= CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT,
	     "We need to have at least one headset device per ASE SINK");

/* Make sure that we have at least one headset device per CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC */
BUILD_ASSERT(ARRAY_SIZE(headsets) >= CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT,
	     "We need to have at least one headset device per ASE SOURCE");

static le_audio_receive_cb receive_cb;

static struct bt_bap_unicast_group *unicast_group;

/* Used for group creation only */
static struct bt_bap_lc3_preset lc3_preset_max = BT_BAP_LC3_PRESET_CONFIGURABLE(
	BT_AUDIO_LOCATION_FRONT_LEFT, BT_AUDIO_CONTEXT_TYPE_MEDIA, CONFIG_LC3_BITRATE_MAX);

static struct bt_bap_lc3_preset lc3_preset_sink = BT_BAP_LC3_UNICAST_PRESET_NRF5340_AUDIO_SINK;
static struct bt_bap_lc3_preset lc3_preset_sink_48_4_1 =
	BT_BAP_LC3_UNICAST_PRESET_48_4_1(BT_AUDIO_LOCATION_ANY, (BT_AUDIO_CONTEXT_TYPE_MEDIA));
static struct bt_bap_lc3_preset lc3_preset_sink_24_2_1 =
	BT_BAP_LC3_UNICAST_PRESET_24_2_1(BT_AUDIO_LOCATION_ANY, (BT_AUDIO_CONTEXT_TYPE_MEDIA));
static struct bt_bap_lc3_preset lc3_preset_sink_16_2_1 =
	BT_BAP_LC3_UNICAST_PRESET_16_2_1(BT_AUDIO_LOCATION_ANY, (BT_AUDIO_CONTEXT_TYPE_MEDIA));

static struct bt_bap_lc3_preset lc3_preset_source = BT_BAP_LC3_UNICAST_PRESET_NRF5340_AUDIO_SOURCE;
static struct bt_bap_lc3_preset lc3_preset_source_48_4_1 =
	BT_BAP_LC3_UNICAST_PRESET_48_4_1(BT_AUDIO_LOCATION_ANY, BT_AUDIO_CONTEXT_TYPE_MEDIA);
static struct bt_bap_lc3_preset lc3_preset_source_24_2_1 =
	BT_BAP_LC3_UNICAST_PRESET_24_2_1(BT_AUDIO_LOCATION_ANY, BT_AUDIO_CONTEXT_TYPE_MEDIA);
static struct bt_bap_lc3_preset lc3_preset_source_16_2_1 =
	BT_BAP_LC3_UNICAST_PRESET_16_2_1(BT_AUDIO_LOCATION_ANY, BT_AUDIO_CONTEXT_TYPE_MEDIA);

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

/**
 * @brief	Add a conn pointer to the discover list.
 *
 * @param[in]	conn	Pointer to the connection to add.
 * @param[in]	dir	Directions to do discovery for.
 *
 * @retval	-ENOMEM	No more space for connections.
 * @retval	0	Successfully added connection.
 */
static int discover_list_add(struct bt_conn *conn, enum unicast_discover_dir dir)
{
	if (!(dir & BT_AUDIO_DIR_SOURCE) && !(dir & BT_AUDIO_DIR_SINK)) {
		LOG_ERR("No direction is set for the discover list entry");
		return -EINVAL;
	}

	/* Check if conn is already in the list */
	for (int i = 0; i < ARRAY_SIZE(discover_list); i++) {
		if (discover_list[i].conn == conn) {
			discover_list[i].source = false;
			discover_list[i].sink = false;

			if (dir & BT_AUDIO_DIR_SOURCE) {
				discover_list[i].source = true;
			}

			if (dir & BT_AUDIO_DIR_SINK) {
				discover_list[i].sink = true;
			}

			return 0;
		}
	}

	/* If not already in the list, add to list if there is room */
	for (int i = 0; i < ARRAY_SIZE(discover_list); i++) {
		if (discover_list[i].conn == NULL) {
			discover_list[i].conn = conn;

			if (dir & BT_AUDIO_DIR_SOURCE) {
				discover_list[i].source = true;
			}

			if (dir & BT_AUDIO_DIR_SINK) {
				discover_list[i].sink = true;
			}

			return 0;
		}
	}

	return -ENOMEM;
}

/**
 * @brief	Remove a conn pointer from the discover list.
 *
 * @param[in]	conn	Pointer to the connection to remove.
 *
 * @retval	-EINVAL	No such connection found.
 * @retval	0	Successfully removed connection.
 */
static int discover_list_remove(struct bt_conn const *const conn)
{
	for (int i = 0; i < ARRAY_SIZE(discover_list); i++) {
		if (discover_list[i].conn == conn) {
			discover_list[i].conn = NULL;
			discover_list[i].sink = false;
			discover_list[i].source = false;

			return 0;
		}
	}

	return -EINVAL;
}

/**
 * @brief	Get discover_list index for given @p conn.
 *
 * @param[in]	conn	Pointer to the connection.
 *
 * @return	-ESRCH if @p conn not found, index otherwise.
 */
static int discover_list_index_get(struct bt_conn const *const conn)
{
	for (int i = 0; i < ARRAY_SIZE(discover_list); i++) {
		if (conn == discover_list[i].conn) {
			return i;
		}
	}

	return -ESRCH;
}

/**
 * @brief  Get the common presentation delay for all headsets.
 *
 * @param[in]	index		The index of the headset to test against.
 * @param[out]	pres_dly_us	Pointer to where the presentation delay will be stored.
 *
 * @retval	0	Operation successful.
 * @retval	-EINVAL	Any error.
 */
static int headset_pres_delay_find(uint8_t index, uint32_t *pres_dly_us)
{
	uint32_t pres_dly_min = headsets[index].sink_ep->qos_pref.pd_min;
	uint32_t pres_dly_max = headsets[index].sink_ep->qos_pref.pd_max;
	uint32_t pref_dly_min = headsets[index].sink_ep->qos_pref.pref_pd_min;
	uint32_t pref_dly_max = headsets[index].sink_ep->qos_pref.pref_pd_max;

	LOG_DBG("Index: %d, Pref min: %d, pref max: %d, pres_min: %d, pres_max: %d", index,
		pref_dly_min, pref_dly_max, pres_dly_min, pres_dly_max);

	for (int i = 0; i < ARRAY_SIZE(headsets); i++) {
		if (headsets[i].sink_ep != NULL) {
			pres_dly_min = MAX(pres_dly_min, headsets[i].sink_ep->qos_pref.pd_min);
			pres_dly_max = MIN(pres_dly_max, headsets[i].sink_ep->qos_pref.pd_max);
			pref_dly_min = MAX(pref_dly_min, headsets[i].sink_ep->qos_pref.pref_pd_min);
			pref_dly_max = MIN(pref_dly_max, headsets[i].sink_ep->qos_pref.pref_pd_max);
		}
	}

	if (IS_ENABLED(CONFIG_BT_AUDIO_PRES_DELAY_SRCH_MIN)) {
		*pres_dly_us = pres_dly_min;

		return 0;
	}

	if (IS_ENABLED(CONFIG_BT_AUDIO_PRES_DELAY_SRCH_MAX)) {
		*pres_dly_us = pres_dly_max;

		return 0;
	}

	if (IS_ENABLED(CONFIG_BT_AUDIO_PRES_DELAY_SRCH_PREF_MIN)) {
		/* Preferred min is 0, so we set min supported */
		if (pref_dly_min == 0) {
			*pres_dly_us = pres_dly_min;
		} else {
			*pres_dly_us = pref_dly_min;
		}

		return 0;
	}

	if (IS_ENABLED(CONFIG_BT_AUDIO_PRES_DELAY_SRCH_PREF_MAX)) {
		/* Preferred max is 0, so we set max supported */
		if (pref_dly_max == 0) {
			*pres_dly_us = pres_dly_max;
		} else {
			*pres_dly_us = pref_dly_max;
		}

		return 0;
	}

	LOG_ERR("Trying to use unrecognized search mode");

	return -EINVAL;
}

/**
 * @brief	Get channel index based on connection.
 *
 * @param[in]	conn	The connection to search for.
 * @param[out]	index	The channel index.
 *
 * @retval	0	Operation successful.
 * @retval	-EINVAL	There is no match.
 */
static int channel_index_get(const struct bt_conn *conn, uint8_t *index)
{
	if (conn == NULL) {
		LOG_ERR("No connection provided");
		return -EINVAL;
	}

	for (int i = 0; i < ARRAY_SIZE(headsets); i++) {
		if (headsets[i].headset_conn == conn) {
			*index = i;
			return 0;
		}
	}

	LOG_WRN("Connection not found");

	return -EINVAL;
}

static void supported_sample_rates_print(uint16_t supported_sample_rates, enum bt_audio_dir dir)
{
	char supported_str[20] = "";

	if (supported_sample_rates & BT_AUDIO_CODEC_LC3_FREQ_48KHZ) {
		strcat(supported_str, "48, ");
	}

	if (supported_sample_rates & BT_AUDIO_CODEC_LC3_FREQ_24KHZ) {
		strcat(supported_str, "24, ");
	}

	if (supported_sample_rates & BT_AUDIO_CODEC_LC3_FREQ_16KHZ) {
		strcat(supported_str, "16, ");
	}

	if (dir == BT_AUDIO_DIR_SINK) {
		LOG_DBG("Headset supports: %s kHz in sink direction", supported_str);
	} else if (dir == BT_AUDIO_DIR_SOURCE) {
		LOG_DBG("Headset supports: %s kHz in source direction", supported_str);
	}
}

static bool sink_parse_cb(struct bt_data *data, void *user_data)
{
	if (data->type == BT_AUDIO_CODEC_LC3_FREQ) {
		uint16_t lc3_freq_bit = sys_get_le16(data->data);

		supported_sample_rates_print(lc3_freq_bit, BT_AUDIO_DIR_SINK);

		/* Try with the preferred sample rate first */
		switch (CONFIG_BT_AUDIO_PREF_SAMPLE_RATE_VALUE) {
		case BT_AUDIO_CODEC_CONFIG_LC3_FREQ_48KHZ:
			if (lc3_freq_bit & BT_AUDIO_CODEC_LC3_FREQ_48KHZ) {
				lc3_preset_sink = lc3_preset_sink_48_4_1;
				*(bool *)user_data = true;
				/* Found what we were looking for, stop parsing LTV */
				return false;
			}

			break;

		case BT_AUDIO_CODEC_CONFIG_LC3_FREQ_24KHZ:
			if (lc3_freq_bit & BT_AUDIO_CODEC_LC3_FREQ_24KHZ) {
				lc3_preset_sink = lc3_preset_sink_24_2_1;
				*(bool *)user_data = true;
				/* Found what we were looking for, stop parsing LTV */
				return false;
			}

			break;

		case BT_AUDIO_CODEC_CONFIG_LC3_FREQ_16KHZ:
			if (lc3_freq_bit & BT_AUDIO_CODEC_LC3_FREQ_16KHZ) {
				lc3_preset_sink = lc3_preset_sink_16_2_1;
				*(bool *)user_data = true;
				/* Found what we were looking for, stop parsing LTV */
				return false;
			}

			break;
		}

		/* If no match with the preferred, revert to trying highest first */
		if (lc3_freq_bit & BT_AUDIO_CODEC_LC3_FREQ_48KHZ) {
			lc3_preset_sink = lc3_preset_sink_48_4_1;
			*(bool *)user_data = true;
		} else if (lc3_freq_bit & BT_AUDIO_CODEC_LC3_FREQ_24KHZ) {
			lc3_preset_sink = lc3_preset_sink_24_2_1;
			*(bool *)user_data = true;
		} else if (lc3_freq_bit & BT_AUDIO_CODEC_LC3_FREQ_16KHZ) {
			lc3_preset_sink = lc3_preset_sink_16_2_1;
			*(bool *)user_data = true;
		}

		/* Found what we were looking for, stop parsing LTV */
		return false;
	}

	/* Did not find what we were looking for, continue parsing LTV */
	return true;
}

static bool source_parse_cb(struct bt_data *data, void *user_data)
{
	if (data->type == BT_AUDIO_CODEC_LC3_FREQ) {
		uint16_t lc3_freq_bit = sys_get_le16(data->data);

		supported_sample_rates_print(lc3_freq_bit, BT_AUDIO_DIR_SOURCE);

		/* Try with the preferred sample rate first */
		switch (CONFIG_BT_AUDIO_PREF_SAMPLE_RATE_VALUE) {
		case BT_AUDIO_CODEC_CONFIG_LC3_FREQ_48KHZ:
			if (lc3_freq_bit & BT_AUDIO_CODEC_LC3_FREQ_48KHZ) {
				lc3_preset_source = lc3_preset_source_48_4_1;
				*(bool *)user_data = true;
				/* Found what we were looking for, stop parsing LTV */
				return false;
			}

			break;

		case BT_AUDIO_CODEC_CONFIG_LC3_FREQ_24KHZ:
			if (lc3_freq_bit & BT_AUDIO_CODEC_LC3_FREQ_24KHZ) {
				lc3_preset_source = lc3_preset_source_24_2_1;
				*(bool *)user_data = true;
				/* Found what we were looking for, stop parsing LTV */
				return false;
			}

			break;

		case BT_AUDIO_CODEC_CONFIG_LC3_FREQ_16KHZ:
			if (lc3_freq_bit & BT_AUDIO_CODEC_LC3_FREQ_16KHZ) {
				lc3_preset_source = lc3_preset_source_16_2_1;
				*(bool *)user_data = true;
				/* Found what we were looking for, stop parsing LTV */
				return false;
			}

			break;
		}

		/* If no match with the preferred, revert to trying highest first */
		if (lc3_freq_bit & BT_AUDIO_CODEC_LC3_FREQ_48KHZ) {
			lc3_preset_source = lc3_preset_source_48_4_1;
			*(bool *)user_data = true;
		} else if (lc3_freq_bit & BT_AUDIO_CODEC_LC3_FREQ_24KHZ) {
			lc3_preset_source = lc3_preset_source_24_2_1;
			*(bool *)user_data = true;
		} else if (lc3_freq_bit & BT_AUDIO_CODEC_LC3_FREQ_16KHZ) {
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
 * @brief	Check if the gateway can support the headset codec capabilities.
 *
 * @note	Currently only the sampling frequency is checked.
 *
 * @param[in]	cap_array	The array of pointers to codec capabilities.
 * @param[in]	num_caps	The size of cap_array.
 * @param[in]	dir		Direction of the capabilities to check.
 *
 * @return	True if valid codec capability found, false otherwise.
 */
static bool valid_codec_cap_check(struct bt_audio_codec_cap cap_array[], uint8_t num_caps,
				  enum bt_audio_dir dir)
{
	bool valid_result = false;

	/* Only the sampling frequency is checked */
	if (dir == BT_AUDIO_DIR_SINK) {
		for (int i = 0; i < num_caps; i++) {
			(void)bt_audio_data_parse(cap_array[i].data, cap_array[i].data_len,
						  sink_parse_cb, &valid_result);
		}
	} else if (dir == BT_AUDIO_DIR_SOURCE) {
		for (int i = 0; i < num_caps; i++) {
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

		if (type == BT_AUDIO_CODEC_CONFIG_LC3_CHAN_ALLOC) {
			const uint32_t loc_32 = loc;

			sys_put_le32(loc_32, value);

			return;
		}
		i += value_len;
	}
}

static int update_sink_stream_qos(struct le_audio_headset *headset, uint32_t pres_delay_us)
{
	int ret;

	if (headset->sink_stream.ep == NULL) {
		return -ESRCH;
	}

	if (headset->sink_stream.qos == NULL) {
		LOG_WRN("No QoS found for %p", &headset->sink_stream);
		return -ENXIO;
	}

	if (headset->sink_stream.qos->pd != pres_delay_us) {
		if (playing_state &&
		    le_audio_ep_state_check(headset->sink_stream.ep, BT_BAP_EP_STATE_STREAMING)) {
			LOG_DBG("Update streaming %s headset, connection %p, stream %p",
				headset->ch_name, &headset->headset_conn, &headset->sink_stream);

			headset->qos_reconfigure = true;
			headset->reconfigure_pd = pres_delay_us;

			ret = bt_bap_stream_disable(&headset->sink_stream);
			if (ret) {
				LOG_ERR("Unable to disable stream: %d", ret);
				return ret;
			}
		} else {
			LOG_DBG("Reset %s headset, connection %p, stream %p", headset->ch_name,
				&headset->headset_conn, &headset->sink_stream);

			headset->sink_stream.qos->pd = pres_delay_us;

			ret = bt_bap_stream_qos(headset->headset_conn, unicast_group);
			if (ret && ret != -EINVAL) {
				/* ret == -EINVAL means that the stream is not ready to
				 * be configured yet
				 */
				LOG_ERR("Unable to configure %s headset: %d", headset->ch_name,
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
	int index = discover_list_index_get(conn);

	if (index < 0) {
		LOG_WRN("Failed to get discover_list index");
		return;
	}

	if ((loc & BT_AUDIO_LOCATION_FRONT_LEFT) || (loc & BT_AUDIO_LOCATION_SIDE_LEFT)) {
		if (headsets[AUDIO_CH_L].headset_conn == NULL) {
			headsets[AUDIO_CH_L].headset_conn = conn;
			headsets[AUDIO_CH_L].waiting_for_sink_disc = discover_list[index].sink;
			headsets[AUDIO_CH_L].waiting_for_source_disc = discover_list[index].source;
		}

	} else if ((loc & BT_AUDIO_LOCATION_FRONT_RIGHT) || (loc & BT_AUDIO_LOCATION_SIDE_RIGHT)) {
		if (headsets[AUDIO_CH_R].headset_conn == NULL) {
			headsets[AUDIO_CH_R].headset_conn = conn;
			headsets[AUDIO_CH_R].waiting_for_sink_disc = discover_list[index].sink;
			headsets[AUDIO_CH_R].waiting_for_source_disc = discover_list[index].source;
		}
	} else {
		LOG_WRN("Channel location not supported");
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
		LOG_WRN("No more space. Increase CODEC_CAPAB_COUNT_MAX");
	}
}

static void endpoint_cb(struct bt_conn *conn, enum bt_audio_dir dir, struct bt_bap_ep *ep)
{
	int ret;
	uint8_t channel_index = 0;

	ret = channel_index_get(conn, &channel_index);
	if (ret) {
		LOG_ERR("Unknown connection, should not reach here");
		return;
	}

	if (dir == BT_AUDIO_DIR_SINK) {
		if (ep != NULL) {
			if (headsets[channel_index].num_sink_eps > 0) {
				LOG_WRN("More than one sink endpoint found, idx 0 is used "
					"by default");
				return;
			}

			headsets[channel_index].sink_ep = ep;
			headsets[channel_index].num_sink_eps++;
			return;
		}

		if (headsets[channel_index].sink_ep == NULL) {
			LOG_WRN("No sink endpoints found");
		}

		return;
	} else if (dir == BT_AUDIO_DIR_SOURCE) {
		if (ep != NULL) {
			if (headsets[channel_index].num_source_eps > 0) {
				LOG_WRN("More than one source endpoint found, idx 0 is "
					"used by default");
				return;
			}

			headsets[channel_index].source_ep = ep;
			headsets[channel_index].num_source_eps++;
			return;
		}

		if (headsets[channel_index].source_ep == NULL) {
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
	uint8_t channel_index;
	uint8_t temp_cap_index;

	ret = channel_index_get(conn, &channel_index);
	if (ret) {
		LOG_ERR("Unknown connection, should not reach here");
		return;
	}

	if (err == BT_ATT_ERR_ATTRIBUTE_NOT_FOUND) {
		if (dir == BT_AUDIO_DIR_SINK) {
			LOG_WRN("No sinks found");
			headsets[channel_index].waiting_for_sink_disc = false;
		} else if (dir == BT_AUDIO_DIR_SOURCE) {
			LOG_WRN("No sources found");
			headsets[channel_index].waiting_for_source_disc = false;
			/**
			 * We usually wait until both sink and source has been discovered
			 * before configuring, but since no source was found and we have a
			 * sink, we need to configure that.
			 */
			if (headsets[channel_index].sink_ep != NULL) {
				ret = bt_bap_stream_config(conn,
							   &headsets[channel_index].sink_stream,
							   headsets[channel_index].sink_ep,
							   &lc3_preset_sink.codec_cfg);
				if (ret) {
					LOG_ERR("Could not configure sink stream, ret = %d", ret);
				}
			}
		}

		return;
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
		if (dir == BT_AUDIO_DIR_SINK) {
			memcpy(&headsets[channel_index].sink_codec_cap[i],
			       &temp_cap[temp_cap_index].codec[i],
			       sizeof(struct bt_audio_codec_cap));
		} else if (dir == BT_AUDIO_DIR_SOURCE) {
			memcpy(&headsets[channel_index].source_codec_cap[i],
			       &temp_cap[temp_cap_index].codec[i],
			       sizeof(struct bt_audio_codec_cap));
		}
	}

	if (dir == BT_AUDIO_DIR_SINK) {
		if (valid_codec_cap_check(headsets[channel_index].sink_codec_cap,
					  temp_cap[temp_cap_index].num_caps, BT_AUDIO_DIR_SINK)) {
			if (conn == headsets[AUDIO_CH_L].headset_conn) {
				bt_audio_codec_allocation_set(&lc3_preset_sink.codec_cfg,
							      BT_AUDIO_LOCATION_FRONT_LEFT);
			} else if (conn == headsets[AUDIO_CH_R].headset_conn) {
				bt_audio_codec_allocation_set(&lc3_preset_sink.codec_cfg,
							      BT_AUDIO_LOCATION_FRONT_RIGHT);
			} else {
				LOG_ERR("Unknown connection, cannot set allocation");
			}
		} else {
			/* NOTE: The string below is used by the Nordic CI system */
			LOG_WRN("No valid codec capability found for %s headset sink",
				headsets[channel_index].ch_name);
			headsets[channel_index].sink_ep = NULL;
		}
	} else if (dir == BT_AUDIO_DIR_SOURCE) {
		if (valid_codec_cap_check(headsets[channel_index].source_codec_cap,
					  temp_cap[temp_cap_index].num_caps, BT_AUDIO_DIR_SOURCE)) {
			if (conn == headsets[AUDIO_CH_L].headset_conn) {
				bt_audio_codec_allocation_set(&lc3_preset_source.codec_cfg,
							      BT_AUDIO_LOCATION_FRONT_LEFT);
			} else if (conn == headsets[AUDIO_CH_R].headset_conn) {
				bt_audio_codec_allocation_set(&lc3_preset_source.codec_cfg,
							      BT_AUDIO_LOCATION_FRONT_RIGHT);
			} else {
				LOG_ERR("Unknown connection, cannot set allocation");
			}
		} else {
			LOG_WRN("No valid codec capability found for %s headset source",
				headsets[channel_index].ch_name);
			headsets[channel_index].source_ep = NULL;
		}
	}

	/* Free up the slot in temp_cap */
	memset(temp_cap[temp_cap_index].codec, 0, sizeof(temp_cap[temp_cap_index].codec));
	temp_cap[temp_cap_index].conn = NULL;
	temp_cap[temp_cap_index].num_caps = 0;

	if (dir == BT_AUDIO_DIR_SINK) {
		headsets[channel_index].waiting_for_sink_disc = false;

		if (headsets[channel_index].waiting_for_source_disc) {
			ret = bt_bap_unicast_client_discover(conn, BT_AUDIO_DIR_SOURCE);
			if (ret) {
				LOG_WRN("Failed to discover source: %d", ret);
			}

			return;
		}
	} else if (dir == BT_AUDIO_DIR_SOURCE) {
		headsets[channel_index].waiting_for_source_disc = false;
	}

	if (headsets[channel_index].sink_ep) {
		ret = bt_bap_stream_config(conn, &headsets[channel_index].sink_stream,
					   headsets[channel_index].sink_ep,
					   &lc3_preset_sink.codec_cfg);
		if (ret) {
			LOG_ERR("Could not configure sink stream, ret = %d", ret);
		}
	}

	if (headsets[channel_index].source_ep) {
		ret = bt_bap_stream_config(conn, &headsets[channel_index].source_stream,
					   headsets[channel_index].source_ep,
					   &lc3_preset_source.codec_cfg);
		if (ret) {
			LOG_ERR("Could not configure source stream, ret = %d", ret);
		}
	}
}

static struct bt_bap_unicast_client_cb unicast_client_cbs = {
	.location = unicast_client_location_cb,
	.available_contexts = available_contexts_cb,
	.pac_record = pac_record_cb,
	.endpoint = endpoint_cb,
	.discover = discover_cb,
};

static void stream_sent_cb(struct bt_bap_stream *stream)
{
	int ret;
	uint8_t channel_index;

	if (le_audio_ep_state_check(stream->ep, BT_BAP_EP_STATE_STREAMING)) {

		ret = channel_index_get(stream->conn, &channel_index);
		if (ret) {
			LOG_ERR("Channel index not found");
		} else {
			ERR_CHK(bt_le_audio_tx_stream_sent(channel_index));
		}
	} else {
		LOG_WRN("Not in streaming state");
	}
}

static void stream_configured_cb(struct bt_bap_stream *stream,
				 const struct bt_audio_codec_qos_pref *pref)
{
	int ret;
	uint8_t channel_index;
	uint32_t new_pres_dly_us;

	ret = channel_index_get(stream->conn, &channel_index);
	if (ret) {
		LOG_ERR("Channel index not found");
		return;
	}

	if (stream->ep->dir == BT_AUDIO_DIR_SINK) {
		/* NOTE: The string below is used by the Nordic CI system */
		LOG_INF("%s sink stream configured", headsets[channel_index].ch_name);
	} else if (stream->ep->dir == BT_AUDIO_DIR_SOURCE) {
		LOG_INF("%s source stream configured", headsets[channel_index].ch_name);
	} else {
		LOG_WRN("Endpoint direction not recognized: %d", stream->ep->dir);
		return;
	}
	LOG_DBG("Configured Stream info: %s, %p, dir %d", headsets[channel_index].ch_name,
		(void *)stream, stream->ep->dir);

	ret = headset_pres_delay_find(channel_index, &new_pres_dly_us);
	if (ret) {
		LOG_ERR("Cannot get a valid presentation delay");
		return;
	}

	if (headsets[channel_index].waiting_for_source_disc) {
		return;
	}

	if (le_audio_ep_state_check(headsets[channel_index].sink_stream.ep,
				    BT_BAP_EP_STATE_CODEC_CONFIGURED)) {
		for (int i = 0; i < ARRAY_SIZE(headsets); i++) {
			if (i != channel_index && headsets[i].headset_conn != NULL) {
				ret = update_sink_stream_qos(&headsets[i], new_pres_dly_us);
				if (ret && ret != -ESRCH) {
					LOG_ERR("Presentation delay not set for %s "
						"headset: %d",
						headsets[channel_index].ch_name, ret);
				}
			}
		}

		LOG_DBG("Set %s headset, connection %p, stream %p", headsets[channel_index].ch_name,
			&headsets[channel_index].headset_conn,
			&headsets[channel_index].sink_stream);

		headsets[channel_index].sink_stream.qos->pd = new_pres_dly_us;
	}

	le_audio_event_publish(LE_AUDIO_EVT_CONFIG_RECEIVED, stream->conn, stream->ep->dir);

	/* Make sure both sink and source ep (if both are discovered) are configured before
	 * QoS
	 */
	if ((headsets[channel_index].sink_ep != NULL &&
	     !le_audio_ep_state_check(headsets[channel_index].sink_stream.ep,
				      BT_BAP_EP_STATE_CODEC_CONFIGURED)) ||
	    (headsets[channel_index].source_ep != NULL &&
	     !le_audio_ep_state_check(headsets[channel_index].source_stream.ep,
				      BT_BAP_EP_STATE_CODEC_CONFIGURED))) {
		return;
	}

	ret = bt_bap_stream_qos(headsets[channel_index].headset_conn, unicast_group);
	if (ret) {
		LOG_ERR("QoS not set for %s headset: %d", headsets[channel_index].ch_name, ret);
	}
}

static void stream_qos_set_cb(struct bt_bap_stream *stream)
{
	int ret;
	uint8_t channel_index;

	ret = channel_index_get(stream->conn, &channel_index);

	if (headsets[channel_index].qos_reconfigure) {
		LOG_DBG("Reconfiguring: %s to PD: %d", headsets[channel_index].ch_name,
			headsets[channel_index].reconfigure_pd);

		headsets[channel_index].qos_reconfigure = false;
		headsets[channel_index].sink_stream.qos->pd =
			headsets[channel_index].reconfigure_pd;

		ret = bt_bap_stream_qos(headsets[channel_index].headset_conn, unicast_group);
		if (ret) {
			LOG_ERR("Unable to reconfigure %s: %d", headsets[channel_index].ch_name,
				ret);
		}
	} else {
		LOG_DBG("Set %s to PD: %d", headsets[channel_index].ch_name, stream->qos->pd);

		if (playing_state) {
			ret = bt_bap_stream_enable(stream, lc3_preset_sink.codec_cfg.meta,
						   lc3_preset_sink.codec_cfg.meta_len);
			if (ret) {
				LOG_ERR("Unable to enable stream: %d", ret);
				return;
			}

			LOG_INF("Enable stream %p", stream);
		}
	}
}

static void stream_enabled_cb(struct bt_bap_stream *stream)
{
	int ret;
	uint8_t channel_index;
	struct worker_data work_data;
	enum bt_audio_dir dir;

	dir = le_audio_stream_dir_get(stream);
	if (dir <= 0) {
		LOG_ERR("Failed to get dir of stream %p", stream);
		return;
	}

	LOG_DBG("Stream enabled: %p", stream);

	ret = channel_index_get(stream->conn, &channel_index);
	if (ret) {
		LOG_ERR("Error getting channel index");
		return;
	}

	if (dir == BT_AUDIO_DIR_SINK &&
	    le_audio_ep_state_check(headsets[channel_index].sink_stream.ep,
				    BT_BAP_EP_STATE_ENABLING)) {
		if (!k_work_delayable_is_pending(&headsets[channel_index].stream_start_sink_work)) {
			work_data.channel_index = channel_index;
			work_data.retries = 0;
			work_data.dir = BT_AUDIO_DIR_SINK;

			LOG_DBG("k_msg_put: ch: %d, dir: %d, retries %d", work_data.channel_index,
				work_data.dir, work_data.retries);

			ret = k_msgq_put(&kwork_msgq, &work_data, K_NO_WAIT);
			if (ret) {
				LOG_ERR("No space in the queue for work_data");
				return;
			}

			k_work_schedule(&headsets[channel_index].stream_start_sink_work, K_NO_WAIT);
		}
	}

	if (dir == BT_AUDIO_DIR_SOURCE &&
	    le_audio_ep_state_check(headsets[channel_index].source_stream.ep,
				    BT_BAP_EP_STATE_ENABLING)) {
		if (!k_work_delayable_is_pending(
			    &headsets[channel_index].stream_start_source_work)) {
			work_data.channel_index = channel_index;
			work_data.retries = 0;
			work_data.dir = BT_AUDIO_DIR_SOURCE;

			LOG_DBG("k_msg_put: ch: %d, dir: %d, retries %d", work_data.channel_index,
				work_data.dir, work_data.retries);

			ret = k_msgq_put(&kwork_msgq, &work_data, K_NO_WAIT);
			if (ret) {
				LOG_ERR("No space in the queue for work_data");
				return;
			}

			k_work_schedule(&headsets[channel_index].stream_start_source_work,
					K_NO_WAIT);
		}
	}
}

static void stream_started_cb(struct bt_bap_stream *stream)
{
	int ret;
	uint8_t channel_index;
	enum bt_audio_dir dir;

	dir = le_audio_stream_dir_get(stream);
	if (dir <= 0) {
		LOG_ERR("Failed to get dir of stream %p", stream);
		return;
	}

	ret = channel_index_get(stream->conn, &channel_index);
	if (ret) {
		LOG_ERR("Channel index not found");
	} else {
		ERR_CHK(bt_le_audio_tx_stream_started(channel_index));
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
	int ret;
	uint8_t channel_index;

	/* NOTE: The string below is used by the Nordic CI system */
	LOG_INF("Stream %p stopped. Reason %d", (void *)stream, reason);

	ret = channel_index_get(stream->conn, &channel_index);
	if (ret) {
		LOG_ERR("Channel index not found");
	} else {
		ERR_CHK(bt_le_audio_tx_stream_stopped(channel_index));
	}

	/* Check if the other stream is streaming, send event if not */
	if (stream == &headsets[AUDIO_CH_L].sink_stream) {
		if (!le_audio_ep_state_check(headsets[AUDIO_CH_R].sink_stream.ep,
					     BT_BAP_EP_STATE_STREAMING)) {
			le_audio_event_publish(LE_AUDIO_EVT_NOT_STREAMING, stream->conn,
					       BT_AUDIO_DIR_SINK);
		}
	} else if (stream == &headsets[AUDIO_CH_R].sink_stream) {
		if (!le_audio_ep_state_check(headsets[AUDIO_CH_L].sink_stream.ep,
					     BT_BAP_EP_STATE_STREAMING)) {
			le_audio_event_publish(LE_AUDIO_EVT_NOT_STREAMING, stream->conn,
					       BT_AUDIO_DIR_SINK);
		}
	} else {
		LOG_WRN("Unknown stream");
	}
}

static void stream_released_cb(struct bt_bap_stream *stream)
{
	LOG_DBG("Audio Stream %p released", (void *)stream);

	/* Check if the other stream is streaming, send event if not */
	if (stream == &headsets[AUDIO_CH_L].sink_stream) {
		if (!le_audio_ep_state_check(headsets[AUDIO_CH_R].sink_stream.ep,
					     BT_BAP_EP_STATE_STREAMING)) {
			le_audio_event_publish(LE_AUDIO_EVT_NOT_STREAMING, stream->conn,
					       BT_AUDIO_DIR_SINK);
		}

		LOG_DBG("Left sink stream released");
	} else if (stream == &headsets[AUDIO_CH_R].sink_stream) {
		if (!le_audio_ep_state_check(headsets[AUDIO_CH_L].sink_stream.ep,
					     BT_BAP_EP_STATE_STREAMING)) {
			le_audio_event_publish(LE_AUDIO_EVT_NOT_STREAMING, stream->conn,
					       BT_AUDIO_DIR_SINK);
		}

		LOG_DBG("Right sink stream released");
	} else if (stream == &headsets[AUDIO_CH_L].source_stream) {
		LOG_DBG("Left source stream released");
	} else if (stream == &headsets[AUDIO_CH_R].source_stream) {
		LOG_DBG("Right source stream released");
	} else {
		LOG_WRN("Unknown stream");
	}
}

#if (CONFIG_BT_AUDIO_RX)
static void stream_recv_cb(struct bt_bap_stream *stream, const struct bt_iso_recv_info *info,
			   struct net_buf *buf)
{
	int ret;
	bool bad_frame = false;
	uint8_t channel_index;

	if (receive_cb == NULL) {
		LOG_ERR("The RX callback has not been set");
		return;
	}

	if (!(info->flags & BT_ISO_FLAGS_VALID)) {
		bad_frame = true;
	}

	ret = channel_index_get(stream->conn, &channel_index);
	if (ret) {
		LOG_ERR("Channel index not found");
		return;
	}

	receive_cb(buf->data, buf->len, bad_frame, info->ts, channel_index,
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

static void work_stream_start(struct k_work *work)
{
	int ret;
	struct worker_data work_data;

	ret = k_msgq_get(&kwork_msgq, &work_data, K_NO_WAIT);
	if (ret) {
		LOG_ERR("Cannot get info for start stream");
		return;
	}

	LOG_DBG("k_msg_get: ch: %d, dir: %d, retries %d", work_data.channel_index, work_data.dir,
		work_data.retries);

	if (work_data.dir == BT_AUDIO_DIR_SINK) {
		if (!le_audio_ep_state_check(headsets[work_data.channel_index].sink_stream.ep,
					     BT_BAP_EP_STATE_STREAMING)) {
			ret = bt_bap_stream_start(&headsets[work_data.channel_index].sink_stream);
		}
	} else if (work_data.dir == BT_AUDIO_DIR_SOURCE) {
		if (!le_audio_ep_state_check(headsets[work_data.channel_index].source_stream.ep,
					     BT_BAP_EP_STATE_STREAMING)) {
			ret = bt_bap_stream_start(&headsets[work_data.channel_index].source_stream);
		}
	} else {
		LOG_ERR("Trying to use unknown direction: %d", work_data.dir);
		le_audio_event_publish(LE_AUDIO_EVT_NO_VALID_CFG,
				       headsets[work_data.channel_index].headset_conn,
				       work_data.dir);

		return;
	}

	work_data.retries++;

	if ((ret == -EBUSY) && (work_data.retries < CIS_CONN_RETRY_TIMES)) {
		LOG_DBG("Got connect error from stream %d Retrying. code: %d count: %d",
			work_data.channel_index, ret, work_data.retries);
		LOG_DBG("k_msg_put: ch: %d, dir: %d, retries %d", work_data.channel_index,
			work_data.dir, work_data.retries);
		ret = k_msgq_put(&kwork_msgq, &work_data, K_NO_WAIT);
		if (ret) {
			LOG_ERR("No space in the queue for work_data");
			return;
		}
		/* Delay added to prevent controller overloading */
		if (work_data.dir == BT_AUDIO_DIR_SINK) {
			k_work_reschedule(&headsets[work_data.channel_index].stream_start_sink_work,
					  K_MSEC(CIS_CONN_RETRY_DELAY_MS));
		} else if (work_data.dir == BT_AUDIO_DIR_SOURCE) {
			k_work_reschedule(
				&headsets[work_data.channel_index].stream_start_source_work,
				K_MSEC(CIS_CONN_RETRY_DELAY_MS));
		}
	} else if (ret != 0) {
		LOG_WRN("Failed to establish CIS, ret = %d", ret);
		/** The connection could have invalid configs, or abnormal behavior cause the CIS
		 * failed to establish. Sending an event for triggering disconnection could clean up
		 * the abnormal state and restart the connection.
		 */
		le_audio_event_publish(LE_AUDIO_EVT_NO_VALID_CFG,
				       headsets[work_data.channel_index].headset_conn,
				       work_data.dir);
	} else if (k_msgq_peek(&kwork_msgq, &work_data) != -ENOMSG) {
		if (work_data.dir == BT_AUDIO_DIR_SINK &&
		    !k_work_delayable_is_pending(
			    &headsets[work_data.channel_index].stream_start_sink_work)) {
			k_work_reschedule(&headsets[work_data.channel_index].stream_start_sink_work,
					  K_MSEC(CIS_CONN_RETRY_DELAY_MS));
		} else if (work_data.dir == BT_AUDIO_DIR_SOURCE &&
			   !k_work_delayable_is_pending(
				   &headsets[work_data.channel_index].stream_start_source_work)) {
			k_work_reschedule(
				&headsets[work_data.channel_index].stream_start_source_work,
				K_MSEC(CIS_CONN_RETRY_DELAY_MS));
		}
	}
}

static void disconnected_headset_cleanup(uint8_t chan_idx)
{
	headsets[chan_idx].headset_conn = NULL;
	k_work_cancel_delayable(&headsets[chan_idx].stream_start_sink_work);
	headsets[chan_idx].sink_ep = NULL;
	memset(headsets[chan_idx].sink_codec_cap, 0, sizeof(headsets[chan_idx].sink_codec_cap));
	k_work_cancel_delayable(&headsets[chan_idx].stream_start_source_work);
	headsets[chan_idx].source_ep = NULL;
	memset(headsets[chan_idx].source_codec_cap, 0, sizeof(headsets[chan_idx].source_codec_cap));

	headsets[chan_idx].num_sink_eps = 0;
	headsets[chan_idx].num_source_eps = 0;
}

int unicast_client_config_get(struct bt_conn *conn, enum bt_audio_dir dir, uint32_t *bitrate,
			      uint32_t *sampling_rate_hz)
{
	int ret;
	uint8_t headset_idx;

	if (conn == NULL) {
		LOG_ERR("No valid connection pointer received");
		return -EINVAL;
	}

	if (bitrate == NULL && sampling_rate_hz == NULL) {
		LOG_ERR("No valid pointers received");
		return -ENXIO;
	}

	ret = channel_index_get(conn, &headset_idx);
	if (ret) {
		LOG_WRN("No configured streams found");
		return ret;
	}

	if (dir == BT_AUDIO_DIR_SINK) {
		if (headsets[headset_idx].sink_stream.codec_cfg == NULL) {
			LOG_ERR("No codec found for the stream");

			return -ENXIO;
		}

		if (sampling_rate_hz != NULL) {
			ret = le_audio_freq_hz_get(headsets[headset_idx].sink_stream.codec_cfg,
						   sampling_rate_hz);
			if (ret) {
				LOG_ERR("Invalid sampling frequency: %d", ret);
				return -ENXIO;
			}
		}

		if (bitrate != NULL) {
			ret = le_audio_bitrate_get(headsets[headset_idx].sink_stream.codec_cfg,
						   bitrate);
			if (ret) {
				LOG_ERR("Unable to calculate bitrate: %d", ret);
				return -ENXIO;
			}
		}
	} else if (dir == BT_AUDIO_DIR_SOURCE) {
		if (headsets[headset_idx].source_stream.codec_cfg == NULL) {
			LOG_ERR("No codec found for the stream");
			return -ENXIO;
		}

		if (sampling_rate_hz != NULL) {
			ret = le_audio_freq_hz_get(headsets[headset_idx].source_stream.codec_cfg,
						   sampling_rate_hz);
			if (ret) {
				LOG_ERR("Invalid sampling frequency: %d", ret);
				return -ENXIO;
			}
		}

		if (bitrate != NULL) {
			ret = le_audio_bitrate_get(headsets[headset_idx].source_stream.codec_cfg,
						   bitrate);
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
	uint8_t channel_index;

	/* Make sure discovery_list is purged for the disconnected conn */
	ret = discover_list_remove(conn);
	if (ret) {
		LOG_WRN("Failed to remove conn from discover_list: %d", ret);
	}

	ret = channel_index_get(conn, &channel_index);
	if (ret) {
		LOG_WRN("Unknown connection disconnected");
	} else {
		disconnected_headset_cleanup(channel_index);
	}
}

int unicast_client_discover(struct bt_conn *conn, enum unicast_discover_dir dir)
{
	int ret;

	ret = discover_list_add(conn, dir);
	if (ret) {
		LOG_ERR("Failed to add to discover_list: %d", ret);
		return ret;
	}

	if (dir == UNICAST_SERVER_BIDIR) {
		/* If we need to discover both source and sink, do sink first */
		ret = bt_bap_unicast_client_discover(conn, BT_AUDIO_DIR_SINK);
		return ret;
	}

	ret = bt_bap_unicast_client_discover(conn, dir);
	return ret;
}

int unicast_client_start(void)
{
	int ret_left = 0;
	int ret_right = 0;

	if (le_audio_ep_state_check(headsets[AUDIO_CH_L].sink_stream.ep,
				    BT_BAP_EP_STATE_QOS_CONFIGURED)) {
		ret_left = bt_bap_stream_enable(&headsets[AUDIO_CH_L].sink_stream,
						lc3_preset_sink.codec_cfg.meta,
						lc3_preset_sink.codec_cfg.meta_len);

		if (ret_left) {
			LOG_WRN("Failed to enable left stream: %d", ret_left);
		}
	}

	if (le_audio_ep_state_check(headsets[AUDIO_CH_R].sink_stream.ep,
				    BT_BAP_EP_STATE_QOS_CONFIGURED)) {
		ret_right = bt_bap_stream_enable(&headsets[AUDIO_CH_R].sink_stream,
						 lc3_preset_sink.codec_cfg.meta,
						 lc3_preset_sink.codec_cfg.meta_len);

		if (ret_right) {
			LOG_WRN("Failed to enable right stream: %d", ret_right);
		}
	}

	if (ret_left || ret_right) {
		return -EIO;
	}

	playing_state = true;

	return 0;
}

int unicast_client_stop(void)
{
	int ret_left = 0;
	int ret_right = 0;

	le_audio_event_publish(LE_AUDIO_EVT_NOT_STREAMING, NULL, 0);

	if (le_audio_ep_state_check(headsets[AUDIO_CH_L].sink_stream.ep,
				    BT_BAP_EP_STATE_STREAMING)) {
		ret_left = bt_bap_stream_disable(&headsets[AUDIO_CH_L].sink_stream);

		if (ret_left) {
			LOG_WRN("Failed to disable left stream: %d", ret_left);
		}
	}

	if (le_audio_ep_state_check(headsets[AUDIO_CH_R].sink_stream.ep,
				    BT_BAP_EP_STATE_STREAMING)) {
		ret_right = bt_bap_stream_disable(&headsets[AUDIO_CH_R].sink_stream);

		if (ret_right) {
			LOG_WRN("Failed to disable right stream: %d", ret_right);
		}
	}

	if (ret_left || ret_right) {
		return -EIO;
	}

	playing_state = false;

	return 0;
}

int unicast_client_send(struct le_audio_encoded_audio enc_audio)
{
	int ret;
#if CONFIG_BT_AUDIO_TX
	struct bt_bap_stream *bap_tx_streams[CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT];

	for (int i = 0; i < CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT; i++) {
		bap_tx_streams[i] = &headsets[i].sink_stream;
	}

	ret = bt_le_audio_tx_send(bap_tx_streams, enc_audio,
				  CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT);
	if (ret) {
		return ret;
	}
#endif /*CONFIG_BT_AUDIO_TX*/

	return 0;
}

int unicast_client_disable(void)
{
	return -ENOTSUP;
}

int unicast_client_enable(le_audio_receive_cb recv_cb)
{
	int ret;
	static bool initialized;
	int headset_iterator = 0;
	int stream_iterator = 0;
	struct bt_bap_unicast_group_stream_pair_param pair_params[ARRAY_SIZE(headsets)];
	/* 2 streams (one sink and one source stream) for each headset */
	struct bt_bap_unicast_group_stream_param group_stream_params[(ARRAY_SIZE(headsets) * 2)];
	struct bt_bap_unicast_group_param group_param;

	if (initialized) {
		LOG_WRN("Already initialized");
		return -EALREADY;
	}

	if (recv_cb == NULL) {
		LOG_ERR("Receive callback is NULL");
		return -EINVAL;
	}

	receive_cb = recv_cb;

	LOG_DBG("Start workers");

	for (int i = 0; i < ARRAY_SIZE(headsets); i++) {
		headsets[i].sink_stream.ops = &stream_ops;
		headsets[i].source_stream.ops = &stream_ops;
		k_work_init_delayable(&headsets[i].stream_start_sink_work, work_stream_start);
		k_work_init_delayable(&headsets[i].stream_start_source_work, work_stream_start);
	}

	ret = bt_bap_unicast_client_register_cb(&unicast_client_cbs);
	if (ret != 0) {
		LOG_ERR("Failed to register client callbacks: %d", ret);
		return ret;
	}

	ret = bt_le_audio_tx_init();
	if (ret) {
		return ret;
	}

	for (int i = 0; i < ARRAY_SIZE(headsets); i++) {
		switch (i) {
		case AUDIO_CH_L:
			headsets[i].ch_name = "LEFT";
			break;
		case AUDIO_CH_R:
			headsets[i].ch_name = "RIGHT";
			break;
		default:
			LOG_WRN("Trying to set name to undefined channel");
			headsets[i].ch_name = "UNDEFINED";
			break;
		}
	}

	for (int i = 0; i < ARRAY_SIZE(group_stream_params); i++) {
		/* Every other stream should be sink or source */
		if ((i % 2) == 0) {
			group_stream_params[i].qos = &lc3_preset_max.qos;
			group_stream_params[i].stream = &headsets[headset_iterator].sink_stream;
		} else {
			group_stream_params[i].qos = &lc3_preset_max.qos;
			group_stream_params[i].stream = &headsets[headset_iterator].source_stream;
			headset_iterator++;
		}
	}

	for (int i = 0; i < ARRAY_SIZE(pair_params); i++) {
		pair_params[i].tx_param = &group_stream_params[stream_iterator];
		stream_iterator++;

		if (IS_ENABLED(CONFIG_BT_AUDIO_RX)) {
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
		return ret;
	}

	initialized = true;

	return 0;
}
