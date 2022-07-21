/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "le_audio.h"

#include <bluetooth/bluetooth.h>
#include <bluetooth/audio/audio.h>

#include "macros_common.h"
#include "ctrl_events.h"
#include "audio_datapath.h"
#include "ble_audio_services.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(cis_gateway, CONFIG_LOG_BLE_LEVEL);

#define BT_AUDIO_LC3_UNICAST_PRESET_NRF5340_AUDIO                                                  \
	BT_AUDIO_LC3_PRESET(                                                                       \
		BT_CODEC_LC3_CONFIG(BT_CODEC_CONFIG_LC3_FREQ_48KHZ,                                \
				    BT_CODEC_CONFIG_LC3_DURATION_10,                               \
				    LE_AUDIO_SDU_SIZE_OCTETS(CONFIG_LC3_BITRATE),                  \
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

#define DEVICE_NAME_PEER CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_PEER_LEN (sizeof(DEVICE_NAME_PEER) - 1)

static struct bt_conn *default_conn;
static struct bt_audio_stream audio_stream;
static struct bt_audio_unicast_group *unicast_group;
static struct bt_codec *remote_codecs[CONFIG_BT_AUDIO_UNICAST_CLIENT_PAC_COUNT];
static struct bt_audio_ep *sinks[CONFIG_BT_AUDIO_UNICAST_CLIENT_ASE_SNK_COUNT];
/* clang-format off */
static struct net_buf_pool *iso_tx_pools[] = { LISTIFY(CONFIG_BT_ISO_MAX_CHAN,
						       NET_BUF_POOL_PTR_ITERATE, (,)) };
/* clang-format on */
static struct bt_audio_lc3_preset lc3_preset_nrf5340 = BT_AUDIO_LC3_UNICAST_PRESET_NRF5340_AUDIO;

static atomic_t iso_tx_pool_alloc[CONFIG_BT_ISO_MAX_CHAN];

static void ble_acl_start_scan(void);

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

static void stream_sent_cb(struct bt_audio_stream *stream)
{
	atomic_dec(&iso_tx_pool_alloc[0]);
}

static void stream_configured_cb(struct bt_audio_stream *stream,
				 const struct bt_codec_qos_pref *pref)
{
	int ret;

	ret = bt_audio_stream_qos(default_conn, unicast_group);
	if (ret) {
		LOG_ERR("Unable to setup QoS: %d", ret);
	}
}

static void stream_qos_set_cb(struct bt_audio_stream *stream)
{
	int ret;

	ret = bt_audio_stream_enable(stream, lc3_preset_nrf5340.codec.meta,
				     lc3_preset_nrf5340.codec.meta_count);
	if (ret) {
		LOG_ERR("Unable to enable stream: %d", ret);
	}
}

static void stream_enabled_cb(struct bt_audio_stream *stream)
{
	int ret;

	ret = bt_audio_stream_start(stream);
	if (ret) {
		LOG_ERR("Unable to start stream: %d", ret);
	}
}

static void stream_started_cb(struct bt_audio_stream *stream)
{
	int ret;

	LOG_INF("Audio stream %p started", (void *)stream);

	ret = ctrl_events_le_audio_event_send(LE_AUDIO_EVT_STREAMING);
	ERR_CHK(ret);
}

static void stream_metadata_updated_cb(struct bt_audio_stream *stream)
{
	LOG_INF("Audio Stream %p metadata updated", (void *)stream);
}

static void stream_disabled_cb(struct bt_audio_stream *stream)
{
	LOG_INF("Audio Stream %p disabled", (void *)stream);
}

static void stream_stopped_cb(struct bt_audio_stream *stream)
{
	int ret;

	LOG_INF("Audio Stream %p stopped", (void *)stream);

	ret = ctrl_events_le_audio_event_send(LE_AUDIO_EVT_NOT_STREAMING);
	ERR_CHK(ret);

	atomic_clear(&iso_tx_pool_alloc[0]);
}

static void stream_released_cb(struct bt_audio_stream *stream)
{
	LOG_INF("Audio Stream %p released", (void *)stream);
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
		sinks[index] = ep;
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
	int ret;

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
			add_remote_sink(ep, params->num_eps);
		} else {
			LOG_ERR("Invalid param type: %u", params->dir);
		}

		return;
	}

	LOG_INF("Discover complete: err %d", params->err);

	(void)memset(params, 0, sizeof(*params));
	ret = bt_audio_stream_config(default_conn, &audio_stream, sinks[0],
				     &lc3_preset_nrf5340.codec);
	if (ret) {
		LOG_ERR("Could not configure stream");
	}
}

static int device_found(uint8_t type, const uint8_t *data, uint8_t data_len,
			const bt_addr_le_t *addr)
{
	int ret;

	if ((data_len == DEVICE_NAME_PEER_LEN) &&
	    (strncmp(DEVICE_NAME_PEER, data, DEVICE_NAME_PEER_LEN) == 0)) {
		LOG_INF("Device found");
		ret = bt_le_scan_stop();
		if (ret) {
			LOG_WRN("Stop scan failed: %d", ret);
		}

		ret = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_MULTI,
					&default_conn);
		if (ret) {
			LOG_WRN("Create ACL connection failed: %d", ret);
			ble_acl_start_scan();
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
			LOG_INF("AD malformed");
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
	/* Note: May lead to connection creation */
	ad_parse(p_ad, addr);
}

static void ble_acl_start_scan(void)
{
	int ret;

	ret = bt_le_scan_start(BT_LE_SCAN_PASSIVE, on_device_found);
	if (ret) {
		LOG_INF("Scanning failed to start: %d", ret);
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
		LOG_ERR("Failed to connect to %s: %d", addr, err);

		bt_conn_unref(default_conn);
		default_conn = NULL;

		ble_acl_start_scan();
		return;
	}

	if (conn != default_conn) {
		return;
	}

	LOG_INF("Connected: %s", addr);
	ret = bt_conn_set_security(conn, BT_SECURITY_L2);
	if (ret) {
		LOG_ERR("Failed to set security to L2: %d", ret);
	}

	/* TODO: Channel nr is 0 for now, to be changed with dual CIS */
	ble_vcs_discover(conn, 0);
}

static void disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (conn != default_conn) {
		return;
	}

	(void)bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Disconnected: %s (reason 0x%02x)", addr, reason);

	bt_conn_unref(default_conn);
	default_conn = NULL;

	ble_acl_start_scan();
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
		LOG_INF("Security changed: level %d", level);
		static struct bt_audio_discover_params dis_params;

		dis_params.func = discover_sink_cb;
		dis_params.dir = BT_AUDIO_DIR_SINK;

		ret = bt_audio_discover(default_conn, &dis_params);
		if (ret) {
			LOG_INF("Failed to discover sink: %d", ret);
		}
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected_cb,
	.disconnected = disconnected_cb,
	.security_changed = security_changed_cb,
};

static int initialize(le_audio_receive_cb recv_cb)
{
	int ret;
	static bool initialized;
	struct bt_audio_unicast_group_param params;

#if (CONFIG_BT_VCS_CLIENT)
	ret = ble_vcs_client_init();
	if (ret) {
		LOG_ERR("VCS client init failed");
		return ret;
	}
#endif /* (CONFIG_BT_VCS_CLIENT) */

	ARG_UNUSED(recv_cb);
	if (!initialized) {
		audio_stream.ops = &stream_ops;
		params.stream = &audio_stream;
		params.qos = &lc3_preset_unicast_nrf5340.qos;
		params.dir = BT_AUDIO_DIR_SINK;
		ret = bt_audio_unicast_group_create(&params, CONFIG_BT_ISO_MAX_CHAN,
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
	return 0;
}

int le_audio_pause(void)
{
	return 0;
}

int le_audio_send(uint8_t const *const data, size_t size)
{
	int ret;
	static bool wrn_printed[CONFIG_BT_ISO_MAX_CHAN];
	struct net_buf *buf;

	if (is_iso_buffer_full(0)) {
		if (!wrn_printed[0]) {
			LOG_WRN("HCI ISO TX overrun on ch %d - Single print", 0);
			wrn_printed[0] = true;
		}

		return -ENOMEM;
	}

	wrn_printed[0] = false;

	buf = net_buf_alloc(iso_tx_pools[0], K_NO_WAIT);
	if (buf == NULL) {
		/* This should never occur because of the is_iso_buffer_full() check */
		LOG_WRN("Out of TX buffers");
		return -ENOMEM;
	}

	net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);

	//TODO: Handling dual channel sending properly
	net_buf_add_mem(buf, data, LE_AUDIO_SDU_SIZE_OCTETS(CONFIG_LC3_BITRATE));

	struct bt_iso_tx_info tx_info = { 0 };

	ret = bt_iso_chan_get_tx_sync(audio_stream.iso, &tx_info);

	if (ret) {
		LOG_WRN("Error getting ISO TX anchor point: %d", ret);
	}

	if (tx_info.ts != 0 && !ret) {
#if (CONFIG_AUDIO_SOURCE_I2S)
		audio_datapath_sdu_ref_update(tx_info.ts);
#endif
		audio_datapath_just_in_time_check_and_adjust(tx_info.ts);
	}

	atomic_inc(&iso_tx_pool_alloc[0]);

	ret = bt_audio_stream_send(&audio_stream, buf);
	if (ret < 0) {
		LOG_WRN("Failed to send audio data: %d", ret);
		net_buf_unref(buf);
		atomic_dec(&iso_tx_pool_alloc[0]);
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
