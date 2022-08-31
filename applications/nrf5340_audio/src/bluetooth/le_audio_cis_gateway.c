/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "le_audio.h"

#include <bluetooth/bluetooth.h>
#include <bluetooth/audio/audio.h>
#include <../subsys/bluetooth/audio/endpoint.h>

#include "macros_common.h"
#include "ctrl_events.h"
#include "audio_datapath.h"
#include "ble_audio_services.h"
#include "channel_assignment.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(cis_gateway, CONFIG_LOG_BLE_LEVEL);

#define BT_AUDIO_LC3_UNICAST_PRESET_NRF5340_AUDIO                                                  \
	BT_AUDIO_LC3_PRESET(                                                                       \
		BT_CODEC_LC3_CONFIG(BT_CODEC_CONFIG_LC3_FREQ_48KHZ,                                \
				    BT_CODEC_CONFIG_LC3_DURATION_10, BT_AUDIO_LOCATION_FRONT_LEFT, \
				    LE_AUDIO_SDU_SIZE_OCTETS(CONFIG_LC3_BITRATE), 1,               \
				    BT_AUDIO_CONTEXT_TYPE_MEDIA),                                  \
		BT_CODEC_LC3_QOS_10_UNFRAMED(LE_AUDIO_SDU_SIZE_OCTETS(CONFIG_LC3_BITRATE), 2u,     \
					     20u, LE_AUDIO_PRES_DELAY_US))

#define HCI_ISO_BUF_ALLOC_PER_CHAN 2
/* For being able to dynamically define iso_tx_pools */
#define NET_BUF_POOL_ITERATE(i, _)                                                                 \
	NET_BUF_POOL_FIXED_DEFINE(iso_tx_pool_##i, HCI_ISO_BUF_ALLOC_PER_CHAN,                     \
				  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU), 8, NULL);
#define NET_BUF_POOL_PTR_ITERATE(i, ...) IDENTITY(&iso_tx_pool_##i)
LISTIFY(CONFIG_BT_ISO_MAX_CHAN, NET_BUF_POOL_ITERATE, (;))
#define BT_LE_CONN_PARAM_MULTI                                                                     \
	BT_LE_CONN_PARAM(CONFIG_BLE_ACL_CONN_INTERVAL, CONFIG_BLE_ACL_CONN_INTERVAL,               \
			 CONFIG_BLE_ACL_SLAVE_LATENCY, CONFIG_BLE_ACL_SUP_TIMEOUT)
#define CIS_CONN_RETRY_TIMES 5

static struct bt_conn *headset_conn[CONFIG_BT_MAX_CONN];
static struct bt_audio_stream audio_streams[CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SNK_COUNT];
static struct bt_audio_unicast_group *unicast_group;
static struct bt_codec *remote_codecs[CONFIG_BT_AUDIO_UNICAST_CLIENT_PAC_COUNT];
static struct bt_audio_discover_params audio_discover_param[CONFIG_BT_MAX_CONN];

static struct bt_audio_sink {
	struct bt_audio_ep *ep;
	uint32_t seq_num;
} sinks[CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SNK_COUNT];
/* clang-format off */
static struct net_buf_pool *iso_tx_pools[] = { LISTIFY(CONFIG_BT_ISO_MAX_CHAN,
						       NET_BUF_POOL_PTR_ITERATE, (,)) };
/* clang-format on */
static struct bt_audio_lc3_preset lc3_preset_nrf5340 = BT_AUDIO_LC3_UNICAST_PRESET_NRF5340_AUDIO;
static atomic_t iso_tx_pool_alloc[CONFIG_BT_ISO_MAX_CHAN];

struct worker_data {
	uint8_t channel;
	uint8_t retries;
} __aligned(4);
K_MSGQ_DEFINE(kwork_msgq, sizeof(struct worker_data), CONFIG_BT_ISO_MAX_CHAN, 4);
static struct k_work_delayable stream_start_work[CONFIG_BT_ISO_MAX_CHAN];

static uint8_t bonded_num;
static bool playing_state = true;

static void ble_acl_start_scan(void);
static bool ble_acl_gateway_all_links_connected(void);

static bool is_iso_buffer_full(uint8_t idx)
{
	/* net_buf_alloc allocates buffers for APP->NET transfer over HCI RPMsg,
	 * but when these buffers are released it is not guaranteed that the
	 * data has actually been sent. The data might be qued on the NET core,
	 * and this can cause delays in the audio.
	 * When stream_sent_cb() is called the data has been sent.
	 * Data will be discarded if allocation becomes too high, to avoid audio delays.
	 * If the NET and APP core operates in clock sync, discarding should not occur.
	 */

	if (atomic_get(&iso_tx_pool_alloc[idx]) >= HCI_ISO_BUF_ALLOC_PER_CHAN) {
		return true;
	}

	return false;
}

static int stream_index_get(struct bt_audio_stream *stream, uint8_t *index)
{
	for (size_t i = 0U; i < ARRAY_SIZE(audio_streams); i++) {
		if (&audio_streams[i] == stream) {
			*index = i;
			return 0;
		}
	}

	LOG_WRN("Stream not found");

	return -EINVAL;
}

static int stream_index_from_conn_get(struct bt_conn *conn, uint8_t *index)
{
	for (size_t i = 0U; i < ARRAY_SIZE(audio_streams); i++) {
		if (audio_streams[i].conn == conn) {
			*index = i;
			return 0;
		}
	}

	LOG_DBG("Stream may not have started yet");

	return -EINVAL;
}

static int headset_conn_index_get(struct bt_conn *conn, uint8_t *index)
{
	for (size_t i = 0U; i < ARRAY_SIZE(headset_conn); i++) {
		if (headset_conn[i] == conn) {
			*index = i;
			return 0;
		}
	}

	LOG_WRN("Connection not found");

	return -EINVAL;
}

static uint32_t get_and_incr_seq_num(const struct bt_audio_stream *stream)
{
	for (size_t i = 0U; i < ARRAY_SIZE(sinks); i++) {
		if (stream->ep == sinks[i].ep) {
			return sinks[i].seq_num++;
		}
	}

	LOG_WRN("Could not find endpoint from stream %p", stream);

	return 0;
}

static void unicast_client_location_cb(struct bt_conn *conn, enum bt_audio_dir dir,
				       enum bt_audio_location loc)
{
	int ret;

	if (loc == BT_AUDIO_LOCATION_FRONT_LEFT) {
		headset_conn[AUDIO_CH_L] = conn;
	} else if (loc == BT_AUDIO_LOCATION_FRONT_RIGHT) {
		headset_conn[AUDIO_CH_R] = conn;
	} else {
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
	int ret;
	uint8_t index;
	char addr[BT_ADDR_LE_STR_LEN];

	(void)bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	ret = stream_index_from_conn_get(conn, &index);
	if (ret) {
		return;
	}

	LOG_DBG("conn: %s, snk ctx %u src ctx %u\n", addr, snk_ctx, src_ctx);

	if (!(BT_AUDIO_CONTEXT_TYPE_MEDIA & snk_ctx)) {
		if (audio_streams[index].ep->status.state == BT_AUDIO_EP_STATE_STREAMING) {
			le_audio_pause();
		}
	} else {
		if (audio_streams[index].ep->status.state == BT_AUDIO_EP_STATE_QOS_CONFIGURED) {
			le_audio_play();
		}
	}
}

const struct bt_audio_unicast_client_cb unicast_client_cbs = {
	.location = unicast_client_location_cb,
	.available_contexts = available_contexts_cb,
};

static void stream_sent_cb(struct bt_audio_stream *stream)
{
	int ret;
	uint8_t channel_index;

	ret = stream_index_get(stream, &channel_index);
	if (ret) {
		LOG_ERR("Stream not found");
	} else {
		atomic_dec(&iso_tx_pool_alloc[channel_index]);
	}
}

static void stream_configured_cb(struct bt_audio_stream *stream,
				 const struct bt_codec_qos_pref *pref)
{
	int ret;
	uint8_t channel_index;

	ret = stream_index_get(stream, &channel_index);
	if (ret) {
		LOG_ERR("Stream not found");
	} else {
		ret = bt_audio_stream_qos(headset_conn[channel_index], unicast_group);
		if (ret) {
			LOG_ERR("Unable to setup QoS for headset %d: %d", channel_index, ret);
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

	ret = bt_audio_stream_start(&audio_streams[work_data.channel]);
	work_data.retries++;

	if (ret) {
		if (work_data.retries < CIS_CONN_RETRY_TIMES) {
			LOG_WRN("Got connect error from ch %d Retrying. code: %d count: %d",
				work_data.channel, ret, work_data.retries);
			ret = k_msgq_put(&kwork_msgq, &work_data, K_NO_WAIT);
			if (ret) {
				LOG_ERR("No space in the queue for the bond");
			}
			/* Delay added to prevent controller overloading */
			k_work_reschedule(&stream_start_work[work_data.channel], K_MSEC(500));
		} else {
			LOG_ERR("Could not connect ch %d after %d retries", work_data.channel,
				work_data.retries);
			bt_conn_disconnect(headset_conn[work_data.channel],
					   BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		}
	}
}

static void stream_enabled_cb(struct bt_audio_stream *stream)
{
	int ret;
	uint8_t channel_index;

	ret = stream_index_get(stream, &channel_index);
	if (ret) {
		LOG_ERR("Stream not found");
	} else {
		struct worker_data work_data;

		work_data.channel = channel_index;
		work_data.retries = 0;

		ret = k_msgq_put(&kwork_msgq, &work_data, K_NO_WAIT);
		if (ret) {
			LOG_ERR("No space in the queue for the bond");
		}

		k_work_schedule(&stream_start_work[channel_index], K_MSEC(500));
	}
}

static void stream_started_cb(struct bt_audio_stream *stream)
{
	int ret;

	/* Reset sequence number for sinks */
	for (size_t i = 0U; i < ARRAY_SIZE(sinks); i++) {
		if (stream->ep == sinks[i].ep) {
			sinks[i].seq_num = 0U;
			break;
		}
	}

	LOG_INF("Stream %p started", (void *)stream);

	ret = ctrl_events_le_audio_event_send(LE_AUDIO_EVT_STREAMING);
	ERR_CHK(ret);
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

	ret = stream_index_get(stream, &channel_index);
	if (ret) {
		LOG_ERR("Stream not found");
	} else {
		atomic_clear(&iso_tx_pool_alloc[channel_index]);
	}

	if (audio_streams[AUDIO_CH_L].ep->status.state != BT_AUDIO_EP_STATE_STREAMING &&
	    audio_streams[AUDIO_CH_R].ep->status.state != BT_AUDIO_EP_STATE_STREAMING) {
		ret = ctrl_events_le_audio_event_send(LE_AUDIO_EVT_NOT_STREAMING);
		ERR_CHK(ret);
	}
}

static void stream_released_cb(struct bt_audio_stream *stream)
{
	int ret;
	LOG_DBG("Audio Stream %p released", (void *)stream);

	ret = ctrl_events_le_audio_event_send(LE_AUDIO_EVT_NOT_STREAMING);
	ERR_CHK(ret);
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
};

static void add_remote_sink(struct bt_audio_ep *ep, uint8_t index)
{
	if (index > sizeof(sinks)) {
		LOG_ERR("Sink index is out of range");
	} else {
		sinks[index].ep = ep;
	}
}

static void add_remote_codec(struct bt_codec *codec, int index, uint8_t type)
{
	if (type != BT_AUDIO_DIR_SINK && type != BT_AUDIO_DIR_SOURCE) {
		return;
	}

	if (index < CONFIG_BT_AUDIO_UNICAST_CLIENT_PAC_COUNT) {
		remote_codecs[index] = codec;
	}
}

static void discover_sink_cb(struct bt_conn *conn, struct bt_codec *codec, struct bt_audio_ep *ep,
			     struct bt_audio_discover_params *params)
{
	int ret = 0;
	uint8_t ep_index = 0;
	uint8_t conn_index = 0;

	if (params->err) {
		LOG_ERR("Discovery failed: %d", params->err);
		return;
	}

	if (codec != NULL) {
		add_remote_codec(codec, params->num_caps, params->dir);
		return;
	}

	if (ep != NULL) {
		if (params->dir == BT_AUDIO_DIR_SINK) {
			ret = headset_conn_index_get(conn, &conn_index);
			if (ret) {
				ERR_CHK_MSG(ret, "Unknown connection, should not reach here");
			} else {
				ep_index = conn_index;
			}
			add_remote_sink(ep, ep_index);
		} else {
			LOG_ERR("Invalid param type: %u", params->dir);
		}

		return;
	}

	LOG_DBG("Discover complete: err %d", params->err);

	(void)memset(params, 0, sizeof(*params));

	ret = headset_conn_index_get(conn, &conn_index);

	if (ret) {
		LOG_ERR("Unknown connection, should not reach here");
	} else {
#if (CONFIG_BT_VCS_CLIENT)
		ret = ble_vcs_discover(conn, conn_index);
		if (ret) {
			LOG_ERR("Could not do VCS discover");
		}
#endif /* (CONFIG_BT_VCS_CLIENT) */
		ret = bt_audio_stream_config(conn, &audio_streams[conn_index], sinks[conn_index].ep,
					     &lc3_preset_nrf5340.codec);
		if (ret) {
			LOG_ERR("Could not configure stream");
		}
	}
}

static bool ble_acl_gateway_all_links_connected(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(headset_conn); i++) {
		if (headset_conn[i] == NULL) {
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

		ret = bt_conn_le_create(adv_addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_MULTI,
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

		ret = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_MULTI,
					&conn);
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
	} else if (type == BT_GAP_ADV_TYPE_ADV_IND) {
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

	ret = bt_le_scan_start(BT_LE_SCAN_ACTIVE, on_device_found);
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
	uint8_t conn_index;
	char addr[BT_ADDR_LE_STR_LEN];

	(void)bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Disconnected: %s (reason 0x%02x)", addr, reason);

	bt_conn_unref(conn);

	ret = headset_conn_index_get(conn, &conn_index);
	if (ret) {
		LOG_WRN("Unknown connection");
	} else {
		headset_conn[conn_index] = NULL;
	}

	ble_acl_start_scan();
}

static int discover_sink(struct bt_conn *conn)
{
	int ret = 0;
	static uint8_t conn_index;

	audio_discover_param[conn_index].func = discover_sink_cb;
	audio_discover_param[conn_index].dir = BT_AUDIO_DIR_SINK;
	ret = bt_audio_discover(conn, &audio_discover_param[conn_index]);
	conn_index++;

	/* Avoid multiple discover procedure sharing the same params */
	if (conn_index > CONFIG_BT_ISO_MAX_CHAN - 1) {
		conn_index = 0;
	}

	return ret;
}

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

static int iso_stream_send(uint8_t const *const data, size_t size, uint8_t iso_chan_idx)
{
	int ret;
	static bool wrn_printed[CONFIG_BT_ISO_MAX_CHAN];
	struct net_buf *buf;

	if (audio_streams[iso_chan_idx].ep->status.state != BT_AUDIO_EP_STATE_STREAMING) {
		LOG_DBG("Channel not connected %d", iso_chan_idx);
		return 0;
	}

	if (is_iso_buffer_full(iso_chan_idx)) {
		if (!wrn_printed[iso_chan_idx]) {
			LOG_WRN("HCI ISO TX overrun on ch %d - Single print", iso_chan_idx);
			wrn_printed[iso_chan_idx] = true;
		}

		return -ENOMEM;
	}

	wrn_printed[iso_chan_idx] = false;

	buf = net_buf_alloc(iso_tx_pools[iso_chan_idx], K_NO_WAIT);
	if (buf == NULL) {
		/* This should never occur because of the is_iso_buffer_full() check */
		LOG_WRN("Out of TX buffers");
		return -ENOMEM;
	}

	net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);
	net_buf_add_mem(buf, data, size);

	atomic_inc(&iso_tx_pool_alloc[iso_chan_idx]);

	ret = bt_audio_stream_send(&audio_streams[iso_chan_idx], buf,
				   get_and_incr_seq_num(&audio_streams[iso_chan_idx]),
				   BT_ISO_TIMESTAMP_NONE);
	if (ret < 0) {
		LOG_WRN("Failed to send audio data: %d", ret);
		net_buf_unref(buf);
		atomic_dec(&iso_tx_pool_alloc[iso_chan_idx]);
	}

	return 0;
}

static int initialize(le_audio_receive_cb recv_cb)
{
	int ret;
	static bool initialized;
	struct bt_audio_unicast_group_param group_params[CONFIG_BT_ISO_MAX_CHAN];

	for (int i = 0; i < CONFIG_BT_ISO_MAX_CHAN; i++) {
		k_work_init_delayable(&stream_start_work[i], work_stream_start);
	}

	ret = bt_audio_unicast_client_register_cb(&unicast_client_cbs);
	if (ret != 0) {
		LOG_ERR("Failed to register client callbacks: %d", ret);
		return ret;
	}
#if (CONFIG_BT_VCS_CLIENT)
	ret = ble_vcs_client_init();
	if (ret) {
		LOG_ERR("VCS client init failed");
		return ret;
	}
#endif /* (CONFIG_BT_VCS_CLIENT) */

	ARG_UNUSED(recv_cb);
	if (!initialized) {
		bt_conn_cb_register(&conn_callbacks);
		for (size_t i = 0; i < ARRAY_SIZE(audio_streams); i++) {
			audio_streams[i].ops = &stream_ops;
			group_params[i].stream = &audio_streams[i];
			group_params[i].qos = &lc3_preset_nrf5340.qos;
			group_params[i].dir = BT_AUDIO_DIR_SINK;
		}
		ret = bt_audio_unicast_group_create(group_params, CONFIG_BT_ISO_MAX_CHAN,
						    &unicast_group);

		if (ret) {
			LOG_ERR("Failed to create unicast group: %d", ret);
			return ret;
		}
		initialized = true;
	}
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

int le_audio_play(void)
{
	int ret;

	playing_state = true;

	if (audio_streams[AUDIO_CH_L].ep->status.state == BT_AUDIO_EP_STATE_QOS_CONFIGURED) {
		ret = bt_audio_stream_enable(&audio_streams[AUDIO_CH_L],
					     lc3_preset_nrf5340.codec.meta,
					     lc3_preset_nrf5340.codec.meta_count);

		if (ret) {
			LOG_WRN("Failed to enable left stream");
		}
	}

	if (audio_streams[AUDIO_CH_R].ep->status.state == BT_AUDIO_EP_STATE_QOS_CONFIGURED) {
		ret = bt_audio_stream_enable(&audio_streams[AUDIO_CH_R],
					     lc3_preset_nrf5340.codec.meta,
					     lc3_preset_nrf5340.codec.meta_count);

		if (ret) {
			LOG_WRN("Failed to enable right stream");
		}
	}

	return 0;
}

int le_audio_pause(void)
{
	int ret;

	playing_state = false;

	if (audio_streams[AUDIO_CH_L].ep->status.state == BT_AUDIO_EP_STATE_STREAMING) {
		ret = bt_audio_stream_disable(&audio_streams[AUDIO_CH_L]);

		if (ret) {
			LOG_WRN("Failed to disable left stream");
		}
	}

	if (audio_streams[AUDIO_CH_R].ep->status.state == BT_AUDIO_EP_STATE_STREAMING) {
		ret = bt_audio_stream_disable(&audio_streams[AUDIO_CH_R]);

		if (ret) {
			LOG_WRN("Failed to disable right stream");
		}
	}

	return 0;
}

int le_audio_send(uint8_t const *const data, size_t size)
{
	int ret;
	struct bt_iso_tx_info tx_info = { 0 };
	size_t sdu_size = LE_AUDIO_SDU_SIZE_OCTETS(CONFIG_LC3_BITRATE);

	if (size != sdu_size * CONFIG_BT_ISO_MAX_CHAN) {
		LOG_ERR("Not enough data for stereo stream");
		return -ECANCELED;
	}

	if (audio_streams[AUDIO_CH_L].ep->status.state == BT_AUDIO_EP_STATE_STREAMING) {
		ret = bt_iso_chan_get_tx_sync(audio_streams[AUDIO_CH_L].iso, &tx_info);
	} else if (audio_streams[AUDIO_CH_R].ep->status.state == BT_AUDIO_EP_STATE_STREAMING) {
		ret = bt_iso_chan_get_tx_sync(audio_streams[AUDIO_CH_R].iso, &tx_info);
	} else {
		LOG_DBG("No headset in stream state");
		return -ECANCELED;
	}
	if (ret) {
		LOG_DBG("Error getting ISO TX anchor point: %d", ret);
	}

	if (tx_info.ts != 0 && !ret) {
#if (CONFIG_AUDIO_SOURCE_I2S)
		audio_datapath_sdu_ref_update(tx_info.ts);
#endif
		audio_datapath_just_in_time_check_and_adjust(tx_info.ts);
	}

	ret = iso_stream_send(data, sdu_size, AUDIO_CH_L);
	if (ret) {
		LOG_DBG("Failed to send data to left channel");
	}

	ret = iso_stream_send(&data[sdu_size], sdu_size, AUDIO_CH_R);
	if (ret) {
		LOG_DBG("Failed to send data to right channel");
	}

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
