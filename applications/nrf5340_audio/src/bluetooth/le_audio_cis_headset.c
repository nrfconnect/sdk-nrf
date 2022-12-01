/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "le_audio.h"

#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/pacs.h>
#include <zephyr/bluetooth/audio/csis.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <../subsys/bluetooth/audio/endpoint.h>

#include "macros_common.h"
#include "ctrl_events.h"
#include "ble_audio_services.h"
#include "audio_datapath.h"
#include "channel_assignment.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cis_headset, CONFIG_BLE_LOG_LEVEL);

#define CHANNEL_COUNT_1 BIT(0)
#define BLE_ISO_LATENCY_MS 10
#define BLE_ISO_RETRANSMITS 2
#define BT_LE_ADV_FAST_CONN                                                                        \
	BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE, BT_GAP_ADV_FAST_INT_MIN_1,                      \
			BT_GAP_ADV_FAST_INT_MAX_1, NULL)

#if CONFIG_STREAM_BIDIRECTIONAL
#define HCI_ISO_BUF_ALLOC_PER_CHAN 2
/* For being able to dynamically define iso_tx_pools */
#define NET_BUF_POOL_ITERATE(i, _)                                                                 \
	NET_BUF_POOL_FIXED_DEFINE(iso_tx_pool_##i, HCI_ISO_BUF_ALLOC_PER_CHAN,                     \
				  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU), 8, NULL);
#define NET_BUF_POOL_PTR_ITERATE(i, ...) IDENTITY(&iso_tx_pool_##i)
LISTIFY(CONFIG_BT_ASCS_ASE_SRC_COUNT, NET_BUF_POOL_ITERATE, (;))

/* clang-format off */
static struct net_buf_pool *iso_tx_pools[] = { LISTIFY(CONFIG_BT_ASCS_ASE_SRC_COUNT,
						       NET_BUF_POOL_PTR_ITERATE, (,)) };
/* clang-format on */
#endif /* CONFIG_STREAM_BIDIRECTIONAL */

enum csis_set_rank { CSIS_HL_RANK = 1, CSIS_HR_RANK = 2, CSIS_SET_SIZE = 2 };

static struct bt_csis *csis;
static struct bt_le_ext_adv *adv_ext;
/* Advertising data for peer connection */
static uint8_t csis_rsi[BT_CSIS_RSI_SIZE];

static const struct bt_data ad_peer[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_ASCS_VAL)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_PACS_VAL)),
	BT_CSIS_DATA_RSI(csis_rsi)
};

/* Callback for locking state change from server side */
static void csis_lock_changed_cb(struct bt_conn *conn, struct bt_csis *csis, bool locked)
{
	LOG_DBG("Client %p %s the lock", (void *)conn, locked ? "locked" : "released");
}

/* Callback for SIRK read request from peer side */
static uint8_t sirk_read_req_cb(struct bt_conn *conn, struct bt_csis *csis)
{
	/* Accept the request to read the SIRK, but return encrypted SIRK instead of plaintext */
	return BT_CSIS_READ_SIRK_REQ_RSP_ACCEPT_ENC;
}

static struct bt_csis_cb csis_callbacks = {
	.lock_changed = csis_lock_changed_cb,
	.sirk_read_req = sirk_read_req_cb,
};
struct bt_csis_register_param csis_param = {
	.set_size = CSIS_SET_SIZE,
	.lockable = true,
#if !CONFIG_BT_CSIS_TEST_SAMPLE_DATA
	/* CSIS SIRK for demo is used, must be changed before production */
	.set_sirk = { 'N', 'R', 'F', '5', '3', '4', '0', '_', 'T', 'W', 'S', '_', 'D', 'E', 'M',
		      'O' },
#else
#warning "CSIS test sample data is used, must be changed before production"
#endif
	.cb = &csis_callbacks,
};

static le_audio_receive_cb receive_cb;

static struct bt_codec lc3_codec = BT_CODEC_LC3(
	BT_CODEC_LC3_FREQ_48KHZ, (BT_CODEC_LC3_DURATION_10 | BT_CODEC_LC3_DURATION_PREFER_10),
	CHANNEL_COUNT_1, LE_AUDIO_SDU_SIZE_OCTETS(CONFIG_LC3_BITRATE_MIN),
	LE_AUDIO_SDU_SIZE_OCTETS(CONFIG_LC3_BITRATE_MAX), 1u, BT_AUDIO_CONTEXT_TYPE_MEDIA);

static enum bt_audio_dir caps_dirs[] = {
	BT_AUDIO_DIR_SINK,
#if CONFIG_STREAM_BIDIRECTIONAL
	BT_AUDIO_DIR_SOURCE,
#endif
};

static const struct bt_codec_qos_pref qos_pref =
	BT_CODEC_QOS_PREF(true, BT_GAP_LE_PHY_2M, BLE_ISO_RETRANSMITS, BLE_ISO_LATENCY_MS,
			  MIN_PRES_DLY_US, MAX_PRES_DLY_US, MIN_PRES_DLY_US, MAX_PRES_DLY_US);
/* clang-format off */
static struct bt_pacs_cap caps[] = {
				{
				     .codec = &lc3_codec,
				},
#if CONFIG_STREAM_BIDIRECTIONAL
				{
				     .codec = &lc3_codec,
				}
#endif /* CONFIG_STREAM_BIDIRECTIONAL */
};
/* clang-format on */

static struct k_work adv_work;
static struct bt_conn *default_conn;
static struct bt_audio_stream
	audio_streams[CONFIG_BT_ASCS_ASE_SNK_COUNT + CONFIG_BT_ASCS_ASE_SRC_COUNT];

#if CONFIG_STREAM_BIDIRECTIONAL
static struct bt_audio_source {
	struct bt_audio_stream *stream;
	uint32_t seq_num;
} sources[CONFIG_BT_ASCS_ASE_SRC_COUNT];
#endif /* CONFIG_STREAM_BIDIRECTIONAL */

/* Left or right channel headset */
static enum audio_channel channel;

/* Bonded address queue */
K_MSGQ_DEFINE(bonds_queue, sizeof(bt_addr_le_t), CONFIG_BT_MAX_PAIRED, 4);

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

		uint32_t octets_per_sdu = bt_codec_cfg_get_octets_per_frame(codec);
		uint32_t bitrate =
			octets_per_sdu * 8 * (1000000 / bt_codec_cfg_get_frame_duration_us(codec));

		LOG_INF("\tOctets per frame: %d (%d kbps)", octets_per_sdu, bitrate);
		LOG_INF("\tFrames per SDU: %d", bt_codec_cfg_get_frame_blocks_per_sdu(codec, true));
	} else {
		LOG_WRN("Codec is not LC3, codec_id: 0x%2x", codec->id);
	}
}

static void advertising_process(struct k_work *work)
{
	int ret;
	struct bt_le_adv_param adv_param;

#if CONFIG_BT_BONDABLE
	bt_addr_le_t addr;

	if (!k_msgq_get(&bonds_queue, &addr, K_NO_WAIT)) {
		char addr_buf[BT_ADDR_LE_STR_LEN];

		adv_param = *BT_LE_ADV_CONN_DIR_LOW_DUTY(&addr);
		adv_param.id = BT_ID_DEFAULT;
		adv_param.options |= BT_LE_ADV_OPT_DIR_ADDR_RPA;

		/* Clear ADV data set before update to direct advertising */
		ret = bt_le_ext_adv_set_data(adv_ext, NULL, 0, NULL, 0);
		if (ret) {
			LOG_ERR("Failed to clear advertising data. Err: %d", ret);
			return;
		}

		ret = bt_le_ext_adv_update_param(adv_ext, &adv_param);
		if (ret) {
			LOG_ERR("Failed to update ext_adv to direct advertising. Err = %d", ret);
			return;
		}

		bt_addr_le_to_str(&addr, addr_buf, BT_ADDR_LE_STR_LEN);
		LOG_INF("Set direct advertising to %s", addr_buf);
	} else
#endif /* CONFIG_BT_BONDABLE */
	{
		ret = bt_csis_generate_rsi(csis, csis_rsi);
		if (ret) {
			LOG_ERR("Failed to generate RSI (ret %d)", ret);
			return;
		}

		ret = bt_le_ext_adv_set_data(adv_ext, ad_peer, ARRAY_SIZE(ad_peer), NULL, 0);
		if (ret) {
			LOG_ERR("Failed to set advertising data. Err: %d", ret);
			return;
		}
	}

	ret = bt_le_ext_adv_start(adv_ext, BT_LE_EXT_ADV_START_DEFAULT);
	if (ret) {
		LOG_ERR("Failed to start advertising set. Err: %d", ret);
		return;
	}

	LOG_INF("Advertising successfully started");
}

#if CONFIG_BT_BONDABLE
static void bond_find(const struct bt_bond_info *info, void *user_data)
{
	int ret;

	/* Filter already connected peers. */
	if (default_conn) {
		const bt_addr_le_t *dst = bt_conn_get_dst(default_conn);

		if (!bt_addr_le_cmp(&info->addr, dst)) {
			LOG_DBG("Already connected");
			return;
		}
	}

	ret = k_msgq_put(&bonds_queue, (void *)&info->addr, K_NO_WAIT);
	if (ret) {
		LOG_WRN("No space in the queue for the bond");
	}
}
#endif /* CONFIG_BT_BONDABLE */

static void advertising_start(void)
{
#if CONFIG_BT_BONDABLE
	k_msgq_purge(&bonds_queue);
	bt_foreach_bond(BT_ID_DEFAULT, bond_find, NULL);
#endif /* CONFIG_BT_BONDABLE */

	k_work_submit(&adv_work);
}

static int lc3_config_cb(struct bt_conn *conn, const struct bt_audio_ep *ep, enum bt_audio_dir dir,
			 const struct bt_codec *codec, struct bt_audio_stream **stream,
			 struct bt_codec_qos_pref *const pref)
{
	int ret;

#if CONFIG_STREAM_BIDIRECTIONAL
	static size_t configured_source_stream_count;
#endif /* CONFIG_STREAM_BIDIRECTIONAL */

	for (int i = 0; i < ARRAY_SIZE(audio_streams); i++) {
		struct bt_audio_stream *audio_stream = &audio_streams[i];

		if (!audio_stream->conn) {
			LOG_DBG("ASE Codec Config stream %p", (void *)audio_stream);

			uint32_t octets_per_sdu = bt_codec_cfg_get_octets_per_frame(codec);

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
				print_codec(codec);
				ret = ctrl_events_le_audio_event_send(LE_AUDIO_EVT_CONFIG_RECEIVED);
				ERR_CHK(ret);
			}
#if CONFIG_STREAM_BIDIRECTIONAL
			else if (dir == BT_AUDIO_DIR_SOURCE) {
				LOG_DBG("BT_AUDIO_DIR_SOURCE");
				print_codec(codec);
				sources[configured_source_stream_count++].stream = audio_stream;
			}
#endif /* CONFIG_STREAM_BIDIRECTIONAL */
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

static int lc3_reconfig_cb(struct bt_audio_stream *stream, enum bt_audio_dir dir,
			   const struct bt_codec *codec, struct bt_codec_qos_pref *const pref)
{
	LOG_DBG("ASE Codec Reconfig: stream %p", (void *)stream);

	return 0;
}

static int lc3_qos_cb(struct bt_audio_stream *stream, const struct bt_codec_qos *qos)
{
	int ret;

	LOG_DBG("QoS: stream %p qos %p", (void *)stream, (void *)qos);
	LOG_INF("Presentation delay %d us is set by initiator", qos->pd);
	ret = audio_datapath_pres_delay_us_set(qos->pd);

	return ret;
}

static int lc3_enable_cb(struct bt_audio_stream *stream, const struct bt_codec_data *meta,
			 size_t meta_count)
{
	int ret;

	LOG_DBG("Enable: stream %p meta_count %d", (void *)stream, meta_count);

	ret = ctrl_events_le_audio_event_send(LE_AUDIO_EVT_STREAMING);
	ERR_CHK(ret);

	return 0;
}

static int lc3_start_cb(struct bt_audio_stream *stream)
{
	LOG_DBG("Stream started %p", (void *)stream);
	return 0;
}

static int lc3_metadata_cb(struct bt_audio_stream *stream, const struct bt_codec_data *meta,
			   size_t meta_count)
{
	LOG_DBG("Metadata: stream %p meta_count %d", (void *)stream, meta_count);
	return 0;
}

static int lc3_disable_cb(struct bt_audio_stream *stream)
{
	LOG_DBG("Disable: stream %p", (void *)stream);
	return 0;
}

static int lc3_stop_cb(struct bt_audio_stream *stream)
{
	int ret;

	LOG_DBG("Stop: stream %p", (void *)stream);

	ret = ctrl_events_le_audio_event_send(LE_AUDIO_EVT_NOT_STREAMING);
	ERR_CHK(ret);

	return 0;
}

static int lc3_release_cb(struct bt_audio_stream *stream)
{
	LOG_DBG("Release: stream %p", (void *)stream);
	return 0;
}

static const struct bt_audio_unicast_server_cb unicast_server_cb = {
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

static void stream_stop_cb(struct bt_audio_stream *stream)
{
	int ret;

	LOG_INF("Stream stopped");

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

	advertising_start();
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
#if (CONFIG_BT_MCC)
		ret = ble_mcs_discover(default_conn);
		if (ret) {
			LOG_ERR("Failed to start discovery of MCS: %d", ret);
		}
#endif /* CONFIG_BT_MCC */
	}
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected_cb,
	.disconnected = disconnected_cb,
	.security_changed = security_changed_cb,
};

static struct bt_audio_stream_ops stream_ops = { .recv = stream_recv_cb,
						 .stopped = stream_stop_cb };

static int initialize(le_audio_receive_cb recv_cb)
{
	int ret;
	static bool initialized;

	if (!initialized) {
		bt_audio_unicast_server_register_cb(&unicast_server_cb);
		bt_conn_cb_register(&conn_callbacks);
#if (CONFIG_BT_VCS)
		ret = ble_vcs_server_init();
		if (ret) {
			LOG_ERR("VCS server init failed");
			return ret;
		}
#endif /* (CONFIG_BT_VCS) */

#if (CONFIG_BT_MCC)
		ret = ble_mcs_client_init();
		if (ret) {
			LOG_ERR("MCS client init failed");
			return ret;
		}
#endif /* CONFIG_BT_MCC */

		receive_cb = recv_cb;
		channel_assignment_get(&channel);

		for (int i = 0; i < ARRAY_SIZE(caps); i++) {
			ret = bt_pacs_cap_register(caps_dirs[i], &caps[i]);
			if (ret) {
				LOG_ERR("Capability register failed");
				return ret;
			}
		}
		if (channel == AUDIO_CH_L) {
			csis_param.rank = CSIS_HL_RANK;
			ret = bt_pacs_set_location(BT_AUDIO_DIR_SINK, BT_AUDIO_LOCATION_FRONT_LEFT);
			if (ret) {
				LOG_ERR("Location set failed");
				return ret;
			}

		} else {
			csis_param.rank = CSIS_HR_RANK;
			ret = bt_pacs_set_location(BT_AUDIO_DIR_SINK,
						   BT_AUDIO_LOCATION_FRONT_RIGHT);
			if (ret) {
				LOG_ERR("Location set failed");
				return ret;
			}
		}
#if CONFIG_STREAM_BIDIRECTIONAL
		ret = bt_pacs_set_available_contexts(BT_AUDIO_DIR_SINK,
						     BT_AUDIO_CONTEXT_TYPE_MEDIA |
							     BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL |
							     BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
		if (ret) {
			LOG_ERR("Available context set failed");
			return ret;
		}

		ret = bt_pacs_set_available_contexts(BT_AUDIO_DIR_SOURCE,
						     BT_AUDIO_CONTEXT_TYPE_MEDIA |
							     BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL |
							     BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
		if (ret) {
			LOG_ERR("Available context set failed");
			return ret;
		}

		ret = bt_pacs_set_location(BT_AUDIO_DIR_SOURCE, BT_AUDIO_LOCATION_FRONT_LEFT);
		if (ret) {
			LOG_ERR("Location set failed");
			return ret;
		}
#else
		ret = bt_pacs_set_available_contexts(BT_AUDIO_DIR_SINK,
						     BT_AUDIO_CONTEXT_TYPE_MEDIA |
							     BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
		if (ret) {
			LOG_ERR("Available context set failed");
			return ret;
		}
#endif /* CONFIG_STREAM_BIDIRECTIONAL */
		for (int i = 0; i < ARRAY_SIZE(audio_streams); i++) {
			bt_audio_stream_cb_register(&audio_streams[i], &stream_ops);
		}

		ret = bt_cap_acceptor_register(&csis_param, &csis);
		if (ret) {
			LOG_ERR("Failed to register CAP acceptor");
			return ret;
		}

		ret = bt_le_ext_adv_create(BT_LE_EXT_ADV_CONN_NAME, NULL, &adv_ext);
		if (ret) {
			LOG_ERR("Failed to create advertising set");
			return ret;
		}

		k_work_init(&adv_work, advertising_process);
		initialized = true;
	}

	return 0;
}

int le_audio_user_defined_button_press(void)
{
	return 0;
}

int le_audio_config_get(uint32_t *bitrate, uint32_t *sampling_rate)
{
	/* Get the configuration for the sink stream */
	int frames_per_sec = 1000000 / bt_codec_cfg_get_frame_duration_us(audio_streams[0].codec);
	int bits_per_frame = bt_codec_cfg_get_octets_per_frame(audio_streams[0].codec) * 8;

	*sampling_rate = bt_codec_cfg_get_freq(audio_streams[0].codec);
	*bitrate = frames_per_sec * bits_per_frame;
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
	return ble_mcs_play_pause(default_conn);
}

int le_audio_send(uint8_t const *const data, size_t size)
{
#if CONFIG_STREAM_BIDIRECTIONAL
	int ret;
	struct net_buf *buf;

	if (sources[0].stream->ep->status.state != BT_AUDIO_EP_STATE_STREAMING) {
		LOG_DBG("Return channel not connected");
		return 0;
	}

	buf = net_buf_alloc(iso_tx_pools[0], K_NO_WAIT);
	if (buf == NULL) {
		LOG_WRN("Out of TX buffers");
		return -ENOMEM;
	}

	net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);
	net_buf_add_mem(buf, data, size);

	ret = bt_audio_stream_send(sources[0].stream, buf, sources[0].seq_num++,
				   BT_ISO_TIMESTAMP_NONE);
	if (ret < 0) {
		LOG_WRN("Failed to send audio data: %d", ret);
		net_buf_unref(buf);
	}
#endif /* CONFIG_STREAM_BIDIRECTIONAL */

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

	advertising_start();

	return 0;
}

int le_audio_disable(void)
{
	return 0;
}
