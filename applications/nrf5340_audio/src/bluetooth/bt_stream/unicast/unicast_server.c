/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "unicast_server.h"

#include <zephyr/zbus/zbus.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/pacs.h>
#include <zephyr/bluetooth/audio/csip.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/lc3.h>

/* TODO: Remove when a get_info function is implemented in host */
#include <../subsys/bluetooth/audio/bap_endpoint.h>

#include "macros_common.h"
#include "nrf5340_audio_common.h"
#include "channel_assignment.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(unicast_server, CONFIG_UNICAST_SERVER_LOG_LEVEL);

BUILD_ASSERT(CONFIG_BT_ASCS_ASE_SRC_COUNT <= 1,
	     "A maximum of one source stream is currently supported");

ZBUS_CHAN_DEFINE(le_audio_chan, struct le_audio_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
		 ZBUS_MSG_INIT(0));

#define BLE_ISO_LATENCY_MS  10
#define BLE_ISO_RETRANSMITS 2

#if (CONFIG_BT_AUDIO_TX)
#define HCI_ISO_BUF_ALLOC_PER_CHAN 2
/* For being able to dynamically define iso_tx_pools */
#define NET_BUF_POOL_ITERATE(i, _)                                                                 \
	NET_BUF_POOL_FIXED_DEFINE(iso_tx_pool_##i, HCI_ISO_BUF_ALLOC_PER_CHAN,                     \
				  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU), 8, NULL);
#define NET_BUF_POOL_PTR_ITERATE(i, ...) IDENTITY(&iso_tx_pool_##i)
LISTIFY(CONFIG_BT_ASCS_ASE_SRC_COUNT, NET_BUF_POOL_ITERATE, (;))

static atomic_t iso_tx_pool_alloc;
/* clang-format off */
static struct net_buf_pool *iso_tx_pools[] = { LISTIFY(CONFIG_BT_ASCS_ASE_SRC_COUNT,
							   NET_BUF_POOL_PTR_ITERATE, (,)) };
/* clang-format on */
#endif /* (CONFIG_BT_AUDIO_TX) */

#define CSIP_SET_SIZE 2
enum csip_set_rank {
	CSIP_HL_RANK = 1,
	CSIP_HR_RANK = 2
};

static le_audio_receive_cb receive_cb;
static struct bt_csip_set_member_svc_inst *csip;
/* Left or right channel headset */
static enum audio_channel channel;

/* Advertising data for peer connection */
static uint8_t csip_rsi_adv_data[BT_CSIP_RSI_SIZE];

static uint8_t flags_adv_data[] = {BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR};

static uint8_t gap_appear_adv_data[] = {BT_BYTES_LIST_LE16(CONFIG_BT_DEVICE_APPEARANCE)};

static const uint8_t cap_adv_data[] = {
	BT_UUID_16_ENCODE(BT_UUID_CAS_VAL),
	BT_AUDIO_UNICAST_ANNOUNCEMENT_TARGETED,
};

#define AVAILABLE_SOURCE_CONTEXT                                                                   \
	(BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED | BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL)

#define AVAILABLE_SINK_CONTEXT                                                                     \
	(BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED | BT_AUDIO_CONTEXT_TYPE_MEDIA |                         \
	 BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL)

static uint8_t unicast_server_adv_data[] = {
	BT_UUID_16_ENCODE(BT_UUID_ASCS_VAL),
	BT_AUDIO_UNICAST_ANNOUNCEMENT_TARGETED,
#if defined(CONFIG_BT_AUDIO_RX)
	BT_BYTES_LIST_LE16(AVAILABLE_SINK_CONTEXT),
#endif /* CONFIG_BT_AUDIO_RX */
#if defined(CONFIG_BT_AUDIO_TX)
	BT_BYTES_LIST_LE16(AVAILABLE_SOURCE_CONTEXT),
#endif	      /* CONFIG_BT_AUDIO_TX */
	0x00, /* Metadata length */
};

static void le_audio_event_publish(enum le_audio_evt_type event, struct bt_conn *conn)
{
	int ret;
	struct le_audio_msg msg;

	msg.event = event;
	msg.conn = conn;

	ret = zbus_chan_pub(&le_audio_chan, &msg, LE_AUDIO_ZBUS_EVENT_WAIT_TIME);
	ERR_CHK(ret);
}

/* Callback for locking state change from server side */
static void csip_lock_changed_cb(struct bt_conn *conn, struct bt_csip_set_member_svc_inst *csip,
				 bool locked)
{
	LOG_DBG("Client %p %s the lock", (void *)conn, locked ? "locked" : "released");
}

/* Callback for SIRK read request from peer side */
static uint8_t sirk_read_req_cb(struct bt_conn *conn, struct bt_csip_set_member_svc_inst *csip)
{
	/* Accept the request to read the SIRK, but return encrypted SIRK instead of plaintext */
	return BT_CSIP_READ_SIRK_REQ_RSP_ACCEPT_ENC;
}

static struct bt_csip_set_member_cb csip_callbacks = {
	.lock_changed = csip_lock_changed_cb,
	.sirk_read_req = sirk_read_req_cb,
};
struct bt_csip_set_member_register_param csip_param = {
	.set_size = CSIP_SET_SIZE,
	.lockable = true,
#if !CONFIG_BT_CSIP_SET_MEMBER_TEST_SAMPLE_DATA
	/* CSIP SIRK for demo is used, must be changed before production */
	.set_sirk = {'N', 'R', 'F', '5', '3', '4', '0', '_', 'T', 'W', 'S', '_', 'D', 'E', 'M',
		     'O'},
#else
#warning "CSIP test sample data is used, must be changed before production"
#endif
	.cb = &csip_callbacks,
};

static struct bt_audio_codec_cap lc3_codec = BT_AUDIO_CODEC_CAP_LC3(
	BT_AUDIO_CODEC_CAPABILIY_FREQ,
	(BT_AUDIO_CODEC_LC3_DURATION_10 | BT_AUDIO_CODEC_LC3_DURATION_PREFER_10),
	BT_AUDIO_CODEC_LC3_CHAN_COUNT_SUPPORT(1), LE_AUDIO_SDU_SIZE_OCTETS(CONFIG_LC3_BITRATE_MIN),
	LE_AUDIO_SDU_SIZE_OCTETS(CONFIG_LC3_BITRATE_MAX), 1u, BT_AUDIO_CONTEXT_TYPE_MEDIA);

static enum bt_audio_dir caps_dirs[] = {
	BT_AUDIO_DIR_SINK,
#if (CONFIG_BT_AUDIO_TX)
	BT_AUDIO_DIR_SOURCE,
#endif /* (CONFIG_BT_AUDIO_TX) */
};

static const struct bt_audio_codec_qos_pref qos_pref = BT_AUDIO_CODEC_QOS_PREF(
	true, BT_GAP_LE_PHY_2M, BLE_ISO_RETRANSMITS, BLE_ISO_LATENCY_MS,
	CONFIG_AUDIO_MIN_PRES_DLY_US, CONFIG_AUDIO_MAX_PRES_DLY_US,
	CONFIG_BT_AUDIO_PREFERRED_MIN_PRES_DLY_US, CONFIG_BT_AUDIO_PREFERRED_MAX_PRES_DLY_US);

/* clang-format off */
static struct bt_pacs_cap caps[] = {
#if (CONFIG_BT_AUDIO_RX)
				{
					 .codec_cap = &lc3_codec,
				},
#endif
#if (CONFIG_BT_AUDIO_TX)
				{
					 .codec_cap = &lc3_codec,
				}
#endif /* (CONFIG_BT_AUDIO_TX) */
};
/* clang-format on */

static struct bt_bap_stream
	audio_streams[CONFIG_BT_ASCS_ASE_SNK_COUNT + CONFIG_BT_ASCS_ASE_SRC_COUNT];

#if (CONFIG_BT_AUDIO_TX)
BUILD_ASSERT(CONFIG_BT_ASCS_ASE_SRC_COUNT <= 1,
	     "CIS headset only supports one source stream for now");
static struct bt_audio_source {
	struct bt_bap_stream *stream;
	uint32_t seq_num;
} sources[CONFIG_BT_ASCS_ASE_SRC_COUNT];
#endif /* (CONFIG_BT_AUDIO_TX) */

static void print_codec(const struct bt_audio_codec_cfg *codec, enum bt_audio_dir dir)
{
	if (codec->id == BT_AUDIO_CODEC_LC3_ID) {
		/* LC3 uses the generic LTV format - other codecs might do as well */
		enum bt_audio_location chan_allocation;

		if (dir == BT_AUDIO_DIR_SINK) {
			LOG_INF("LC3 codec config for sink:");
		} else if (dir == BT_AUDIO_DIR_SOURCE) {
			LOG_INF("LC3 codec config for source:");
		} else {
			LOG_INF("LC3 codec config for <unknown dir>:");
		}

		LOG_INF("\tFrequency: %d Hz", bt_audio_codec_cfg_get_freq(codec));
		LOG_INF("\tFrame Duration: %d us", bt_audio_codec_cfg_get_frame_duration_us(codec));
		if (bt_audio_codec_cfg_get_chan_allocation_val(codec, &chan_allocation) == 0) {
			LOG_INF("\tChannel allocation: 0x%x", chan_allocation);
		}

		uint32_t octets_per_sdu = bt_audio_codec_cfg_get_octets_per_frame(codec);
		uint32_t bitrate = octets_per_sdu * 8 *
				   (1000000 / bt_audio_codec_cfg_get_frame_duration_us(codec));

		LOG_INF("\tOctets per frame: %d (%d bps)", octets_per_sdu, bitrate);
		LOG_INF("\tFrames per SDU: %d",
			bt_audio_codec_cfg_get_frame_blocks_per_sdu(codec, true));
	} else {
		LOG_WRN("Codec is not LC3, codec_id: 0x%2x", codec->id);
	}
}

static int lc3_config_cb(struct bt_conn *conn, const struct bt_bap_ep *ep, enum bt_audio_dir dir,
			 const struct bt_audio_codec_cfg *codec, struct bt_bap_stream **stream,
			 struct bt_audio_codec_qos_pref *const pref, struct bt_bap_ascs_rsp *rsp)
{
	LOG_DBG("LC3 config callback");

	for (int i = 0; i < ARRAY_SIZE(audio_streams); i++) {
		struct bt_bap_stream *audio_stream = &audio_streams[i];

		if (!audio_stream->conn) {
			LOG_DBG("ASE Codec Config stream %p", (void *)audio_stream);

			uint32_t octets_per_sdu = bt_audio_codec_cfg_get_octets_per_frame(codec);

			if (octets_per_sdu > LE_AUDIO_SDU_SIZE_OCTETS(CONFIG_LC3_BITRATE_MAX)) {
				LOG_WRN("Too high bitrate");
				return -EINVAL;
			} else if (octets_per_sdu <
				   LE_AUDIO_SDU_SIZE_OCTETS(CONFIG_LC3_BITRATE_MIN)) {
				LOG_WRN("Too low bitrate");
				return -EINVAL;
			}

			if (dir == BT_AUDIO_DIR_SINK) {
				LOG_DBG("BT_AUDIO_DIR_SINK");
				print_codec(codec, dir);
				le_audio_event_publish(LE_AUDIO_EVT_CONFIG_RECEIVED, conn);
			}
#if (CONFIG_BT_AUDIO_TX)
			else if (dir == BT_AUDIO_DIR_SOURCE) {
				LOG_DBG("BT_AUDIO_DIR_SOURCE");
				print_codec(codec, dir);
				le_audio_event_publish(LE_AUDIO_EVT_CONFIG_RECEIVED, conn);

				/* CIS headset only supports one source stream for now */
				sources[0].stream = audio_stream;
			}
#endif /* (CONFIG_BT_AUDIO_TX) */
			else {
				LOG_ERR("UNKNOWN DIR");
				return -EINVAL;
			}

			*stream = audio_stream;
			*pref = qos_pref;

			return 0;
		}
	}

	LOG_WRN("No audio_stream available");
	return -ENOMEM;
}

static int lc3_reconfig_cb(struct bt_bap_stream *stream, enum bt_audio_dir dir,
			   const struct bt_audio_codec_cfg *codec,
			   struct bt_audio_codec_qos_pref *const pref, struct bt_bap_ascs_rsp *rsp)
{
	LOG_DBG("ASE Codec Reconfig: stream %p", (void *)stream);

	return 0;
}

static int lc3_qos_cb(struct bt_bap_stream *stream, const struct bt_audio_codec_qos *qos,
		      struct bt_bap_ascs_rsp *rsp)
{
	le_audio_event_publish(LE_AUDIO_EVT_PRES_DELAY_SET, stream->conn);

	LOG_DBG("QoS: stream %p qos %p", (void *)stream, (void *)qos);

	return 0;
}

static int lc3_enable_cb(struct bt_bap_stream *stream, const uint8_t *meta, size_t meta_len,
			 struct bt_bap_ascs_rsp *rsp)
{
	LOG_DBG("Enable: stream %p meta_len %d", (void *)stream, meta_len);

	return 0;
}

static int lc3_start_cb(struct bt_bap_stream *stream, struct bt_bap_ascs_rsp *rsp)
{
	LOG_DBG("Start stream %p", (void *)stream);
	return 0;
}

static int lc3_metadata_cb(struct bt_bap_stream *stream, const uint8_t *meta, size_t meta_len,
			   struct bt_bap_ascs_rsp *rsp)
{
	LOG_DBG("Metadata: stream %p meta_len %d", (void *)stream, meta_len);
	return 0;
}

static int lc3_disable_cb(struct bt_bap_stream *stream, struct bt_bap_ascs_rsp *rsp)
{
	LOG_DBG("Disable: stream %p", (void *)stream);

	le_audio_event_publish(LE_AUDIO_EVT_NOT_STREAMING, stream->conn);

	return 0;
}

static int lc3_stop_cb(struct bt_bap_stream *stream, struct bt_bap_ascs_rsp *rsp)
{
	LOG_DBG("Stop: stream %p", (void *)stream);

	le_audio_event_publish(LE_AUDIO_EVT_NOT_STREAMING, stream->conn);

	return 0;
}

static int lc3_release_cb(struct bt_bap_stream *stream, struct bt_bap_ascs_rsp *rsp)
{
	LOG_DBG("Release: stream %p", (void *)stream);

	le_audio_event_publish(LE_AUDIO_EVT_NOT_STREAMING, stream->conn);

	return 0;
}

static const struct bt_bap_unicast_server_cb unicast_server_cb = {
	.config = lc3_config_cb,
	.reconfig = lc3_reconfig_cb,
	.qos = lc3_qos_cb,
	.enable = lc3_enable_cb,
	.start = lc3_start_cb,
	.metadata = lc3_metadata_cb,
	.disable = lc3_disable_cb,
	.stop = lc3_stop_cb,
	.release = lc3_release_cb,
};

#if (CONFIG_BT_AUDIO_RX)
static void stream_recv_cb(struct bt_bap_stream *stream, const struct bt_iso_recv_info *info,
			   struct net_buf *buf)
{
	bool bad_frame = false;

	if (receive_cb == NULL) {
		LOG_ERR("The RX callback has not been set");
		return;
	}

	if (!(info->flags & BT_ISO_FLAGS_VALID)) {
		bad_frame = true;
	}

	receive_cb(buf->data, buf->len, bad_frame, info->ts, channel,
		   bt_audio_codec_cfg_get_octets_per_frame(stream->codec_cfg));
}
#endif /* (CONFIG_BT_AUDIO_RX) */

#if (CONFIG_BT_AUDIO_TX)
static void stream_sent_cb(struct bt_bap_stream *stream)
{
	atomic_dec(&iso_tx_pool_alloc);
}
#endif /* (CONFIG_BT_AUDIO_TX) */

static void stream_enabled_cb(struct bt_bap_stream *stream)
{
	int ret;
	LOG_DBG("Stream %p enabled", stream);

	if (stream->ep->dir == BT_AUDIO_DIR_SINK) {
		/* Automatically do the receiver start ready operation */
		ret = bt_bap_stream_start(stream);
		if (ret != 0) {
			LOG_ERR("Failed to start stream: %d", ret);
			return;
		}
	}
}

static void stream_disabled_cb(struct bt_bap_stream *stream)
{
	LOG_INF("Stream %p disabled", stream);
}

static void stream_started_cb(struct bt_bap_stream *stream)
{
	LOG_INF("Stream %p started", stream);

	le_audio_event_publish(LE_AUDIO_EVT_STREAMING, stream->conn);
}

static void stream_stopped_cb(struct bt_bap_stream *stream, uint8_t reason)
{
	LOG_DBG("Stream %p stopped. Reason: %d", stream, reason);

#if (CONFIG_BT_AUDIO_TX)
	atomic_clear(&iso_tx_pool_alloc);
#endif /* (CONFIG_BT_AUDIO_TX) */

	le_audio_event_publish(LE_AUDIO_EVT_NOT_STREAMING, stream->conn);
}

static void stream_released_cb(struct bt_bap_stream *stream)
{
	/* NOTE: The string below is used by the Nordic CI system */
	LOG_INF("Stream %p released", stream);
}

static struct bt_bap_stream_ops stream_ops = {
#if (CONFIG_BT_AUDIO_RX)
	.recv = stream_recv_cb,
#endif /* (CONFIG_BT_AUDIO_RX) */
#if (CONFIG_BT_AUDIO_TX)
	.sent = stream_sent_cb,
#endif /* (CONFIG_BT_AUDIO_TX) */
	.enabled = stream_enabled_cb,
	.disabled = stream_disabled_cb,
	.started = stream_started_cb,
	.stopped = stream_stopped_cb,
	.released = stream_released_cb,
};

static int adv_buf_put(struct bt_data *adv_buf, uint8_t adv_buf_vacant, int *index, uint8_t type,
		       size_t data_len, const uint8_t *data)
{
	if ((adv_buf_vacant - *index) <= 0) {
		return -ENOMEM;
	}

	adv_buf[*index].type = type;
	adv_buf[*index].data_len = data_len;
	adv_buf[*index].data = data;
	(*index)++;

	return 0;
}

int unicast_server_config_get(uint32_t *bitrate, uint32_t *sampling_rate_hz,
			      uint32_t *pres_delay_us)
{
	if (bitrate == NULL && sampling_rate_hz == NULL && pres_delay_us == NULL) {
		LOG_ERR("No valid pointers received");
		return -ENXIO;
	}

	if (audio_streams[0].codec_cfg == NULL) {
		LOG_ERR("No codec found for the stream");
		return -ENXIO;
	}

	if (sampling_rate_hz != NULL) {
		*sampling_rate_hz = bt_audio_codec_cfg_get_freq(audio_streams[0].codec_cfg);
	}

	if (bitrate != NULL) {
		/* Get the configuration for the sink stream */
		int frames_per_sec = 1000000 / bt_audio_codec_cfg_get_frame_duration_us(
						       audio_streams[0].codec_cfg);
		int bits_per_frame =
			bt_audio_codec_cfg_get_octets_per_frame(audio_streams[0].codec_cfg) * 8;

		*bitrate = frames_per_sec * bits_per_frame;
	}

	if (pres_delay_us != NULL) {
		if (audio_streams[0].qos == NULL) {
			LOG_ERR("No QoS found for the stream");
			return -ENXIO;
		}

		*pres_delay_us = audio_streams[0].qos->pd;
	}

	return 0;
}

int unicast_server_uuid_populate(struct net_buf_simple *uuid_buf)
{
	if (net_buf_simple_tailroom(uuid_buf) >= (BT_UUID_SIZE_16 * 2)) {
		net_buf_simple_add_le16(uuid_buf, BT_UUID_ASCS_VAL);
		net_buf_simple_add_le16(uuid_buf, BT_UUID_PACS_VAL);

	} else {
		LOG_ERR("Not enough space for UUIDS");
		return -ENOMEM;
	}

	return 0;
}

int unicast_server_adv_populate(struct bt_data *adv_buf, uint8_t adv_buf_vacant)
{
	int ret;
	int adv_buf_cnt = 0;

	ret = adv_buf_put(adv_buf, adv_buf_vacant, &adv_buf_cnt, BT_DATA_SVC_DATA16,
			  ARRAY_SIZE(unicast_server_adv_data), &unicast_server_adv_data[0]);
	if (ret) {
		return ret;
	}

	if (IS_ENABLED(CONFIG_BT_CSIP_SET_MEMBER)) {
		ret = adv_buf_put(adv_buf, adv_buf_vacant, &adv_buf_cnt, BT_DATA_CSIS_RSI,
				  ARRAY_SIZE(csip_rsi_adv_data), &csip_rsi_adv_data[0]);
		if (ret) {
			return ret;
		}
	}

	ret = adv_buf_put(adv_buf, adv_buf_vacant, &adv_buf_cnt, BT_DATA_GAP_APPEARANCE,
			  ARRAY_SIZE(gap_appear_adv_data), &gap_appear_adv_data[0]);
	if (ret) {
		return ret;
	}

	ret = adv_buf_put(adv_buf, adv_buf_vacant, &adv_buf_cnt, BT_DATA_FLAGS,
			  ARRAY_SIZE(flags_adv_data), &flags_adv_data[0]);
	if (ret) {
		return ret;
	}

	ret = adv_buf_put(adv_buf, adv_buf_vacant, &adv_buf_cnt, BT_DATA_SVC_DATA16,
			  ARRAY_SIZE(cap_adv_data), &cap_adv_data[0]);
	if (ret) {
		return ret;
	}

	return adv_buf_cnt;
}

int unicast_server_send(struct le_audio_encoded_audio enc_audio)
{
#if (CONFIG_BT_AUDIO_TX)
	int ret;
	struct net_buf *buf;
	static bool hci_wrn_printed;

	if (enc_audio.num_ch > CONFIG_BT_ASCS_ASE_SRC_COUNT) {
		LOG_ERR("Trying to send %d channel(s), but %d ASEs are configured for source",
			enc_audio.num_ch, CONFIG_BT_ASCS_ASE_SRC_COUNT);
		return -EINVAL;
	}

	/* CIS headset only supports one source stream for now */
	if (sources[0].stream->ep->status.state != BT_BAP_EP_STATE_STREAMING) {
		LOG_DBG("Return channel not connected");
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
	if (atomic_get(&iso_tx_pool_alloc) >= HCI_ISO_BUF_ALLOC_PER_CHAN) {
		if (!hci_wrn_printed) {
			LOG_WRN("HCI ISO TX overrun");
			hci_wrn_printed = true;
		}

		return -ENOMEM;
	}

	hci_wrn_printed = false;

	buf = net_buf_alloc(iso_tx_pools[0], K_NO_WAIT);
	if (buf == NULL) {
		/* This should never occur because of the iso_tx_pool_alloc
		 * check above
		 */
		LOG_WRN("Out of TX buffers");
		return -ENOMEM;
	}

	net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);
	net_buf_add_mem(buf, enc_audio.data, enc_audio.size);

	atomic_inc(&iso_tx_pool_alloc);
	ret = bt_bap_stream_send(sources[0].stream, buf, sources[0].seq_num++,
				 BT_ISO_TIMESTAMP_NONE);
	if (ret < 0) {
		LOG_WRN("Failed to send audio data: %d", ret);
		net_buf_unref(buf);
		atomic_dec(&iso_tx_pool_alloc);
		return ret;
	}

	return 0;
#else
	return -ENOTSUP;
#endif /* (CONFIG_BT_AUDIO_TX) */
}

int unicast_server_disable(void)
{
	return -ENOTSUP;
}

int unicast_server_enable(le_audio_receive_cb recv_cb)
{
	int ret;
	static bool initialized;

	if (initialized) {
		LOG_WRN("Already initialized");
		return -EALREADY;
	}

	if (recv_cb == NULL) {
		LOG_ERR("Receive callback is NULL");
		return -EINVAL;
	}

	receive_cb = recv_cb;

	bt_bap_unicast_server_register_cb(&unicast_server_cb);

	channel_assignment_get(&channel);

	for (int i = 0; i < ARRAY_SIZE(caps); i++) {
		ret = bt_pacs_cap_register(caps_dirs[i], &caps[i]);
		if (ret) {
			LOG_ERR("Capability register failed");
			return ret;
		}
	}

	if (channel == AUDIO_CH_L) {
		csip_param.rank = CSIP_HL_RANK;

		ret = bt_pacs_set_location(BT_AUDIO_DIR_SINK, BT_AUDIO_LOCATION_FRONT_LEFT);
		if (ret) {
			LOG_ERR("Location set failed");
			return ret;
		}

	} else if (channel == AUDIO_CH_R) {
		csip_param.rank = CSIP_HR_RANK;

		ret = bt_pacs_set_location(BT_AUDIO_DIR_SINK, BT_AUDIO_LOCATION_FRONT_RIGHT);
		if (ret) {
			LOG_ERR("Location set failed");
			return ret;
		}

	} else {
		LOG_ERR("Channel not supported");
		return -ECANCELED;
	}

	if (IS_ENABLED(CONFIG_STREAM_BIDIRECTIONAL)) {
		ret = bt_pacs_set_supported_contexts(BT_AUDIO_DIR_SINK,
						     BT_AUDIO_CONTEXT_TYPE_MEDIA |
							     BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL |
							     BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);

		if (ret) {
			LOG_ERR("Supported context set failed. Err: %d", ret);
			return ret;
		}

		ret = bt_pacs_set_available_contexts(BT_AUDIO_DIR_SINK,
						     BT_AUDIO_CONTEXT_TYPE_MEDIA |
							     BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL |
							     BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
		if (ret) {
			LOG_ERR("Available context set failed. Err: %d", ret);
			return ret;
		}

		ret = bt_pacs_set_supported_contexts(BT_AUDIO_DIR_SOURCE,
						     BT_AUDIO_CONTEXT_TYPE_MEDIA |
							     BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL |
							     BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);

		if (ret) {
			LOG_ERR("Supported context set failed. Err: %d", ret);
			return ret;
		}

		ret = bt_pacs_set_available_contexts(BT_AUDIO_DIR_SOURCE,
						     BT_AUDIO_CONTEXT_TYPE_MEDIA |
							     BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL |
							     BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
		if (ret) {
			LOG_ERR("Available context set failed. Err: %d", ret);
			return ret;
		}

		if (channel == AUDIO_CH_L) {
			ret = bt_pacs_set_location(BT_AUDIO_DIR_SOURCE,
						   BT_AUDIO_LOCATION_FRONT_LEFT);
			if (ret) {
				LOG_ERR("Location set failed");
				return ret;
			}
		} else if (channel == AUDIO_CH_R) {
			ret = bt_pacs_set_location(BT_AUDIO_DIR_SOURCE,
						   BT_AUDIO_LOCATION_FRONT_RIGHT);
			if (ret) {
				LOG_ERR("Location set failed");
				return ret;
			}
		} else {
			LOG_ERR("Channel not supported");
			return -ECANCELED;
		}
	} else {
		ret = bt_pacs_set_supported_contexts(BT_AUDIO_DIR_SINK,
						     BT_AUDIO_CONTEXT_TYPE_MEDIA |
							     BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);

		if (ret) {
			LOG_ERR("Supported context set failed. Err: %d ", ret);
			return ret;
		}

		ret = bt_pacs_set_available_contexts(BT_AUDIO_DIR_SINK,
						     BT_AUDIO_CONTEXT_TYPE_MEDIA |
							     BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);

		if (ret) {
			LOG_ERR("Available context set failed. Err: %d", ret);
			return ret;
		}
	}

	for (int i = 0; i < ARRAY_SIZE(audio_streams); i++) {
		bt_bap_stream_cb_register(&audio_streams[i], &stream_ops);
	}

	if (IS_ENABLED(CONFIG_BT_CSIP_SET_MEMBER)) {
		ret = bt_cap_acceptor_register(&csip_param, &csip);
		if (ret) {
			LOG_ERR("Failed to register CAP acceptor");
			return ret;
		}

		ret = bt_csip_set_member_generate_rsi(csip, csip_rsi_adv_data);
		if (ret) {
			LOG_ERR("Failed to generate RSI (ret %d)", ret);
			return ret;
		}
	}

	initialized = true;

	return 0;
}
