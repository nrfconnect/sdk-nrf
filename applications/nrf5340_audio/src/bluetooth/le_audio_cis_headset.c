/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#if (CONFIG_AUDIO_DEV == HEADSET)

#include "le_audio.h"

#include <bluetooth/audio/audio.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/audio/audio.h>
#include <bluetooth/audio/capabilities.h>

#include "macros_common.h"
#include "ctrl_events.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(cis_headset, CONFIG_LOG_BLE_LEVEL);

#define CHANNEL_COUNT_1 BIT(0)
#define BLE_ISO_LATENCY_MS 10
#define BLE_ISO_RETRANSMITS 2
#define BLE_ISO_PRSENTATION_DELAY_MIN_US 10000
#define BLE_ISO_PRSENTATION_DELAY_MAX_US 40000
#define BLE_ISO_PREF_PRSENTATION_DELAY_MIN_US 10000
#define BLE_ISO_PREF_PRSENTATION_DELAY_MAX_US 40000
#define DEVICE_NAME_PEER CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_PEER_LEN (sizeof(DEVICE_NAME_PEER) - 1)

static le_audio_receive_cb receive_cb;
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME_PEER, DEVICE_NAME_PEER_LEN),
};
static struct bt_audio_capability_ops lc3_cap_codec_ops;
static struct bt_codec lc3_codec =
	BT_CODEC_LC3(BT_CODEC_LC3_FREQ_ANY, BT_CODEC_LC3_DURATION_10, CHANNEL_COUNT_1, 40u, 120u,
		     1u, BT_AUDIO_CONTEXT_TYPE_MEDIA, BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
static struct bt_audio_capability caps = {
	.dir = BT_AUDIO_DIR_SINK,
	.pref = BT_AUDIO_CAPABILITY_PREF(
		BT_AUDIO_CAPABILITY_UNFRAMED_SUPPORTED, BT_GAP_LE_PHY_2M, BLE_ISO_RETRANSMITS,
		BLE_ISO_LATENCY_MS, BLE_ISO_PRSENTATION_DELAY_MIN_US,
		BLE_ISO_PRSENTATION_DELAY_MAX_US, BLE_ISO_PREF_PRSENTATION_DELAY_MIN_US,
		BLE_ISO_PREF_PRSENTATION_DELAY_MAX_US),
	.codec = &lc3_codec,
	.ops = &lc3_cap_codec_ops,
};
static struct bt_conn *default_conn;
static struct bt_audio_stream audio_stream;

static void print_codec(const struct bt_codec *codec)
{
	if (codec->id == BT_CODEC_LC3_ID) {
		/* LC3 uses the generic LTV format - other codecs might do as well */
		uint32_t chan_allocation;

		LOG_INF("Codec config for LC3:");
		LOG_INF("\tFrequency: %d Hz", bt_codec_cfg_get_freq(codec));
		LOG_INF("\tFrame Duration: %d us", bt_codec_cfg_get_frame_duration_us(codec));
		if (bt_codec_cfg_get_chan_allocation_val(codec, &chan_allocation) == 0) {
			LOG_INF("\tChannel allocation: 0x%x", chan_allocation);
		}

		LOG_INF("\tOctets per frame: %d", bt_codec_cfg_get_octets_per_frame(codec));
		LOG_INF("\tFrames per SDU: %d", bt_codec_cfg_get_frame_blocks_per_sdu(codec, true));
	} else {
		LOG_INF("Codec is not LC3, codec_id: 0x%2x", codec->id);
	}
}

static struct bt_audio_stream *lc3_cap_config_cb(struct bt_conn *conn, struct bt_audio_ep *ep,
						 enum bt_audio_dir dir,
						 struct bt_audio_capability *cap,
						 struct bt_codec *codec)
{
	int ret;
	struct bt_audio_stream *stream = &audio_stream;

	if (!stream->conn) {
		LOG_INF("ASE Codec Config stream %p", (void *)stream);

		print_codec(stream->codec);

		ret = ctrl_events_le_audio_event_send(LE_AUDIO_EVT_CONFIG_RECEIVED);
		ERR_CHK(ret);

		return stream;
	}

	LOG_WRN("No audio_stream available");
	return NULL;
}

static int lc3_cap_reconfig_cb(struct bt_audio_stream *stream, struct bt_audio_capability *cap,
			       struct bt_codec *codec)
{
	LOG_INF("ASE Codec Reconfig: stream %p cap %p", (void *)stream, (void *)cap);

	/* We only support one QoS at the moment, reject changes */
	return -ENOEXEC;
}

static int lc3_cap_qos_cb(struct bt_audio_stream *stream, struct bt_codec_qos *qos)
{
	LOG_INF("QoS: stream %p qos %p", (void *)stream, (void *)qos);

	return 0;
}

static int lc3_cap_enable_cb(struct bt_audio_stream *stream, struct bt_codec_data *meta,
			     size_t meta_count)
{
	int ret;

	LOG_INF("Enable: stream %p meta_count %u", (void *)stream, meta_count);

	ret = ctrl_events_le_audio_event_send(LE_AUDIO_EVT_STREAMING);
	ERR_CHK(ret);

	return 0;
}

static int lc3_cap_start_cb(struct bt_audio_stream *stream)
{
	LOG_INF("Start: stream %p", (void *)stream);
	return 0;
}

static int lc3_cap_metadata_cb(struct bt_audio_stream *stream, struct bt_codec_data *meta,
			       size_t meta_count)
{
	LOG_INF("Metadata: stream %p meta_count %u", (void *)stream, meta_count);
	return 0;
}

static int lc3_cap_disable_cb(struct bt_audio_stream *stream)
{
	LOG_INF("Disable: stream %p", (void *)stream);
	return 0;
}

static int lc3_cap_stop_cb(struct bt_audio_stream *stream)
{
	int ret;

	LOG_INF("Stop: stream %p", (void *)stream);

	ret = ctrl_events_le_audio_event_send(LE_AUDIO_EVT_NOT_STREAMING);
	ERR_CHK(ret);

	return 0;
}

static int lc3_cap_release_cb(struct bt_audio_stream *stream)
{
	LOG_INF("Release: stream %p", (void *)stream);
	return 0;
}

static struct bt_audio_capability_ops lc3_cap_codec_ops = {
	.config = lc3_cap_config_cb,
	.reconfig = lc3_cap_reconfig_cb,
	.qos = lc3_cap_qos_cb,
	.enable = lc3_cap_enable_cb,
	.start = lc3_cap_start_cb,
	.metadata = lc3_cap_metadata_cb,
	.disable = lc3_cap_disable_cb,
	.stop = lc3_cap_stop_cb,
	.release = lc3_cap_release_cb,
};

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
		LOG_INF("Received %u total ISO packets", recv_cnt);
	}
}

static void stream_stop_cb(struct bt_audio_stream *stream)
{
	int ret;

	LOG_INF("Stop: stream %p", (void *)stream);

	ret = ctrl_events_le_audio_event_send(LE_AUDIO_EVT_NOT_STREAMING);
	ERR_CHK(ret);
}

static void connected_cb(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (err) {
		default_conn = NULL;
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_INF("Connected: %s", addr);
	default_conn = bt_conn_ref(conn);
}

static void disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (conn != default_conn) {
		LOG_WRN("Disconnected on wrong conn");
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Disconnected: %s (reason 0x%02x)", addr, reason);

	bt_conn_unref(default_conn);
	default_conn = NULL;
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected_cb,
	.disconnected = disconnected_cb,
};

static struct bt_audio_stream_ops stream_ops = { .recv = stream_recv_cb,
						 .stopped = stream_stop_cb };

static int initialize(le_audio_receive_cb recv_cb)
{
	int ret;
	static bool initialized;

	if (!initialized) {
		receive_cb = recv_cb;
		ret = bt_audio_capability_register(&caps);
		if (ret) {
			LOG_ERR("Capability register failed");
			return ret;
		}
		ret = bt_audio_capability_set_location(BT_AUDIO_DIR_SINK,
						       BT_AUDIO_LOCATION_SIDE_LEFT);
		if (ret) {
			LOG_ERR("Location set failed");
			return ret;
		}

		ret = bt_audio_capability_set_available_contexts(
			BT_AUDIO_DIR_SINK,
			BT_AUDIO_CONTEXT_TYPE_MEDIA | BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
		if (ret) {
			LOG_ERR("Available contexte set failed");
			return ret;
		}

		bt_audio_stream_cb_register(&audio_stream, &stream_ops);
		initialized = true;
	}
	return 0;
}

int le_audio_config_get(uint32_t *bitrate, uint32_t *sampling_rate)
{
	int frames_per_sec = 1000000 / bt_codec_cfg_get_frame_duration_us(audio_stream.codec);
	int bits_per_frame = bt_codec_cfg_get_octets_per_frame(audio_stream.codec) * 8;

	*sampling_rate = bt_codec_cfg_get_freq(audio_stream.codec);
	*bitrate = frames_per_sec * bits_per_frame;
	return 0;
}

int le_audio_volume_up(void)
{
	return 0;
}

int le_audio_volume_down(void)
{
	return 0;
}

int le_audio_volume_mute(void)
{
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
	return 0;
}

int le_audio_enable(le_audio_receive_cb recv_cb)
{
	int ret;

	ret = initialize(recv_cb);
	if (ret) {
		LOG_ERR("Initialize failed");
		return ret;
	}
	ret = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), NULL, 0);

	if (ret) {
		LOG_ERR("Advertising failed to start: %d", ret);
		return ret;
	}

	LOG_INF("Advertising successfully started");
	return 0;
}

int le_audio_disable(void)
{
	return 0;
}

#endif /* (CONFIG_AUDIO_DEV == HEADSET) */
