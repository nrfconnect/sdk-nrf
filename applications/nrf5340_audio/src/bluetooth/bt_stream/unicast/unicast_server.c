/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "unicast_server.h"

#include <zephyr/zbus/zbus.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/pacs.h>
#include <zephyr/bluetooth/audio/csip.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/lc3.h>

#include "macros_common.h"
#include "zbus_common.h"
#include "bt_mgmt.h"
#include "bt_le_audio_tx.h"
#include "le_audio.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(unicast_server, CONFIG_UNICAST_SERVER_LOG_LEVEL);

ZBUS_CHAN_DEFINE(le_audio_chan, struct le_audio_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
		 ZBUS_MSG_INIT(0));

#define BLE_ISO_LATENCY_MS 10
#define CSIP_SET_SIZE	   2
enum csip_set_rank {
	CSIP_HL_RANK = 1,
	CSIP_HR_RANK = 2
};

static le_audio_receive_cb receive_cb;
static struct bt_csip_set_member_svc_inst *csip;

/* Advertising data for peer connection */
static uint8_t csip_rsi_adv_data[BT_CSIP_RSI_SIZE];

static uint8_t flags_adv_data;

static uint8_t gap_appear_adv_data[BT_UUID_SIZE_16];

static const uint8_t cap_adv_data[] = {
	BT_UUID_16_ENCODE(BT_UUID_CAS_VAL),
	BT_AUDIO_UNICAST_ANNOUNCEMENT_TARGETED,
};

#if defined(CONFIG_BT_AUDIO_RX)
#define AVAILABLE_SINK_CONTEXT (BT_AUDIO_CONTEXT_TYPE_ANY)
#else
#define AVAILABLE_SINK_CONTEXT BT_AUDIO_CONTEXT_TYPE_PROHIBITED
#endif /* CONFIG_BT_AUDIO_RX */

static struct bt_cap_stream *cap_tx_streams[CONFIG_BT_ASCS_MAX_ASE_SRC_COUNT];

#if defined(CONFIG_BT_AUDIO_TX)
#define AVAILABLE_SOURCE_CONTEXT (BT_AUDIO_CONTEXT_TYPE_ANY)
#else
#define AVAILABLE_SOURCE_CONTEXT BT_AUDIO_CONTEXT_TYPE_PROHIBITED
#endif /* CONFIG_BT_AUDIO_TX */

static struct bt_bap_unicast_server_register_param unicast_server_params = {
	CONFIG_BT_ASCS_MAX_ASE_SNK_COUNT, CONFIG_BT_ASCS_MAX_ASE_SRC_COUNT};

static uint8_t unicast_server_adv_data[] = {
	BT_UUID_16_ENCODE(BT_UUID_ASCS_VAL),
	BT_AUDIO_UNICAST_ANNOUNCEMENT_TARGETED,
	BT_BYTES_LIST_LE16(AVAILABLE_SINK_CONTEXT),
	BT_BYTES_LIST_LE16(AVAILABLE_SOURCE_CONTEXT),
	0x00, /* Metadata length */
};

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
	.cb = &csip_callbacks,
};

#if defined(CONFIG_BT_AUDIO_RX)
static struct bt_audio_codec_cap lc3_codec_sink = BT_AUDIO_CODEC_CAP_LC3(
	BT_AUDIO_CODEC_CAPABILIY_FREQ,
	(BT_AUDIO_CODEC_CAP_DURATION_10 | BT_AUDIO_CODEC_CAP_DURATION_PREFER_10),
	BT_AUDIO_CODEC_CAP_CHAN_COUNT_SUPPORT(1), LE_AUDIO_SDU_SIZE_OCTETS(CONFIG_LC3_BITRATE_MIN),
	LE_AUDIO_SDU_SIZE_OCTETS(CONFIG_LC3_BITRATE_MAX), 1u, AVAILABLE_SINK_CONTEXT);
#endif /* (CONFIG_BT_AUDIO_RX) */

#if defined(CONFIG_BT_AUDIO_TX)
static struct bt_audio_codec_cap lc3_codec_source = BT_AUDIO_CODEC_CAP_LC3(
	BT_AUDIO_CODEC_CAPABILIY_FREQ,
	(BT_AUDIO_CODEC_CAP_DURATION_10 | BT_AUDIO_CODEC_CAP_DURATION_PREFER_10),
	BT_AUDIO_CODEC_CAP_CHAN_COUNT_SUPPORT(1), LE_AUDIO_SDU_SIZE_OCTETS(CONFIG_LC3_BITRATE_MIN),
	LE_AUDIO_SDU_SIZE_OCTETS(CONFIG_LC3_BITRATE_MAX), 1u, AVAILABLE_SOURCE_CONTEXT);
#endif /* (CONFIG_BT_AUDIO_TX) */

static enum bt_audio_dir caps_dirs[] = {
#if defined(CONFIG_BT_AUDIO_RX)
	BT_AUDIO_DIR_SINK,
#endif /* CONFIG_BT_AUDIO_RX */
#if defined(CONFIG_BT_AUDIO_TX)
	BT_AUDIO_DIR_SOURCE,
#endif /* (CONFIG_BT_AUDIO_TX) */
};

static const struct bt_audio_codec_qos_pref qos_pref = BT_AUDIO_CODEC_QOS_PREF(
	true, BT_GAP_LE_PHY_2M, CONFIG_BT_AUDIO_RETRANSMITS, BLE_ISO_LATENCY_MS,
	CONFIG_AUDIO_MIN_PRES_DLY_US, CONFIG_AUDIO_MAX_PRES_DLY_US,
	CONFIG_BT_AUDIO_PREFERRED_MIN_PRES_DLY_US, CONFIG_BT_AUDIO_PREFERRED_MAX_PRES_DLY_US);

/* clang-format off */
static struct bt_pacs_cap caps[] = {
#if (CONFIG_BT_AUDIO_RX)
				{
					 .codec_cap = &lc3_codec_sink,
				},
#endif
#if (CONFIG_BT_AUDIO_TX)
				{
					 .codec_cap = &lc3_codec_source,
				}
#endif /* (CONFIG_BT_AUDIO_TX) */
};
/* clang-format on */

static struct bt_cap_stream
	cap_audio_streams[CONFIG_BT_ASCS_MAX_ASE_SNK_COUNT + CONFIG_BT_ASCS_MAX_ASE_SRC_COUNT];

#if (CONFIG_BT_AUDIO_TX)
BUILD_ASSERT(CONFIG_BT_ASCS_MAX_ASE_SRC_COUNT <= 1,
	     "CIS headset only supports one source stream for now");
#endif /* (CONFIG_BT_AUDIO_TX) */

static int lc3_config_cb(struct bt_conn *conn, const struct bt_bap_ep *ep, enum bt_audio_dir dir,
			 const struct bt_audio_codec_cfg *codec, struct bt_bap_stream **stream,
			 struct bt_audio_codec_qos_pref *const pref, struct bt_bap_ascs_rsp *rsp)
{
	int ret;
	LOG_DBG("LC3 config callback");

	for (int i = 0; i < ARRAY_SIZE(cap_audio_streams); i++) {
		struct bt_cap_stream *cap_audio_stream = &cap_audio_streams[i];

		if (!cap_audio_stream->bap_stream.conn) {
			LOG_DBG("ASE Codec Config stream %p", (void *)cap_audio_stream);

			ret = le_audio_bitrate_check(codec);
			if (!ret) {
				LOG_WRN("Bitrate check failed");
				return -EINVAL;
			}

			ret = le_audio_freq_check(codec);
			if (!ret) {
				LOG_WRN("Sample rate not supported");
				return -EINVAL;
			}

			if (dir == BT_AUDIO_DIR_SINK) {
				LOG_DBG("BT_AUDIO_DIR_SINK");
				le_audio_print_codec(codec, dir);
				le_audio_event_publish(LE_AUDIO_EVT_CONFIG_RECEIVED, conn, dir);
			}
#if (CONFIG_BT_AUDIO_TX)
			else if (dir == BT_AUDIO_DIR_SOURCE) {
				LOG_DBG("BT_AUDIO_DIR_SOURCE");
				le_audio_print_codec(codec, dir);
				le_audio_event_publish(LE_AUDIO_EVT_CONFIG_RECEIVED, conn, dir);

				/* CIS headset only supports one source stream for now */
				cap_tx_streams[0] = cap_audio_stream;
			}
#endif /* (CONFIG_BT_AUDIO_TX) */
			else {
				LOG_ERR("UNKNOWN DIR");
				return -EINVAL;
			}

			*stream = &cap_audio_stream->bap_stream;
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
	enum bt_audio_dir dir;

	dir = le_audio_stream_dir_get(stream);
	if (dir <= 0) {
		LOG_ERR("Failed to get dir of stream %p", stream);
		return -EIO;
	}

	le_audio_event_publish(LE_AUDIO_EVT_PRES_DELAY_SET, stream->conn, dir);

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
	enum bt_audio_dir dir;

	dir = le_audio_stream_dir_get(stream);
	if (dir <= 0) {
		LOG_ERR("Failed to get dir of stream %p", stream);
		return -EIO;
	}

	LOG_DBG("Disable: stream %p", (void *)stream);

	le_audio_event_publish(LE_AUDIO_EVT_NOT_STREAMING, stream->conn, dir);

	return 0;
}

static int lc3_stop_cb(struct bt_bap_stream *stream, struct bt_bap_ascs_rsp *rsp)
{
	enum bt_audio_dir dir;

	dir = le_audio_stream_dir_get(stream);
	if (dir <= 0) {
		LOG_ERR("Failed to get dir of stream %p", stream);
		return -EIO;
	}

	LOG_DBG("Stop: stream %p", (void *)stream);

	le_audio_event_publish(LE_AUDIO_EVT_NOT_STREAMING, stream->conn, dir);

	return 0;
}

static int lc3_release_cb(struct bt_bap_stream *stream, struct bt_bap_ascs_rsp *rsp)
{
	enum bt_audio_dir dir;

	dir = le_audio_stream_dir_get(stream);
	if (dir <= 0) {
		LOG_ERR("Failed to get dir of stream %p", stream);
		return -EIO;
	}

	LOG_DBG("Release: stream %p", (void *)stream);

	le_audio_event_publish(LE_AUDIO_EVT_NOT_STREAMING, stream->conn, dir);

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

	receive_cb(buf->data, buf->len, bad_frame, info->ts, 0,
		   bt_audio_codec_cfg_get_octets_per_frame(stream->codec_cfg));
}
#endif /* (CONFIG_BT_AUDIO_RX) */

#if (CONFIG_BT_AUDIO_TX)
static void stream_sent_cb(struct bt_bap_stream *stream)
{
	/* Unicast server/CIS headset only supports one source stream for now */
	struct stream_index idx = {
		.lvl1 = 0,
		.lvl2 = 0,
		.lvl3 = 0,
	};
	ERR_CHK(bt_le_audio_tx_stream_sent(idx));
}
#endif /* (CONFIG_BT_AUDIO_TX) */

static void stream_enabled_cb(struct bt_bap_stream *stream)
{
	int ret;
	enum bt_audio_dir dir;

	dir = le_audio_stream_dir_get(stream);
	if (dir <= 0) {
		LOG_ERR("Failed to get dir of stream %p", stream);
		return;
	}

	LOG_DBG("Stream %p enabled", stream);

	if (dir == BT_AUDIO_DIR_SINK) {
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
	enum bt_audio_dir dir;

	dir = le_audio_stream_dir_get(stream);
	if (dir <= 0) {
		LOG_ERR("Failed to get dir of stream %p", stream);
		return;
	}

	LOG_INF("Stream %p started", stream);

	if (dir == BT_AUDIO_DIR_SOURCE && IS_ENABLED(CONFIG_BT_AUDIO_TX)) {
		struct stream_index idx = {
			.lvl1 = 0,
			.lvl2 = 0,
			.lvl3 = 0,
		};
		ERR_CHK(bt_le_audio_tx_stream_started(idx));
	}

	le_audio_event_publish(LE_AUDIO_EVT_STREAMING, stream->conn, dir);
}

static void stream_stopped_cb(struct bt_bap_stream *stream, uint8_t reason)
{
	enum bt_audio_dir dir;

	dir = le_audio_stream_dir_get(stream);
	if (dir <= 0) {
		LOG_ERR("Failed to get dir of stream %p", stream);
		return;
	}

	LOG_DBG("Stream %p stopped. Reason: %d", stream, reason);

	le_audio_event_publish(LE_AUDIO_EVT_NOT_STREAMING, stream->conn, dir);
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

int unicast_server_config_get(struct bt_conn *conn, enum bt_audio_dir dir, uint32_t *bitrate,
			      uint32_t *sampling_rate_hz, uint32_t *pres_delay_us)
{
	int ret;

	if (bitrate == NULL && sampling_rate_hz == NULL && pres_delay_us == NULL) {
		LOG_ERR("No valid pointers received");
		return -ENXIO;
	}

	if (dir == BT_AUDIO_DIR_SINK) {
		/* If multiple sink streams exists, they should have the same configurations,
		 * hence we only check the first one.
		 */
		if (cap_audio_streams[0].bap_stream.codec_cfg == NULL) {
			LOG_ERR("No codec found for the stream");

			return -ENXIO;
		}

		if (sampling_rate_hz != NULL) {
			ret = le_audio_freq_hz_get(cap_audio_streams[0].bap_stream.codec_cfg,
						   sampling_rate_hz);
			if (ret) {
				LOG_ERR("Invalid sampling frequency: %d", ret);
				return -ENXIO;
			}
		}

		if (bitrate != NULL) {
			ret = le_audio_bitrate_get(cap_audio_streams[0].bap_stream.codec_cfg,
						   bitrate);
			if (ret) {
				LOG_ERR("Unable to calculate bitrate: %d", ret);
				return -ENXIO;
			}
		}

		if (pres_delay_us != NULL) {
			if (cap_audio_streams[0].bap_stream.qos == NULL) {
				LOG_ERR("No QoS found for the stream");
				return -ENXIO;
			}

			*pres_delay_us = cap_audio_streams[0].bap_stream.qos->pd;
		}
	} else if (dir == BT_AUDIO_DIR_SOURCE && IS_ENABLED(CONFIG_BT_AUDIO_TX)) {
		/* If multiple source streams exists, they should have the same configurations,
		 * hence we only check the first one.
		 */
		if (cap_tx_streams[0]->bap_stream.codec_cfg == NULL) {
			LOG_ERR("No codec found for the stream");
			return -ENXIO;
		}

		if (sampling_rate_hz != NULL) {
			ret = le_audio_freq_hz_get(cap_tx_streams[0]->bap_stream.codec_cfg,
						   sampling_rate_hz);
			if (ret) {
				LOG_ERR("Invalid sampling frequency: %d", ret);
				return -ENXIO;
			}
		}

		if (bitrate != NULL) {
			ret = le_audio_bitrate_get(cap_tx_streams[0]->bap_stream.codec_cfg,
						   bitrate);
			if (ret) {
				LOG_ERR("Unable to calculate bitrate: %d", ret);
				return -ENXIO;
			}
		}

		if (pres_delay_us != NULL) {
			if (cap_tx_streams[0]->bap_stream.qos == NULL) {
				LOG_ERR("No QoS found for the stream");
				return -ENXIO;
			}

			*pres_delay_us = cap_tx_streams[0]->bap_stream.qos->pd;
			LOG_ERR("pres_delay_us: %d", *pres_delay_us);
		}
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
	uint32_t adv_buf_cnt = 0;

	ret = bt_mgmt_adv_buffer_put(adv_buf, &adv_buf_cnt, adv_buf_vacant,
				     sizeof(unicast_server_adv_data), BT_DATA_SVC_DATA16,
				     unicast_server_adv_data);
	if (ret) {
		return ret;
	}

	if (IS_ENABLED(CONFIG_BT_CSIP_SET_MEMBER)) {
		ret = bt_mgmt_adv_buffer_put(adv_buf, &adv_buf_cnt, adv_buf_vacant,
					     sizeof(csip_rsi_adv_data), BT_DATA_CSIS_RSI,
					     (void *)csip_rsi_adv_data);
		if (ret) {
			return ret;
		}
	}

	sys_put_le16(CONFIG_BT_DEVICE_APPEARANCE, &gap_appear_adv_data[0]);

	ret = bt_mgmt_adv_buffer_put(adv_buf, &adv_buf_cnt, adv_buf_vacant,
				     sizeof(gap_appear_adv_data), BT_DATA_GAP_APPEARANCE,
				     (void *)gap_appear_adv_data);
	if (ret) {
		return ret;
	}

	flags_adv_data = BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR;

	ret = bt_mgmt_adv_buffer_put(adv_buf, &adv_buf_cnt, adv_buf_vacant, sizeof(uint8_t),
				     BT_DATA_FLAGS, (void *)&flags_adv_data);
	if (ret) {
		return ret;
	}

	ret = bt_mgmt_adv_buffer_put(adv_buf, &adv_buf_cnt, adv_buf_vacant,
				     ARRAY_SIZE(cap_adv_data), BT_DATA_SVC_DATA16,
				     (void *)cap_adv_data);
	if (ret) {
		return ret;
	}

	return adv_buf_cnt;
}

int unicast_server_send(struct le_audio_encoded_audio enc_audio)
{
#if (CONFIG_BT_AUDIO_TX)
	int ret;
	uint8_t num_active_streams = 0;

	struct le_audio_tx_info tx[CONFIG_BT_ASCS_MAX_ASE_SRC_COUNT];

	for (int i = 0; i < ARRAY_SIZE(cap_tx_streams); i++) {
		if (!le_audio_ep_state_check(cap_tx_streams[i]->bap_stream.ep,
					     BT_BAP_EP_STATE_STREAMING)) {
			continue;
		}

		/* Set cap stream pointer */
		tx[num_active_streams].cap_stream = cap_tx_streams[i];

		/* Set index */
		tx[num_active_streams].idx.lvl1 = 0;
		tx[num_active_streams].idx.lvl2 = 0;
		tx[num_active_streams].idx.lvl3 = i;

		/* Set channel location */
		tx[num_active_streams].audio_channel = AUDIO_MIC;

		num_active_streams++;
	}

	ret = bt_le_audio_tx_send(tx, num_active_streams, enc_audio);
	if (ret) {
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

int unicast_server_enable(le_audio_receive_cb recv_cb, enum bt_audio_location location)
{
	int ret;
	static bool initialized;

	__ASSERT(strlen(CONFIG_BT_SET_IDENTITY_RESOLVING_KEY) == BT_CSIP_SIRK_SIZE,
		 "SIRK incorrect size, must be 16 bytes");

	if (initialized) {
		LOG_WRN("Already initialized");
		return -EALREADY;
	}

	if (recv_cb == NULL && IS_ENABLED(CONFIG_BT_AUDIO_RX)) {
		LOG_ERR("Receive callback is NULL");
		return -EINVAL;
	}

	receive_cb = recv_cb;

	bt_bap_unicast_server_register(&unicast_server_params);
	bt_bap_unicast_server_register_cb(&unicast_server_cb);

	if (IS_ENABLED(CONFIG_BT_CSIP_SET_MEMBER_TEST_SAMPLE_DATA)) {
		LOG_WRN("CSIP test sample data is used, must be changed "
			"before production");
	} else {
		if (strcmp(CONFIG_BT_SET_IDENTITY_RESOLVING_KEY_DEFAULT,
			   CONFIG_BT_SET_IDENTITY_RESOLVING_KEY) == 0) {
			LOG_WRN("CSIP using the default SIRK, must be changed "
				"before production");
		}

		memcpy(csip_param.sirk, CONFIG_BT_SET_IDENTITY_RESOLVING_KEY, BT_CSIP_SIRK_SIZE);
	}

	for (int i = 0; i < ARRAY_SIZE(caps); i++) {
		ret = bt_pacs_cap_register(caps_dirs[i], &caps[i]);
		if (ret) {
			LOG_ERR("Capability register failed. Err: %d", ret);
			return ret;
		}
	}

	if (IS_ENABLED(CONFIG_BT_AUDIO_RX)) {
		if (location == BT_AUDIO_LOCATION_FRONT_LEFT) {
			csip_param.rank = CSIP_HL_RANK;
		} else if (location == BT_AUDIO_LOCATION_FRONT_RIGHT) {
			csip_param.rank = CSIP_HR_RANK;
		} else {
			LOG_ERR("Channel not supported");
			return -ECANCELED;
		}

		ret = bt_pacs_set_location(BT_AUDIO_DIR_SINK, location);
		if (ret) {
			LOG_ERR("Location set failed. Err: %d", ret);
			return ret;
		}
	}

	if (IS_ENABLED(CONFIG_BT_AUDIO_TX)) {
		bt_le_audio_tx_init();

		ret = bt_pacs_set_location(BT_AUDIO_DIR_SOURCE, location);
		if (ret) {
			LOG_ERR("Location set failed. Err: %d", ret);
			return ret;
		}
	}

	ret = bt_pacs_set_supported_contexts(BT_AUDIO_DIR_SINK, AVAILABLE_SINK_CONTEXT);

	if (ret) {
		LOG_ERR("Supported context set failed. Err: %d", ret);
		return ret;
	}

	ret = bt_pacs_set_available_contexts(BT_AUDIO_DIR_SINK, AVAILABLE_SINK_CONTEXT);
	if (ret) {
		LOG_ERR("Available context set failed. Err: %d", ret);
		return ret;
	}

	ret = bt_pacs_set_supported_contexts(BT_AUDIO_DIR_SOURCE, AVAILABLE_SOURCE_CONTEXT);

	if (ret) {
		LOG_ERR("Supported context set failed. Err: %d", ret);
		return ret;
	}

	ret = bt_pacs_set_available_contexts(BT_AUDIO_DIR_SOURCE, AVAILABLE_SOURCE_CONTEXT);
	if (ret) {
		LOG_ERR("Available context set failed. Err: %d", ret);
		return ret;
	}

	for (int i = 0; i < ARRAY_SIZE(cap_audio_streams); i++) {
		bt_cap_stream_ops_register(&cap_audio_streams[i], &stream_ops);
	}

	if (IS_ENABLED(CONFIG_BT_CSIP_SET_MEMBER)) {
		ret = bt_cap_acceptor_register(&csip_param, &csip);
		if (ret) {
			LOG_ERR("Failed to register CAP acceptor. Err: %d", ret);
			return ret;
		}

		ret = bt_csip_set_member_generate_rsi(csip, csip_rsi_adv_data);
		if (ret) {
			LOG_ERR("Failed to generate RSI. Err: %d", ret);
			return ret;
		}
	}

	initialized = true;

	return 0;
}
