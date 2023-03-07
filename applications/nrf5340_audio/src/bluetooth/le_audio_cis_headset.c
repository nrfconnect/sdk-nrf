/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "le_audio.h"

#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/pacs.h>
#include <zephyr/bluetooth/audio/csip.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <../subsys/bluetooth/audio/endpoint.h>

#include "macros_common.h"
#include "ctrl_events.h"
#include "ble_audio_services.h"
#include "ble_hci_vsc.h"
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

static atomic_t iso_tx_pool_alloc;
/* clang-format off */
static struct net_buf_pool *iso_tx_pools[] = { LISTIFY(CONFIG_BT_ASCS_ASE_SRC_COUNT,
						       NET_BUF_POOL_PTR_ITERATE, (,)) };
/* clang-format on */
#endif /* CONFIG_STREAM_BIDIRECTIONAL */

#define CSIP_SET_SIZE 2
enum csip_set_rank { CSIP_HL_RANK = 1, CSIP_HR_RANK = 2 };

static struct bt_csip_set_member_svc_inst *csip;
static struct bt_le_ext_adv *adv_ext;
/* Advertising data for peer connection */
static uint8_t csip_rsi[BT_CSIP_RSI_SIZE];

static const struct bt_data ad_peer[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_ASCS_VAL)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_PACS_VAL)),
	BT_CSIP_DATA_RSI(csip_rsi)
};

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
	.set_sirk = { 'N', 'R', 'F', '5', '3', '4', '0', '_', 'T', 'W', 'S', '_', 'D', 'E', 'M',
		      'O' },
#else
#warning "CSIP test sample data is used, must be changed before production"
#endif
	.cb = &csip_callbacks,
};

static le_audio_receive_cb receive_cb;

static struct bt_codec lc3_codec = BT_CODEC_LC3(
	BT_AUDIO_CODEC_CAPABILIY_FREQ, (BT_CODEC_LC3_DURATION_10 | BT_CODEC_LC3_DURATION_PREFER_10),
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
BUILD_ASSERT(CONFIG_BT_ASCS_ASE_SRC_COUNT <= 1,
	     "CIS headset only supports one source stream for now");
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

		LOG_INF("\tOctets per frame: %d (%d bps)", octets_per_sdu, bitrate);
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
		ret = bt_csip_set_member_generate_rsi(csip, csip_rsi);
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

				/* CIS headset only supports one source stream for now */
				sources[0].stream = audio_stream;
			}
#endif /* CONFIG_STREAM_BIDIRECTIONAL */
			else {
				LOG_ERR("UNKNOWN DIR");
				return -EINVAL;
			}

			*stream = audio_stream;
			*pref = qos_pref;
			if (IS_ENABLED(CONFIG_BT_MCC)) {
				ret = ble_mcs_discover(conn);
				if (ret) {
					LOG_ERR("Failed to start discovery of MCS: %d", ret);
				}
			}

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
	int ret;

	LOG_DBG("Disable: stream %p", (void *)stream);

	ret = ctrl_events_le_audio_event_send(LE_AUDIO_EVT_NOT_STREAMING);
	ERR_CHK(ret);

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
	int ret;

	LOG_DBG("Release: stream %p", (void *)stream);

	ret = ctrl_events_le_audio_event_send(LE_AUDIO_EVT_NOT_STREAMING);
	ERR_CHK(ret);

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

static void stream_sent_cb(struct bt_audio_stream *stream)
{
#if CONFIG_STREAM_BIDIRECTIONAL
	atomic_dec(&iso_tx_pool_alloc);
#endif /* CONFIG_STREAM_BIDIRECTIONAL */
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

	receive_cb(buf->data, buf->len, bad_frame, info->ts, channel);

	recv_cnt++;
	if ((recv_cnt % 1000U) == 0U) {
		LOG_DBG("Received %d total ISO packets", recv_cnt);
	}
}

static void stream_start_cb(struct bt_audio_stream *stream)
{
	LOG_INF("Stream started");
}

static void stream_stop_cb(struct bt_audio_stream *stream)
{
	int ret;

	LOG_INF("Stream stopped");
#if CONFIG_STREAM_BIDIRECTIONAL
	atomic_clear(&iso_tx_pool_alloc);
#endif /* CONFIG_STREAM_BIDIRECTIONAL */
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

#if (CONFIG_NRF_21540_ACTIVE)
	int ret;
	uint16_t conn_handle;

	ret = bt_hci_get_conn_handle(conn, &conn_handle);
	if (ret) {
		LOG_ERR("Unable to get conn handle");
	} else {
		ret = ble_hci_vsc_conn_tx_pwr_set(conn_handle, CONFIG_NRF_21540_MAIN_DBM);
		if (ret) {
			LOG_ERR("Failed to set TX power for conn");
		} else {
			LOG_INF("\tTX power set to %d dBm", CONFIG_NRF_21540_MAIN_DBM);
		}
	}
#endif /* (CONFIG_NRF_21540_ACTIVE) */

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
	}
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected_cb,
	.disconnected = disconnected_cb,
	.security_changed = security_changed_cb,
};

static struct bt_audio_stream_ops stream_ops = { .recv = stream_recv_cb,
						 .sent = stream_sent_cb,
						 .started = stream_start_cb,
						 .stopped = stream_stop_cb };

static int initialize(le_audio_receive_cb recv_cb)
{
	int ret;
	static bool initialized;

	if (!initialized) {
		bt_audio_unicast_server_register_cb(&unicast_server_cb);
		bt_conn_cb_register(&conn_callbacks);
#if (CONFIG_BT_VCP_VOL_REND)
		ret = ble_vcs_server_init();
		if (ret) {
			LOG_ERR("VCS server init failed");
			return ret;
		}
#endif /* (CONFIG_BT_VCP_VOL_REND) */

		if (IS_ENABLED(CONFIG_BT_MCC)) {
			ret = ble_mcs_client_init();
			if (ret) {
				LOG_ERR("MCS client init failed");
				return ret;
			}
		}

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
			csip_param.rank = CSIP_HL_RANK;
			ret = bt_pacs_set_location(BT_AUDIO_DIR_SINK, BT_AUDIO_LOCATION_FRONT_LEFT);
			if (ret) {
				LOG_ERR("Location set failed");
				return ret;
			}

		} else if (channel == AUDIO_CH_R) {
			csip_param.rank = CSIP_HR_RANK;
			ret = bt_pacs_set_location(BT_AUDIO_DIR_SINK,
						   BT_AUDIO_LOCATION_FRONT_RIGHT);
			if (ret) {
				LOG_ERR("Location set failed");
				return ret;
			}
		} else {
			LOG_ERR("Channel not supported");
			return -ECANCELED;
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

		ret = bt_cap_acceptor_register(&csip_param, &csip);
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

int le_audio_user_defined_button_press(enum le_audio_user_defined_action action)
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

int le_audio_send(struct encoded_audio enc_audio)
{
#if CONFIG_STREAM_BIDIRECTIONAL
	int ret;
	struct net_buf *buf;
	static bool hci_wrn_printed;

	if (enc_audio.num_ch != 1) {
		LOG_ERR("Num encoded channels must be 1");
		return -EINVAL;
	}

	/* CIS headset only supports one source stream for now */
	if (sources[0].stream->ep->status.state != BT_AUDIO_EP_STATE_STREAMING) {
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
		/* This should never occur because of the iso_tx_pool_alloc check above */
		LOG_WRN("Out of TX buffers");
		return -ENOMEM;
	}

	net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);
	net_buf_add_mem(buf, enc_audio.data, enc_audio.size);

	atomic_inc(&iso_tx_pool_alloc);
	ret = bt_audio_stream_send(sources[0].stream, buf, sources[0].seq_num++,
				   BT_ISO_TIMESTAMP_NONE);
	if (ret < 0) {
		LOG_WRN("Failed to send audio data: %d", ret);
		net_buf_unref(buf);
		atomic_dec(&iso_tx_pool_alloc);
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
