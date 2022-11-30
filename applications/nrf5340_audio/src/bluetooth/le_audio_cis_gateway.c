/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "le_audio.h"

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <../subsys/bluetooth/audio/endpoint.h>

#include "macros_common.h"
#include "ctrl_events.h"
#include "audio_datapath.h"
#include "ble_audio_services.h"
#include "channel_assignment.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cis_gateway, CONFIG_BLE_LOG_LEVEL);

#define HCI_ISO_BUF_ALLOC_PER_CHAN 2
#define CIS_CONN_RETRY_TIMES 5
#define CONNECTION_PARAMETERS                                                                      \
	BT_LE_CONN_PARAM(CONFIG_BLE_ACL_CONN_INTERVAL, CONFIG_BLE_ACL_CONN_INTERVAL,               \
			 CONFIG_BLE_ACL_SLAVE_LATENCY, CONFIG_BLE_ACL_SUP_TIMEOUT)

/* For being able to dynamically define iso_tx_pools */
#define NET_BUF_POOL_ITERATE(i, _)                                                                 \
	NET_BUF_POOL_FIXED_DEFINE(iso_tx_pool_##i, HCI_ISO_BUF_ALLOC_PER_CHAN,                     \
				  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU), 8, NULL);
#define NET_BUF_POOL_PTR_ITERATE(i, ...) IDENTITY(&iso_tx_pool_##i)
LISTIFY(CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SNK_COUNT, NET_BUF_POOL_ITERATE, (;))
/* clang-format off */
static struct net_buf_pool *iso_tx_pools[] = { LISTIFY(CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SNK_COUNT,
						       NET_BUF_POOL_PTR_ITERATE, (,)) };
/* clang-format on */

struct le_audio_headset {
	char *ch_name;
	bool hci_wrn_printed;
	uint32_t seq_num;
	struct bt_audio_stream *sink_stream;
	struct bt_audio_ep *sink_ep;
	struct bt_audio_stream *source_stream;
	struct bt_audio_ep *source_ep;
	struct bt_conn *headset_conn;
	struct net_buf_pool *iso_tx_pool;
	atomic_t iso_tx_pool_alloc;
};

static struct le_audio_headset headsets[CONFIG_BT_MAX_CONN];

/* Make sure that we have at least one headset device per CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SNK */
BUILD_ASSERT(ARRAY_SIZE(headsets) >= CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SNK_COUNT,
	     "We need to have at least one headset device per ASE SINK");

/* Make sure that we have at least one headset device per CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SRC */
BUILD_ASSERT(ARRAY_SIZE(headsets) >= CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SRC_COUNT,
	     "We need to have at least one headset device per ASE SOURCE");

static le_audio_receive_cb receive_cb;
static struct bt_audio_stream audio_streams[CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SNK_COUNT +
					    CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SRC_COUNT];
static struct bt_audio_unicast_group *unicast_group;
static struct bt_codec *remote_codec[CONFIG_BT_AUDIO_UNICAST_CLIENT_PAC_COUNT * CONFIG_BT_MAX_CONN];

static struct bt_audio_lc3_preset lc3_preset_nrf5340 = BT_AUDIO_LC3_UNICAST_PRESET_NRF5340_AUDIO;

struct worker_data {
	uint8_t stream_index;
	uint8_t retries;
} __aligned(4);
K_MSGQ_DEFINE(kwork_msgq, sizeof(struct worker_data), ARRAY_SIZE(audio_streams), 4);

static struct k_work_delayable stream_start_work[ARRAY_SIZE(audio_streams)];

static uint8_t bonded_num;
static bool playing_state = true;

static void ble_acl_start_scan(void);
static bool ble_acl_gateway_all_links_connected(void);

static struct bt_audio_discover_params
	audio_sink_discover_param[CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SNK_COUNT];

#if CONFIG_STREAM_BIDIRECTIONAL
static struct bt_audio_discover_params
	audio_source_discover_param[CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SRC_COUNT];

static int discover_source(struct bt_conn *conn);
#endif /* CONFIG_STREAM_BIDIRECTIONAL */

static int next_free_audio_stream_index_get(uint8_t *index)
{
	for (int i = 0; i < ARRAY_SIZE(audio_streams); i++) {
		if (audio_streams[i].conn == NULL) {
			*index = i;
			return 0;
		}
	}
	LOG_WRN("All stream spots taken");
	return -ENOSPC;
}

/**
 * @brief  Get audio stream index based on the stream pointer
 *
 * @param  stream  The stream to search for
 * @param  index   The audio_stream index
 *
 * @return 0 if succesfull, -EINVAL if there is no match
 */
static int audio_stream_index_get(struct bt_audio_stream *stream, uint8_t *index)
{
	for (int i = 0; i < ARRAY_SIZE(audio_streams); i++) {
		if (&audio_streams[i] == stream) {
			*index = i;
			return 0;
		}
	}

	LOG_WRN("Stream not found");

	return -EINVAL;
}

/**
 * @brief  Check the endpoint state of all active headsets connected
 *
 * @note   This function only checks the sink streams. An 'active' headset is
 *	   one which we have connected to and want to stream to
 *
 * @return True if all active headsets are streaming, false otherwise
 */
static bool all_active_headsets_started(void)
{
	for (int i = 0; i < ARRAY_SIZE(headsets); i++) {
		if (headsets[i].sink_stream != NULL &&
		    headsets[i].sink_stream->ep->status.state != BT_AUDIO_EP_STATE_STREAMING) {
			return false;
		}
	}

	return true;
}

/**
 * @brief  Get channel index based on either conn or stream
 *
 * @param[in]  conn   The connection to search for, can be NULL
 * @param[in]  stream The stream to search for, can be NULL
 * @param[out] index  The channel index
 *
 * @return 0 if succesfull, -EINVAL if there is no match
 */
static int channel_index_get(const struct bt_conn *conn, const struct bt_audio_stream *stream,
			     uint8_t *index)
{
	if (conn == NULL && stream == NULL) {
		LOG_ERR("No conn or stream provided");
		return -EINVAL;
	}

	for (int i = 0; i < ARRAY_SIZE(headsets); i++) {
		if (conn != NULL) {
			if (headsets[i].headset_conn == conn) {
				*index = i;
				return 0;
			}
		} else if (stream != NULL) {
			if (headsets[i].sink_stream == stream ||
			    headsets[i].source_stream == stream) {
				*index = i;
				return 0;
			}
		}
	}

	LOG_WRN("Connection not found");

	return -EINVAL;
}

static uint32_t get_and_incr_seq_num(const struct bt_audio_stream *stream)
{
	uint8_t channel_index;

	if (channel_index_get(NULL, stream, &channel_index) == 0) {
		return headsets[channel_index].seq_num++;
	}

	LOG_WRN("Could not find matching stream %p", stream);

	return 0;
}

static void unicast_client_location_cb(struct bt_conn *conn, enum bt_audio_dir dir,
				       enum bt_audio_location loc)
{
	int ret;

	if ((loc & BT_AUDIO_LOCATION_FRONT_LEFT) != 0) {
		headsets[AUDIO_CH_L].headset_conn = conn;
	}
#if !CONFIG_STREAM_BIDIRECTIONAL
	else if ((loc & BT_AUDIO_LOCATION_FRONT_RIGHT) != 0) {
		headsets[AUDIO_CH_R].headset_conn = conn;
	}
#endif /* !CONFIG_STREAM_BIDIRECTIONAL */
	else {
		LOG_ERR("Channel location not supported");
		ret = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		if (ret) {
			LOG_ERR("Failed to disconnect %d", ret);
		}
	}

	if (!ble_acl_gateway_all_links_connected()) {
		ble_acl_start_scan();
	}
}

static void available_contexts_cb(struct bt_conn *conn, enum bt_audio_context snk_ctx,
				  enum bt_audio_context src_ctx)
{
	char addr[BT_ADDR_LE_STR_LEN];

	(void)bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_DBG("conn: %s, snk ctx %d src ctx %d\n", addr, snk_ctx, src_ctx);
}

const struct bt_audio_unicast_client_cb unicast_client_cbs = {
	.location = unicast_client_location_cb,
	.available_contexts = available_contexts_cb,
};

static void stream_sent_cb(struct bt_audio_stream *stream)
{
	int ret;
	uint8_t channel_index;

	ret = channel_index_get(NULL, stream, &channel_index);
	if (ret) {
		LOG_ERR("Stream not found");
	} else {
		atomic_dec(&headsets[channel_index].iso_tx_pool_alloc);
	}
}

static void stream_configured_cb(struct bt_audio_stream *stream,
				 const struct bt_codec_qos_pref *pref)
{
	int ret;
	uint8_t channel_index;

	ret = channel_index_get(stream->conn, NULL, &channel_index);
	if (ret) {
		LOG_ERR("Stream not found");
		return;
	}

	if (stream->ep->dir == BT_AUDIO_DIR_SINK) {
		headsets[channel_index].sink_stream = stream;
		LOG_INF("%s sink stream connected", headsets[channel_index].ch_name);
	} else if (stream->ep->dir == BT_AUDIO_DIR_SOURCE) {
		headsets[channel_index].source_stream = stream;
		LOG_INF("%s source stream connected", headsets[channel_index].ch_name);
	} else {
		LOG_WRN("Endpoint direction not recognized: %d", stream->ep->dir);
		return;
	}

	if (unicast_group) {
		ret = bt_audio_stream_qos(headsets[channel_index].headset_conn, unicast_group);
		if (ret) {
			LOG_ERR("Unable to set up QoS for headset %d: %d", channel_index, ret);
		}
	}
}

static void stream_qos_set_cb(struct bt_audio_stream *stream)
{
	int ret;

	if (playing_state) {
		ret = bt_audio_stream_enable(stream, lc3_preset_nrf5340.codec.meta,
					     lc3_preset_nrf5340.codec.meta_count);
		if (ret) {
			LOG_ERR("Unable to enable stream: %d", ret);
		}
	}
}

static void work_stream_start(struct k_work *work)
{
	int ret;
	struct worker_data work_data;

	ret = k_msgq_get(&kwork_msgq, &work_data, K_NO_WAIT);
	if (ret) {
		LOG_ERR("Cannot get info for start stream");
	}

	ret = bt_audio_stream_start(&audio_streams[work_data.stream_index]);
	work_data.retries++;

	if (ret) {
		if (work_data.retries < CIS_CONN_RETRY_TIMES) {
			LOG_WRN("Got connect error from stream %d Retrying. code: %d count: %d",
				work_data.stream_index, ret, work_data.retries);
			ret = k_msgq_put(&kwork_msgq, &work_data, K_NO_WAIT);
			if (ret) {
				LOG_ERR("No space in the queue for the bond");
			}
			/* Delay added to prevent controller overloading */
			k_work_reschedule(&stream_start_work[work_data.stream_index], K_MSEC(500));
		} else {
			LOG_ERR("Could not connect ch %d after %d retries", work_data.stream_index,
				work_data.retries);
			bt_conn_disconnect(audio_streams[work_data.stream_index].conn,
					   BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		}
	}
}

static void stream_enabled_cb(struct bt_audio_stream *stream)
{
	int ret;
	uint8_t stream_index;

	ret = audio_stream_index_get(stream, &stream_index);
	if (ret) {
		LOG_ERR("Stream not found");
	} else {
		struct worker_data work_data;

		work_data.stream_index = stream_index;
		work_data.retries = 0;

		ret = k_msgq_put(&kwork_msgq, &work_data, K_NO_WAIT);
		if (ret) {
			LOG_ERR("No space in the queue for the bond");
		}

		if (stream_index == 0) {
			k_work_schedule(&stream_start_work[stream_index], K_MSEC(500));
		} else {
			k_work_schedule(&stream_start_work[stream_index], K_MSEC(2000));
		}
	}
}

static void stream_started_cb(struct bt_audio_stream *stream)
{
	int ret;
	uint8_t channel_index;

	/* Reset sequence number for the stream */
	if (channel_index_get(NULL, stream, &channel_index) == 0) {
		headsets[channel_index].seq_num = 0;
	}

	LOG_INF("Stream %p started", (void *)stream);

	if (all_active_headsets_started()) {
		ret = ctrl_events_le_audio_event_send(LE_AUDIO_EVT_STREAMING);
		ERR_CHK(ret);
	}
}

static void stream_metadata_updated_cb(struct bt_audio_stream *stream)
{
	LOG_DBG("Audio Stream %p metadata updated", (void *)stream);
}

static void stream_disabled_cb(struct bt_audio_stream *stream)
{
	LOG_DBG("Audio Stream %p disabled", (void *)stream);
}

static void stream_stopped_cb(struct bt_audio_stream *stream)
{
	int ret;
	uint8_t channel_index;

	LOG_INF("Stream %p stopped", (void *)stream);

	ret = channel_index_get(NULL, stream, &channel_index);
	if (ret) {
		LOG_ERR("Stream not found");
	} else {
		atomic_clear(&headsets[channel_index].iso_tx_pool_alloc);
	}

	if (headsets[AUDIO_CH_L].sink_stream->ep->status.state != BT_AUDIO_EP_STATE_STREAMING
#if !CONFIG_STREAM_BIDIRECTIONAL
	    && headsets[AUDIO_CH_R].sink_stream->ep->status.state != BT_AUDIO_EP_STATE_STREAMING
#endif
	) {
		ret = ctrl_events_le_audio_event_send(LE_AUDIO_EVT_NOT_STREAMING);
		ERR_CHK(ret);
	}
}

static void stream_released_cb(struct bt_audio_stream *stream)
{
	int ret;
	LOG_DBG("Audio Stream %p released", (void *)stream);

	if (headsets[AUDIO_CH_L].sink_stream->ep->status.state != BT_AUDIO_EP_STATE_STREAMING
#if !CONFIG_STREAM_BIDIRECTIONAL
	    && headsets[AUDIO_CH_R].sink_stream->ep->status.state != BT_AUDIO_EP_STATE_STREAMING
#endif
	) {
		ret = ctrl_events_le_audio_event_send(LE_AUDIO_EVT_NOT_STREAMING);
		ERR_CHK(ret);
	}
}

static void stream_recv_cb(struct bt_audio_stream *stream, const struct bt_iso_recv_info *info,
			   struct net_buf *buf)
{
	static uint32_t recv_cnt;
	bool bad_frame = false;

	if (receive_cb == NULL) {
		LOG_ERR("The RX callback has not been set");
		return;
	}

	if (!(info->flags & BT_ISO_FLAGS_VALID)) {
		bad_frame = true;
	}

	receive_cb(buf->data, buf->len, bad_frame, info->ts);

	recv_cnt++;
	if ((recv_cnt % 1000U) == 0U) {
		LOG_DBG("Received %d total ISO packets", recv_cnt);
	}
}

static struct bt_audio_stream_ops stream_ops = {
	.sent = stream_sent_cb,
	.configured = stream_configured_cb,
	.qos_set = stream_qos_set_cb,
	.enabled = stream_enabled_cb,
	.started = stream_started_cb,
	.metadata_updated = stream_metadata_updated_cb,
	.disabled = stream_disabled_cb,
	.stopped = stream_stopped_cb,
	.released = stream_released_cb,
	.recv = stream_recv_cb,
};

static void add_remote_codec(struct bt_codec *codec, int index, uint8_t type)
{
	if (type != BT_AUDIO_DIR_SINK && type != BT_AUDIO_DIR_SOURCE) {
		return;
	}

	if (index < (CONFIG_BT_AUDIO_UNICAST_CLIENT_PAC_COUNT * CONFIG_BT_MAX_CONN)) {
		remote_codec[index] = codec;
	}
}

static void discover_sink_cb(struct bt_conn *conn, struct bt_codec *codec, struct bt_audio_ep *ep,
			     struct bt_audio_discover_params *params)
{
	int ret = 0;
	uint8_t channel_index = 0;
	uint8_t stream_index = 0;

	if (params->err) {
		LOG_ERR("Discovery failed: %d", params->err);
		return;
	}

	if (codec != NULL) {
		add_remote_codec(codec, params->num_caps, params->dir);
		return;
	}

	ret = channel_index_get(conn, NULL, &channel_index);
	if (ret) {
		LOG_ERR("Unknown connection, should not reach here");
		return;
	}

	if (ep != NULL) {
		headsets[channel_index].sink_ep = ep;
		return;
	}

	LOG_DBG("Sink discover complete: err %d", params->err);

	(void)memset(params, 0, sizeof(*params));

#if (CONFIG_BT_VCS_CLIENT)
	ret = ble_vcs_discover(conn, channel_index);
	if (ret) {
		LOG_ERR("Could not do VCS discover");
	}
#endif /* (CONFIG_BT_VCS_CLIENT) */
	ret = next_free_audio_stream_index_get(&stream_index);
	if (ret) {
		LOG_WRN("Failed to get free index");
		return;
	}

	ret = bt_audio_stream_config(conn, &audio_streams[stream_index],
				     headsets[channel_index].sink_ep, &lc3_preset_nrf5340.codec);
	if (ret) {
		LOG_ERR("Could not configure stream");
	}

#if CONFIG_STREAM_BIDIRECTIONAL
	ret = discover_source(conn);
	if (ret) {
		LOG_WRN("Failed to discover source: %d", ret);
	}
#endif /* CONFIG_STREAM_BIDIRECTIONAL */
}

#if CONFIG_STREAM_BIDIRECTIONAL
static void discover_source_cb(struct bt_conn *conn, struct bt_codec *codec, struct bt_audio_ep *ep,
			       struct bt_audio_discover_params *params)
{
	int ret = 0;

	uint8_t channel_index = 0;
	uint8_t stream_index = 0;
	static bool group_created;
	struct bt_audio_unicast_group_param
		group_params[CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SRC_COUNT +
			     CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SNK_COUNT];

	if (params->err) {
		LOG_ERR("Discovery failed: %d", params->err);
		return;
	}

	if (codec != NULL) {
		add_remote_codec(codec, params->num_caps, params->dir);
		return;
	}

	ret = channel_index_get(conn, NULL, &channel_index);

	if (ret) {
		LOG_ERR("Unknown connection, should not reach here");
		return;
	}

	if (ep != NULL) {
		headsets[channel_index].source_ep = ep;
		return;
	}

	LOG_DBG("Source discover complete: err %d", params->err);

	(void)memset(params, 0, sizeof(*params));

	ret = next_free_audio_stream_index_get(&stream_index);
	if (ret) {
		LOG_WRN("Failed to get free index");
		return;
	}

	ret = bt_audio_stream_config(conn, &audio_streams[stream_index],
				     headsets[channel_index].source_ep, &lc3_preset_nrf5340.codec);
	if (ret) {
		LOG_WRN("Could not configure stream");
		return;
	}

	if (!group_created) {
		for (int i = 0; i < ARRAY_SIZE(audio_streams); i++) {
			group_params[i].stream = &audio_streams[i];
			group_params[i].qos = &lc3_preset_nrf5340.qos;

			/* Make the first stream be sink */
			if (i == 0) {
				group_params[i].dir = BT_AUDIO_DIR_SINK;
				LOG_DBG("Sink stream: %p", (void *)&audio_streams[i]);
			} else {
				group_params[i].dir = BT_AUDIO_DIR_SOURCE;
				LOG_DBG("Source stream: %p", (void *)&audio_streams[i]);
			}
		}

		ret = bt_audio_unicast_group_create(group_params, ARRAY_SIZE(audio_streams),
						    &unicast_group);
		if (ret) {
			LOG_ERR("Failed to create unicast group: %d", ret);
			return;
		}

		group_created = true;
	}
}
#endif /* CONFIG_STREAM_BIDIRECTIONAL */

static bool ble_acl_gateway_all_links_connected(void)
{
	for (int i = 0; i < ARRAY_SIZE(headsets); i++) {
		if (headsets[i].headset_conn == NULL) {
			return false;
		}
	}

	return true;
}

static void bond_check(const struct bt_bond_info *info, void *user_data)
{
	char addr_buf[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(&info->addr, addr_buf, BT_ADDR_LE_STR_LEN);

	LOG_DBG("Stored bonding found: %s", addr_buf);
	bonded_num++;
}

static void bond_connect(const struct bt_bond_info *info, void *user_data)
{
	int ret;
	const bt_addr_le_t *adv_addr = user_data;
	struct bt_conn *conn;

	if (!bt_addr_le_cmp(&info->addr, adv_addr)) {
		LOG_DBG("Found bonded device");

		ret = bt_le_scan_stop();
		if (ret) {
			LOG_WRN("Stop scan failed: %d", ret);
		}

		ret = bt_conn_le_create(adv_addr, BT_CONN_LE_CREATE_CONN, CONNECTION_PARAMETERS,
					&conn);
		if (ret) {
			LOG_WRN("Create ACL connection failed: %d", ret);
			ble_acl_start_scan();
		}
	}
}

static int device_found(uint8_t type, const uint8_t *data, uint8_t data_len,
			const bt_addr_le_t *addr)
{
	int ret;
	struct bt_conn *conn;

	if (ble_acl_gateway_all_links_connected()) {
		LOG_DBG("All headset connected");
		return 0;
	}

	if ((data_len == DEVICE_NAME_PEER_LEN) &&
	    (strncmp(DEVICE_NAME_PEER, data, DEVICE_NAME_PEER_LEN) == 0)) {
		LOG_DBG("Device found");

		ret = bt_le_scan_stop();
		if (ret) {
			LOG_WRN("Stop scan failed: %d", ret);
		}

		ret = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, CONNECTION_PARAMETERS, &conn);
		if (ret) {
			LOG_ERR("Could not init connection");
			ble_acl_start_scan();
			return ret;
		}

		return 0;
	}

	return -ENOENT;
}

/** @brief  Parse BLE advertisement package.
 */
static void ad_parse(struct net_buf_simple *p_ad, const bt_addr_le_t *addr)
{
	while (p_ad->len > 1) {
		uint8_t len = net_buf_simple_pull_u8(p_ad);
		uint8_t type;

		/* Check for early termination */
		if (len == 0) {
			return;
		}

		if (len > p_ad->len) {
			LOG_WRN("AD malformed");
			return;
		}

		type = net_buf_simple_pull_u8(p_ad);

		if (device_found(type, p_ad->data, len - 1, addr) == 0) {
			return;
		}

		(void)net_buf_simple_pull(p_ad, len - 1);
	}
}

static void on_device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			    struct net_buf_simple *p_ad)
{
	/* Direct advertising has no payload, so no need to parse */
	if (type == BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
		if (bonded_num) {
			bt_foreach_bond(BT_ID_DEFAULT, bond_connect, (void *)addr);
		}
		return;
	} else if (type == BT_GAP_ADV_TYPE_ADV_IND || type == BT_GAP_ADV_TYPE_EXT_ADV) {
		/* Note: May lead to connection creation */
		ad_parse(p_ad, addr);
	}
}

static void ble_acl_start_scan(void)
{
	int ret;

	/* Reset number of bonds found */
	bonded_num = 0;

	bt_foreach_bond(BT_ID_DEFAULT, bond_check, NULL);

	ret = bt_le_scan_start(BT_LE_SCAN_PASSIVE, on_device_found);
	if (ret) {
		LOG_WRN("Scanning failed to start: %d", ret);
		return;
	}

	LOG_INF("Scanning successfully started");
}

static void connected_cb(struct bt_conn *conn, uint8_t err)
{
	int ret;
	char addr[BT_ADDR_LE_STR_LEN];

	(void)bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		LOG_ERR("ACL connection to %s failed, error %d", addr, err);

		bt_conn_unref(conn);
		ble_acl_start_scan();

		return;
	}

	/* ACL connection established */
	/* TODO: Setting TX power for connection if set to anything but 0 */
	LOG_INF("Connected: %s", addr);

	ret = bt_conn_set_security(conn, BT_SECURITY_L2);
	if (ret) {
		LOG_ERR("Failed to set security to L2: %d", ret);
	}
}

static void disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
	int ret;
	uint8_t channel_index;
	char addr[BT_ADDR_LE_STR_LEN];

	(void)bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Disconnected: %s (reason 0x%02x)", addr, reason);

	bt_conn_unref(conn);

	ret = channel_index_get(conn, NULL, &channel_index);
	if (ret) {
		LOG_WRN("Unknown connection");
	} else {
		headsets[channel_index].headset_conn = NULL;
	}

	ble_acl_start_scan();
}

static int discover_sink(struct bt_conn *conn)
{
	int ret = 0;
	static uint8_t discover_index;

	audio_sink_discover_param[discover_index].func = discover_sink_cb;
	audio_sink_discover_param[discover_index].dir = BT_AUDIO_DIR_SINK;
	ret = bt_audio_discover(conn, &audio_sink_discover_param[discover_index]);
	discover_index++;

	/* Avoid multiple discover procedure sharing the same params */
	if (discover_index > CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SNK_COUNT - 1) {
		discover_index = 0;
	}

	return ret;
}

#if CONFIG_STREAM_BIDIRECTIONAL
static int discover_source(struct bt_conn *conn)
{
	int ret = 0;
	static uint8_t discover_index;

	audio_source_discover_param[discover_index].func = discover_source_cb;
	audio_source_discover_param[discover_index].dir = BT_AUDIO_DIR_SOURCE;
	ret = bt_audio_discover(conn, &audio_source_discover_param[discover_index]);
	discover_index++;

	/* Avoid multiple discover procedure sharing the same params */
	if (discover_index > CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SRC_COUNT - 1) {
		discover_index = 0;
	}

	return ret;
}
#endif /* CONFIG_STREAM_BIDIRECTIONAL */

static void security_changed_cb(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	int ret;

	if (err) {
		LOG_ERR("Security failed: level %d err %d", level, err);
		ret = bt_conn_disconnect(conn, err);
		if (ret) {
			LOG_ERR("Failed to disconnect %d", ret);
		}
	} else {
		LOG_DBG("Security changed: level %d", level);
		ret = discover_sink(conn);
		if (ret) {
			LOG_WRN("Failed to discover sink: %d", ret);
		}
	}
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected_cb,
	.disconnected = disconnected_cb,
	.security_changed = security_changed_cb,

};

#if (CONFIG_BT_MCS)
/**
 * @brief	Callback handler for play/pause.
 *
 * @note	This callback is called from MCS after receiving a
 *		command from the client or the local media player.
 *
 * @param[in]	play  Boolean to indicate if the stream should be resumed or paused.
 */
static void le_audio_play_pause_cb(bool play)
{
	int ret;

	LOG_DBG("Play/pause cb, state: %d", play);

	if (play) {
		if (audio_streams[AUDIO_CH_L].ep->status.state ==
		    BT_AUDIO_EP_STATE_QOS_CONFIGURED) {
			ret = bt_audio_stream_enable(&audio_streams[AUDIO_CH_L],
						     lc3_preset_nrf5340.codec.meta,
						     lc3_preset_nrf5340.codec.meta_count);

			if (ret) {
				LOG_WRN("Failed to enable left stream");
			}
		}

#if !CONFIG_STREAM_BIDIRECTIONAL
		if (audio_streams[AUDIO_CH_R].ep->status.state ==
		    BT_AUDIO_EP_STATE_QOS_CONFIGURED) {
			ret = bt_audio_stream_enable(&audio_streams[AUDIO_CH_R],
						     lc3_preset_nrf5340.codec.meta,
						     lc3_preset_nrf5340.codec.meta_count);

			if (ret) {
				LOG_WRN("Failed to enable right stream");
			}
		}
#endif /* !CONFIG_STREAM_BIDIRECTIONAL */
	} else {
		ret = ctrl_events_le_audio_event_send(LE_AUDIO_EVT_NOT_STREAMING);
		ERR_CHK(ret);

		if (audio_streams[AUDIO_CH_L].ep->status.state == BT_AUDIO_EP_STATE_STREAMING) {
			ret = bt_audio_stream_disable(&audio_streams[AUDIO_CH_L]);

			if (ret) {
				LOG_WRN("Failed to disable left stream");
			}
		}

#if !CONFIG_STREAM_BIDIRECTIONAL
		if (audio_streams[AUDIO_CH_R].ep->status.state == BT_AUDIO_EP_STATE_STREAMING) {
			ret = bt_audio_stream_disable(&audio_streams[AUDIO_CH_R]);

			if (ret) {
				LOG_WRN("Failed to disable right stream");
			}
		}
#endif /* !CONFIG_STREAM_BIDIRECTIONAL */
	}

	playing_state = play;
}
#endif /* CONFIG_BT_MCS */

static int iso_stream_send(uint8_t const *const data, size_t size, struct le_audio_headset headset)
{
	int ret;
	struct net_buf *buf;

	if (headset.sink_stream->ep->status.state != BT_AUDIO_EP_STATE_STREAMING) {
		LOG_DBG("%s channel not connected", headset.ch_name);
		return 0;
	}

	/* net_buf_alloc allocates buffers for APP->NET transfer over HCI RPMsg,
	 * but when these buffers are released it is not guaranteed that the
	 * data has actually been sent. The data might be queued on the NET core,
	 * and this can cause delays in the audio.
	 * When stream_sent_cb() is called the data has been sent.
	 * Data will be discarded if allocation becomes too high, to avoid audio delays.
	 * If the NET and APP core operates in clock sync, discarding should not occur.
	 */
	if (atomic_get(&headset.iso_tx_pool_alloc) >= HCI_ISO_BUF_ALLOC_PER_CHAN) {
		if (!headset.hci_wrn_printed) {
			LOG_WRN("HCI ISO TX overrun on %s ch - Single print", headset.ch_name);
			headset.hci_wrn_printed = true;
		}

		return -ENOMEM;
	}

	headset.hci_wrn_printed = false;

	buf = net_buf_alloc(headset.iso_tx_pool, K_NO_WAIT);
	if (buf == NULL) {
		/* This should never occur because of the iso_tx_pool_alloc check above */
		LOG_WRN("Out of TX buffers");
		return -ENOMEM;
	}

	net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);
	net_buf_add_mem(buf, data, size);

	atomic_inc(&headset.iso_tx_pool_alloc);

	ret = bt_audio_stream_send(headset.sink_stream, buf,
				   get_and_incr_seq_num(headset.sink_stream),
				   BT_ISO_TIMESTAMP_NONE);
	if (ret < 0) {
		LOG_WRN("Failed to send audio data: %d", ret);
		net_buf_unref(buf);
		atomic_dec(&headset.iso_tx_pool_alloc);
	}

	return 0;
}

static int initialize(le_audio_receive_cb recv_cb)
{
	int ret;
	static bool initialized;

	if (initialized) {
		LOG_WRN("Already initialized");
		return -EALREADY;
	}

	for (int i = 0; i < ARRAY_SIZE(audio_streams); i++) {
		audio_streams[i].ops = &stream_ops;
	}

	for (int i = 0; i < ARRAY_SIZE(audio_streams); i++) {
		k_work_init_delayable(&stream_start_work[i], work_stream_start);
	}

	ret = bt_audio_unicast_client_register_cb(&unicast_client_cbs);
	if (ret != 0) {
		LOG_ERR("Failed to register client callbacks: %d", ret);
		return ret;
	}

	for (int i = 0; i < ARRAY_SIZE(headsets); i++) {
		headsets[i].iso_tx_pool = iso_tx_pools[i];
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

#if !CONFIG_STREAM_BIDIRECTIONAL
	struct bt_audio_unicast_group_param group_params[ARRAY_SIZE(audio_streams)];

	for (int i = 0; i < ARRAY_SIZE(audio_streams); i++) {
		group_params[i].stream = &audio_streams[i];
		group_params[i].qos = &lc3_preset_nrf5340.qos;
		group_params[i].dir = BT_AUDIO_DIR_SINK;
	}

	ret = bt_audio_unicast_group_create(group_params, ARRAY_SIZE(audio_streams),
					    &unicast_group);
	if (ret) {
		LOG_ERR("Failed to create unicast group: %d", ret);
		return ret;
	}
#endif /* !CONFIG_STREAM_BIDIRECTIONAL */

#if (CONFIG_BT_VCS_CLIENT)
	ret = ble_vcs_client_init();
	if (ret) {
		LOG_ERR("VCS client init failed");
		return ret;
	}
#endif /* (CONFIG_BT_VCS_CLIENT) */

#if (CONFIG_BT_MCS)
	ret = ble_mcs_server_init(le_audio_play_pause_cb);
	if (ret) {
		LOG_ERR("MCS server init failed");
		return ret;
	}
#endif /* CONFIG_BT_MCS */

	receive_cb = recv_cb;

	bt_conn_cb_register(&conn_callbacks);

	initialized = true;

	return 0;
}

int le_audio_user_defined_button_press(void)
{
	return 0;
}

int le_audio_config_get(uint32_t *bitrate, uint32_t *sampling_rate)
{
	return 0;
}

int le_audio_volume_up(void)
{
	int ret;

	ret = ble_vcs_volume_up();
	if (ret) {
		LOG_WRN("Failed to increase volume");
		return ret;
	}

	return 0;
}

int le_audio_volume_down(void)
{
	int ret;

	ret = ble_vcs_volume_down();
	if (ret) {
		LOG_WRN("Failed to decrease volume");
		return ret;
	}

	return 0;
}

int le_audio_volume_mute(void)
{
	int ret;

	ret = ble_vcs_volume_mute();
	if (ret) {
		LOG_WRN("Failed to mute volume");
		return ret;
	}

	return 0;
}

int le_audio_play_pause(void)
{
	int ret;

	ret = ble_mcs_play_pause(NULL);
	if (ret) {
		LOG_WRN("Failed to change streaming state");
		return ret;
	}

	return 0;
}

int le_audio_send(uint8_t const *const data, size_t size)
{
	int ret;
	struct bt_iso_tx_info tx_info = { 0 };
	size_t sdu_size = LE_AUDIO_SDU_SIZE_OCTETS(CONFIG_LC3_BITRATE);

	if (size != (sdu_size * 2)) {
		LOG_ERR("Not enough data for stereo stream");
		return -ECANCELED;
	}

	if (headsets[AUDIO_CH_L].sink_stream->ep->status.state == BT_AUDIO_EP_STATE_STREAMING) {
		ret = bt_iso_chan_get_tx_sync(headsets[AUDIO_CH_L].sink_stream->iso, &tx_info);
	}
#if !CONFIG_STREAM_BIDIRECTIONAL
	else if (headsets[AUDIO_CH_R].sink_stream->ep->status.state ==
		 BT_AUDIO_EP_STATE_STREAMING) {
		ret = bt_iso_chan_get_tx_sync(headsets[AUDIO_CH_R].sink_stream->iso, &tx_info);
	}
#endif /* !CONFIG_STREAM_BIDIRECTIONAL */
	else {
		LOG_DBG("No headset in stream state");
		return -ECANCELED;
	}

	if (ret) {
		LOG_DBG("Error getting ISO TX anchor point: %d", ret);
	}

	if (tx_info.ts != 0 && !ret) {
#if ((CONFIG_AUDIO_SOURCE_I2S) && !(CONFIG_STREAM_BIDIRECTIONAL))
		audio_datapath_sdu_ref_update(tx_info.ts);
#endif
		audio_datapath_just_in_time_check_and_adjust(tx_info.ts);
	}

	ret = iso_stream_send(data, sdu_size, headsets[AUDIO_CH_L]);
	if (ret) {
		LOG_DBG("Failed to send data to left channel");
	}

#if !CONFIG_STREAM_BIDIRECTIONAL
	ret = iso_stream_send(&data[sdu_size], sdu_size, headsets[AUDIO_CH_R]);
	if (ret) {
		LOG_DBG("Failed to send data to right channel");
	}
#endif /* !CONFIG_STREAM_BIDIRECTIONAL */

	return 0;
}

int le_audio_enable(le_audio_receive_cb recv_cb)
{
	int ret;

	ret = initialize(recv_cb);
	if (ret) {
		return ret;
	}

	ble_acl_start_scan();
	return 0;
}

int le_audio_disable(void)
{
	return 0;
}
