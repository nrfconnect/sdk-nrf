/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "broadcast_source.h"

#include <zephyr/zbus/zbus.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <../subsys/bluetooth/audio/bap_iso.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>

/* TODO: Remove when a get_info function is implemented in host */
#include <../subsys/bluetooth/audio/bap_endpoint.h>

#include "macros_common.h"
#include "le_audio.h"
#include "nrf5340_audio_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(broadcast_source, CONFIG_BROADCAST_SOURCE_LOG_LEVEL);

BUILD_ASSERT(CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT <= 2,
	     "A maximum of two audio streams are currently supported");

ZBUS_CHAN_DEFINE(le_audio_chan, struct le_audio_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
		 ZBUS_MSG_INIT(0));

ZBUS_CHAN_DEFINE(sdu_ref_chan, struct sdu_ref_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
		 ZBUS_MSG_INIT(0));

#if CONFIG_BT_AUDIO_BROADCAST_CONFIGURABLE
#define BT_BAP_LC3_BROADCAST_PRESET_NRF5340_AUDIO                                                  \
	BT_BAP_LC3_PRESET_CONFIGURABLE(                                                            \
		BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT,                      \
		BT_AUDIO_CONTEXT_TYPE_MEDIA, CONFIG_BT_AUDIO_BITRATE_BROADCAST_SRC)

#elif CONFIG_BT_BAP_BROADCAST_16_2_1
#define BT_BAP_LC3_BROADCAST_PRESET_NRF5340_AUDIO                                                  \
	BT_BAP_LC3_BROADCAST_PRESET_16_2_1(BT_AUDIO_LOCATION_FRONT_LEFT |                          \
						   BT_AUDIO_LOCATION_FRONT_RIGHT,                  \
					   BT_AUDIO_CONTEXT_TYPE_MEDIA)

#elif CONFIG_BT_BAP_BROADCAST_24_2_1
#define BT_BAP_LC3_BROADCAST_PRESET_NRF5340_AUDIO                                                  \
	BT_BAP_LC3_BROADCAST_PRESET_24_2_1(BT_AUDIO_LOCATION_FRONT_LEFT |                          \
						   BT_AUDIO_LOCATION_FRONT_RIGHT,                  \
					   BT_AUDIO_CONTEXT_TYPE_MEDIA)

#elif CONFIG_BT_BAP_BROADCAST_16_2_2
#define BT_BAP_LC3_BROADCAST_PRESET_NRF5340_AUDIO                                                  \
	BT_BAP_LC3_BROADCAST_PRESET_16_2_2(BT_AUDIO_LOCATION_FRONT_LEFT |                          \
						   BT_AUDIO_LOCATION_FRONT_RIGHT,                  \
					   BT_AUDIO_CONTEXT_TYPE_MEDIA)

#elif CONFIG_BT_BAP_BROADCAST_24_2_2
#define BT_BAP_LC3_BROADCAST_PRESET_NRF5340_AUDIO                                                  \
	BT_BAP_LC3_BROADCAST_PRESET_24_2_2(BT_AUDIO_LOCATION_FRONT_LEFT |                          \
						   BT_AUDIO_LOCATION_FRONT_RIGHT,                  \
					   BT_AUDIO_CONTEXT_TYPE_MEDIA)

#else
#error Unsupported LC3 codec preset for broadcast
#endif /* CONFIG_BT_AUDIO_BROADCAST_CONFIGURABLE */

#define STANDARD_QUALITY_16KHZ 16000
#define STANDARD_QUALITY_24KHZ 24000
#define HIGH_QUALITY_48KHZ     48000

#define HCI_ISO_BUF_ALLOC_PER_CHAN 2
/* For being able to dynamically define iso_tx_pools */
#define NET_BUF_POOL_ITERATE(i, _)                                                                 \
	NET_BUF_POOL_FIXED_DEFINE(iso_tx_pool_##i, HCI_ISO_BUF_ALLOC_PER_CHAN,                     \
				  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU), 8, NULL);
#define NET_BUF_POOL_PTR_ITERATE(i, ...) IDENTITY(&iso_tx_pool_##i)
LISTIFY(CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT, NET_BUF_POOL_ITERATE, (;))

/* clang-format off */
static struct net_buf_pool *iso_tx_pools[] = { LISTIFY(CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT,
						       NET_BUF_POOL_PTR_ITERATE, (,)) };
/* clang-format on */

static struct bt_cap_broadcast_source *broadcast_source;

static struct bt_cap_stream cap_streams[CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT];

static struct bt_bap_lc3_preset lc3_preset = BT_BAP_LC3_BROADCAST_PRESET_NRF5340_AUDIO;

static struct bt_le_ext_adv *adv;
static atomic_t iso_tx_pool_alloc[CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT];
static uint32_t seq_num[CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT];
static bool initialized;
static bool delete_broadcast_src;

#if (CONFIG_AURACAST)
NET_BUF_SIMPLE_DEFINE(pba_buf, BT_UUID_SIZE_16 + 2);
static struct bt_data ext_ad[4];
#else
static struct bt_data ext_ad[3];
#endif /* (CONFIG_AURACAST) */

static struct bt_data per_ad[1];

static void le_audio_event_publish(enum le_audio_evt_type event)
{
	int ret;
	struct le_audio_msg msg;

	msg.event = event;

	ret = zbus_chan_pub(&le_audio_chan, &msg, LE_AUDIO_ZBUS_EVENT_WAIT_TIME);
	ERR_CHK(ret);
}

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

static int get_stream_index(struct bt_bap_stream *stream, uint8_t *index)
{
	for (int i = 0; i < ARRAY_SIZE(cap_streams); i++) {
		if (&cap_streams[i].bap_stream == stream) {
			*index = i;
			return 0;
		}
	}

	LOG_WRN("Stream %p not found", (void *)stream);

	return -EINVAL;
}

static void stream_sent_cb(struct bt_bap_stream *stream)
{
	static uint32_t sent_cnt[ARRAY_SIZE(cap_streams)];
	uint8_t index = 0;

	get_stream_index(stream, &index);

	if (atomic_get(&iso_tx_pool_alloc[index])) {
		atomic_dec(&iso_tx_pool_alloc[index]);
	} else {
		LOG_WRN("Decreasing atomic variable for stream %d failed", index);
	}

	sent_cnt[index]++;

	if ((sent_cnt[index] % 1000U) == 0U) {
		LOG_DBG("Sent %d total ISO packets on stream %d", sent_cnt[index], index);
	}
}

static void stream_started_cb(struct bt_bap_stream *stream)
{
	uint8_t index = 0;

	get_stream_index(stream, &index);
	seq_num[index] = 0;

	le_audio_event_publish(LE_AUDIO_EVT_STREAMING);

	/* NOTE: The string below is used by the Nordic CI system */
	LOG_INF("Broadcast source %p started", (void *)stream);
}

static void stream_stopped_cb(struct bt_bap_stream *stream, uint8_t reason)
{
	int ret;

	le_audio_event_publish(LE_AUDIO_EVT_NOT_STREAMING);

	LOG_INF("Broadcast source %p stopped. Reason: %d", (void *)stream, reason);

	if (delete_broadcast_src && broadcast_source != NULL) {
		ret = bt_cap_initiator_broadcast_audio_delete(broadcast_source);
		if (ret) {
			LOG_ERR("Unable to delete broadcast source %p", (void *)stream);
			delete_broadcast_src = false;
			return;
		}

		broadcast_source = NULL;

		LOG_INF("Broadcast source %p deleted", (void *)stream);

		delete_broadcast_src = false;
	}
}

static struct bt_bap_stream_ops stream_ops = {
	.sent = stream_sent_cb,
	.started = stream_started_cb,
	.stopped = stream_stopped_cb,
};

#if (CONFIG_AURACAST)
static void public_broadcast_features_set(uint8_t *features)
{
	int freq = bt_audio_codec_cfg_get_freq(&lc3_preset.codec_cfg);

	if (features == NULL) {
		LOG_ERR("No pointer to features");
		return;
	}

	if (IS_ENABLED(CONFIG_BT_AUDIO_BROADCAST_ENCRYPTED)) {
		*features |= 0x01;
	}

	if (freq == STANDARD_QUALITY_16KHZ || freq == STANDARD_QUALITY_24KHZ) {
		*features |= 0x02;
	} else if (freq == HIGH_QUALITY_48KHZ) {
		*features |= 0x04;
	} else {
		LOG_WRN("%dkHz is not compatible with Auracast, choose 16kHz, 24kHz or 48kHz",
			freq);
	}
}
#endif /* (CONFIG_AURACAST) */

static int adv_create(void)
{
	int ret;
	uint32_t broadcast_id = 0;

	/* Broadcast Audio Streaming Endpoint advertising data */
	NET_BUF_SIMPLE_DEFINE_STATIC(brdcst_id_buf, BT_UUID_SIZE_16 + BT_AUDIO_BROADCAST_ID_SIZE);
	/* Buffer for Appearance */
	NET_BUF_SIMPLE_DEFINE_STATIC(brdcst_appearance_buf, BT_UUID_SIZE_16);
	/* Buffer for Public Broadcast Announcement */
	NET_BUF_SIMPLE_DEFINE_STATIC(base_buf, 128);

	if (IS_ENABLED(CONFIG_BT_AUDIO_USE_BROADCAST_ID_RANDOM)) {
		ret = bt_cap_initiator_broadcast_get_id(broadcast_source, &broadcast_id);
		if (ret) {
			LOG_ERR("Unable to get broadcast ID: %d", ret);
			return ret;
		}
	} else {
		broadcast_id = CONFIG_BT_AUDIO_BROADCAST_ID_FIXED;
	}

	ext_ad[0] = (struct bt_data)BT_DATA(BT_DATA_BROADCAST_NAME, CONFIG_BT_AUDIO_BROADCAST_NAME,
					    sizeof(CONFIG_BT_AUDIO_BROADCAST_NAME) - 1);

	/* Setup extended advertising data */
	net_buf_simple_add_le16(&brdcst_id_buf, BT_UUID_BROADCAST_AUDIO_VAL);
	net_buf_simple_add_le24(&brdcst_id_buf, broadcast_id);

	ext_ad[1].data_len = brdcst_id_buf.len;
	ext_ad[1].type = BT_DATA_SVC_DATA16;
	ext_ad[1].data = brdcst_id_buf.data;

	net_buf_simple_add_le16(&brdcst_appearance_buf, CONFIG_BT_DEVICE_APPEARANCE);

	ext_ad[2].data_len = brdcst_appearance_buf.len;
	ext_ad[2].type = BT_DATA_GAP_APPEARANCE;
	ext_ad[2].data = brdcst_appearance_buf.data;

#if (CONFIG_AURACAST)
	uint8_t pba_features = 0;
	public_broadcast_features_set(&pba_features);

	net_buf_simple_add_le16(&pba_buf, 0x1856);
	net_buf_simple_add_u8(&pba_buf, pba_features);
	/* No metadata, set length to 0 */
	net_buf_simple_add_u8(&pba_buf, 0x00);

	ext_ad[3].data_len = pba_buf.len;
	ext_ad[3].type = BT_DATA_SVC_DATA16;
	ext_ad[3].data = pba_buf.data;
#endif /* (CONFIG_AURACAST) */

	/* Setup periodic advertising data */
	ret = bt_cap_initiator_broadcast_get_base(broadcast_source, &base_buf);
	if (ret) {
		LOG_ERR("Failed to get encoded BASE: %d", ret);
		return ret;
	}

	per_ad[0].data_len = base_buf.len;
	per_ad[0].type = BT_DATA_SVC_DATA16;
	per_ad[0].data = base_buf.data;

	return 0;
}

/**
 * @brief Set the channel allocation to a preset codec configuration.
 *
 * @param	data		The preset codec configuration.
 * @param	data_len	Length of @p data
 * @param	loc		Location bitmask setting.
 */
static void bt_audio_codec_allocation_set(uint8_t *data, uint8_t data_len,
					  enum bt_audio_location loc)
{
	data[0] = data_len - 1;
	data[1] = BT_AUDIO_CODEC_CONFIG_LC3_CHAN_ALLOC;
	sys_put_le32((const uint32_t)loc, &data[2]);
}

void broadcast_source_adv_get(const struct bt_data **ext_adv, size_t *ext_adv_size,
			      const struct bt_data **per_adv, size_t *per_adv_size)
{
	*ext_adv = ext_ad;
	*ext_adv_size = ARRAY_SIZE(ext_ad);
	*per_adv = per_ad;
	*per_adv_size = ARRAY_SIZE(per_ad);
}

int broadcast_source_start(struct bt_le_ext_adv *ext_adv)
{
	int ret;

	if (ext_adv != NULL) {
		adv = ext_adv;
	}

	if (adv == NULL) {
		LOG_ERR("No advertising set available");
		return -EINVAL;
	}

	LOG_DBG("Starting broadcast source");

	/* All streams in a broadcast source is in the same state,
	 * so we can just check the first stream
	 */
	if (cap_streams[0].bap_stream.ep == NULL) {
		LOG_ERR("stream->ep is NULL");
		return -ECANCELED;
	}

	if (cap_streams[0].bap_stream.ep->status.state == BT_BAP_EP_STATE_STREAMING) {
		LOG_WRN("Already streaming");
		return -EALREADY;
	}

	ret = bt_cap_initiator_broadcast_audio_start(broadcast_source, adv);
	if (ret) {
		LOG_WRN("Failed to start broadcast, ret: %d", ret);
		return ret;
	}

	return 0;
}

int broadcast_source_stop(void)
{
	int ret;

	/* All streams in a broadcast source is in the same state,
	 * so we can just check the first stream
	 */
	if (cap_streams[0].bap_stream.ep == NULL) {
		LOG_ERR("stream->ep is NULL");
		return -ECANCELED;
	}

	if (cap_streams[0].bap_stream.ep->status.state == BT_BAP_EP_STATE_STREAMING) {
		ret = bt_cap_initiator_broadcast_audio_stop(broadcast_source);
		if (ret) {
			LOG_WRN("Failed to stop broadcast, ret: %d", ret);
			return ret;
		}
	} else {
		LOG_WRN("Not in a streaming state");
		return -EINVAL;
	}

	return 0;
}

int broadcast_source_send(struct le_audio_encoded_audio enc_audio)
{
	int ret;
	static bool wrn_printed[CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT];
	struct net_buf *buf;
	size_t num_streams = ARRAY_SIZE(cap_streams);
	size_t data_size_pr_stream;
	struct bt_iso_tx_info tx_info = {0};
	struct sdu_ref_msg msg;

	if ((enc_audio.num_ch == 1) || (enc_audio.num_ch == num_streams)) {
		data_size_pr_stream = enc_audio.size / enc_audio.num_ch;
	} else {
		LOG_ERR("Num enc channels is %d Must be 1 or num streams", enc_audio.num_ch);
		return -EINVAL;
	}

	if (data_size_pr_stream != LE_AUDIO_SDU_SIZE_OCTETS(CONFIG_LC3_BITRATE)) {
		LOG_ERR("The encoded data size does not match the SDU size");
		return -ECANCELED;
	}

	for (int i = 0; i < num_streams; i++) {
		if (cap_streams[i].bap_stream.ep->status.state != BT_BAP_EP_STATE_STREAMING) {
			LOG_DBG("Stream %d not in streaming state", i);
			continue;
		}

		if (is_iso_buffer_full(i)) {
			if (!wrn_printed[i]) {
				LOG_WRN("HCI ISO TX overrun on ch %d - Single print", i);
				wrn_printed[i] = true;
			}

			return -ENOMEM;
		}

		wrn_printed[i] = false;

		buf = net_buf_alloc(iso_tx_pools[i], K_NO_WAIT);
		if (buf == NULL) {
			/* This should never occur because of the is_iso_buffer_full() check */
			LOG_WRN("Out of TX buffers");
			return -ENOMEM;
		}

		net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);
		if (enc_audio.num_ch == 1) {
			net_buf_add_mem(buf, &enc_audio.data[0], data_size_pr_stream);
		} else {
			net_buf_add_mem(buf, &enc_audio.data[i * data_size_pr_stream],
					data_size_pr_stream);
		}

		atomic_inc(&iso_tx_pool_alloc[i]);

		ret = bt_bap_stream_send(&cap_streams[i].bap_stream, buf, seq_num[i]++,
					 BT_ISO_TIMESTAMP_NONE);
		if (ret < 0) {
			LOG_WRN("Failed to send audio data: %d", ret);
			net_buf_unref(buf);
			atomic_dec(&iso_tx_pool_alloc[i]);
			return ret;
		}
	}

	ret = bt_iso_chan_get_tx_sync(&cap_streams[0].bap_stream.ep->iso->chan, &tx_info);

	if (ret) {
		LOG_DBG("Error getting ISO TX anchor point: %d", ret);
	} else {
		msg.timestamp = tx_info.ts;
		msg.adjust = false;

		ret = zbus_chan_pub(&sdu_ref_chan, &msg, K_NO_WAIT);
		if (ret) {
			LOG_WRN("Failed to publish timestamp: %d", ret);
		}
	}

	return 0;
}

int broadcast_source_disable(void)
{
	int ret;

	if (cap_streams[0].bap_stream.ep->status.state == BT_BAP_EP_STATE_STREAMING) {
		/* Deleting broadcast source in stream_stopped_cb() */
		delete_broadcast_src = true;

		ret = bt_cap_initiator_broadcast_audio_stop(broadcast_source);
		if (ret) {
			return ret;
		}
	} else if (broadcast_source != NULL) {
		ret = bt_cap_initiator_broadcast_audio_delete(broadcast_source);
		if (ret) {
			return ret;
		}

		broadcast_source = NULL;
	}

	initialized = false;

	LOG_DBG("Broadcast source disabled");

	return 0;
}

int broadcast_source_enable(void)
{
	int ret;

	struct bt_cap_initiator_broadcast_stream_param stream_params[ARRAY_SIZE(cap_streams)];
	uint8_t bis_codec_data[ARRAY_SIZE(stream_params)][6];
	struct bt_cap_initiator_broadcast_subgroup_param
		subgroup_params[CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT];
	struct bt_cap_initiator_broadcast_create_param create_param;

	if (initialized) {
		LOG_WRN("Already initialized");
		return -EALREADY;
	}

	LOG_INF("Enabling broadcast_source %s", CONFIG_BT_AUDIO_BROADCAST_NAME);

	(void)memset(cap_streams, 0, sizeof(cap_streams));

	/* Populate BISes */
	for (size_t i = 0; i < ARRAY_SIZE(stream_params); i++) {
		stream_params[i].stream = &cap_streams[i];

		bt_cap_stream_ops_register(stream_params[i].stream, &stream_ops);
		/* TODO: Remove this once the fixed call above has been upstreamed */
		bt_bap_stream_cb_register(&stream_params[i].stream->bap_stream, &stream_ops);

		stream_params[i].data_len = ARRAY_SIZE(bis_codec_data[i]);
		stream_params[i].data = bis_codec_data[i];

		/* The channel allocation is set incrementally */
		bt_audio_codec_allocation_set(stream_params[i].data, stream_params[i].data_len,
					      BIT(i));
	}

	/* Create BIGs */
	for (size_t i = 0U; i < ARRAY_SIZE(subgroup_params); i++) {
		subgroup_params[i].stream_count = ARRAY_SIZE(stream_params);
		subgroup_params[i].stream_params = &stream_params[i];
		subgroup_params[i].codec_cfg = &lc3_preset.codec_cfg;
#if (CONFIG_BT_AUDIO_BROADCAST_IMMEDIATE_FLAG)
		/* Immediate rendering flag */
		subgroup_params[i].codec_cfg->meta_len++;
		subgroup_params[i].codec_cfg->meta[subgroup_params[i].codec_cfg->meta_len] = 0x09;
#endif /* (CONFIG_BT_AUDIO_BROADCAST_IMMEDIATE_FLAG) */
	}

	/* Create broadcast_source */
	create_param.subgroup_count = ARRAY_SIZE(subgroup_params);
	create_param.subgroup_params = subgroup_params;
	create_param.qos = &lc3_preset.qos;

	if (IS_ENABLED(CONFIG_BT_AUDIO_PACKING_INTERLEAVED)) {
		create_param.packing = BT_ISO_PACKING_INTERLEAVED;
	} else {
		create_param.packing = BT_ISO_PACKING_SEQUENTIAL;
	}

	if (IS_ENABLED(CONFIG_BT_AUDIO_BROADCAST_ENCRYPTED)) {
		create_param.encryption = true;
		memset(create_param.broadcast_code, 0, sizeof(create_param.broadcast_code));
		memcpy(create_param.broadcast_code, CONFIG_BT_AUDIO_BROADCAST_ENCRYPTION_KEY,
		       MIN(sizeof(CONFIG_BT_AUDIO_BROADCAST_ENCRYPTION_KEY),
			   sizeof(create_param.broadcast_code)));
	} else {
		create_param.encryption = false;
	}

	LOG_DBG("Creating broadcast source");

	ret = bt_cap_initiator_broadcast_audio_create(&create_param, &broadcast_source);
	if (ret) {
		LOG_ERR("Failed to create broadcast source, ret: %d", ret);
		return ret;
	}

	/* Create advertising set */
	ret = adv_create();
	if (ret) {
		LOG_ERR("Failed to create advertising set");
		return ret;
	}

	initialized = true;

	LOG_DBG("Broadcast source enabled");

	return 0;
}
