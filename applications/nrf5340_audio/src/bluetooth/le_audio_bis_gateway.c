/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "le_audio.h"

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/audio.h>
/* TODO: Remove when a get_info function is implemented in host */
#include <../subsys/bluetooth/audio/endpoint.h>
#include <../subsys/bluetooth/audio/audio_iso.h>

#include "macros_common.h"
#include "ctrl_events.h"
#include "audio_datapath.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bis_gateway, CONFIG_BLE_LOG_LEVEL);

BUILD_ASSERT(CONFIG_BT_AUDIO_BROADCAST_SRC_STREAM_COUNT <= 2,
	     "A maximum of two audio streams are currently supported");

#define HCI_ISO_BUF_ALLOC_PER_CHAN 2
#define STANDARD_QUALITY_16KHZ 16000
#define STANDARD_QUALITY_24KHZ 24000
#define HIGH_QUALITY_48KHZ 48000

/* For being able to dynamically define iso_tx_pools */
#define NET_BUF_POOL_ITERATE(i, _)                                                                 \
	NET_BUF_POOL_FIXED_DEFINE(iso_tx_pool_##i, HCI_ISO_BUF_ALLOC_PER_CHAN,                     \
				  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU), 8, NULL);
#define NET_BUF_POOL_PTR_ITERATE(i, ...) IDENTITY(&iso_tx_pool_##i)
LISTIFY(CONFIG_BT_AUDIO_BROADCAST_SRC_STREAM_COUNT, NET_BUF_POOL_ITERATE, (;))

/* clang-format off */
static struct net_buf_pool *iso_tx_pools[] = { LISTIFY(CONFIG_BT_AUDIO_BROADCAST_SRC_STREAM_COUNT,
						       NET_BUF_POOL_PTR_ITERATE, (,)) };
/* clang-format on */

static struct bt_audio_broadcast_source *broadcast_source;

static struct bt_audio_stream audio_streams[CONFIG_BT_AUDIO_BROADCAST_SRC_STREAM_COUNT];

static struct bt_audio_lc3_preset lc3_preset = BT_AUDIO_LC3_BROADCAST_PRESET_NRF5340_AUDIO;

static atomic_t iso_tx_pool_alloc[CONFIG_BT_AUDIO_BROADCAST_SRC_STREAM_COUNT];
static bool delete_broadcast_src;
static uint32_t seq_num[CONFIG_BT_AUDIO_BROADCAST_SRC_STREAM_COUNT];

static struct bt_le_ext_adv *adv;

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

static int get_stream_index(struct bt_audio_stream *stream, uint8_t *index)
{
	for (int i = 0; i < ARRAY_SIZE(audio_streams); i++) {
		if (&audio_streams[i] == stream) {
			*index = i;
			return 0;
		}
	}

	LOG_WRN("Stream %p not found", (void *)stream);

	return -EINVAL;
}

static void stream_sent_cb(struct bt_audio_stream *stream)
{
	static uint32_t sent_cnt[ARRAY_SIZE(audio_streams)];
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

static void stream_started_cb(struct bt_audio_stream *stream)
{
	int ret;
	uint8_t index = 0;

	get_stream_index(stream, &index);
	seq_num[index] = 0;

	ret = ctrl_events_le_audio_event_send(LE_AUDIO_EVT_STREAMING);
	ERR_CHK(ret);

	LOG_INF("Broadcast source %p started", (void *)stream);
}

static void stream_stopped_cb(struct bt_audio_stream *stream)
{
	int ret;

	ret = ctrl_events_le_audio_event_send(LE_AUDIO_EVT_NOT_STREAMING);
	ERR_CHK(ret);

	LOG_INF("Broadcast source %p stopped", (void *)stream);

	if (delete_broadcast_src && broadcast_source != NULL) {
		ret = bt_audio_broadcast_source_delete(broadcast_source);
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

static struct bt_audio_stream_ops stream_ops = { .sent = stream_sent_cb,
						 .started = stream_started_cb,
						 .stopped = stream_stopped_cb };

#if (CONFIG_AURACAST)
static void public_broadcast_features_set(uint8_t *features)
{
	int freq = bt_codec_cfg_get_freq(&lc3_preset.codec);

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

	/* Broadcast Audio Streaming Endpoint advertising data */
	NET_BUF_SIMPLE_DEFINE(ad_buf, BT_UUID_SIZE_16 + BT_AUDIO_BROADCAST_ID_SIZE);
	/* Buffer for Public Broadcast Announcement */
	NET_BUF_SIMPLE_DEFINE(base_buf, 128);

#if (CONFIG_AURACAST)
	NET_BUF_SIMPLE_DEFINE(pba_buf, BT_UUID_SIZE_16 + 2);
	struct bt_data ext_ad[4];
	uint8_t pba_features = 0;
#else
	struct bt_data ext_ad[3];
#endif /* (CONFIG_AURACAST) */
	struct bt_data per_ad;

	uint32_t broadcast_id = 0;

	/* Create a non-connectable non-scannable advertising set */
	ret = bt_le_ext_adv_create(BT_LE_EXT_ADV_NCONN_NAME, NULL, &adv);
	if (ret) {
		LOG_ERR("Unable to create extended advertising set: %d", ret);
		return ret;
	}

	/* Set periodic advertising parameters */
	ret = bt_le_per_adv_set_param(adv, BT_LE_PER_ADV_DEFAULT);
	if (ret) {
		LOG_ERR("Failed to set periodic advertising parameters (ret %d)", ret);
		return ret;
	}

	if (IS_ENABLED(CONFIG_BT_AUDIO_USE_BROADCAST_ID_RANDOM)) {
		ret = bt_audio_broadcast_source_get_id(broadcast_source, &broadcast_id);
		if (ret) {
			LOG_ERR("Unable to get broadcast ID: %d", ret);
			return ret;
		}
	} else {
		broadcast_id = CONFIG_BT_AUDIO_BROADCAST_ID_FIXED;
	}

	ext_ad[0] = (struct bt_data)BT_DATA_BYTES(BT_DATA_BROADCAST_NAME,
						  CONFIG_BT_AUDIO_BROADCAST_NAME);

	/* Setup extended advertising data */
	net_buf_simple_add_le16(&ad_buf, BT_UUID_BROADCAST_AUDIO_VAL);
	net_buf_simple_add_le24(&ad_buf, broadcast_id);

	ext_ad[1] = (struct bt_data)BT_DATA(BT_DATA_SVC_DATA16, ad_buf.data, ad_buf.len);

	ext_ad[2] = (struct bt_data)BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE,
						  (CONFIG_BT_DEVICE_APPEARANCE >> 0) & 0xff,
						  (CONFIG_BT_DEVICE_APPEARANCE >> 8) & 0xff);

#if (CONFIG_AURACAST)
	public_broadcast_features_set(&pba_features);

	net_buf_simple_add_le16(&pba_buf, 0x1856);
	net_buf_simple_add_u8(&pba_buf, pba_features);
	/* No metadata, set length to 0 */
	net_buf_simple_add_u8(&pba_buf, 0x00);

	ext_ad[3] = (struct bt_data)BT_DATA(BT_DATA_SVC_DATA16, pba_buf.data, pba_buf.len);
#endif /* (CONFIG_AURACAST) */

	ret = bt_le_ext_adv_set_data(adv, ext_ad, ARRAY_SIZE(ext_ad), NULL, 0);
	if (ret) {
		LOG_ERR("Failed to set extended advertising data: %d", ret);
		return ret;
	}

	/* Setup periodic advertising data */
	ret = bt_audio_broadcast_source_get_base(broadcast_source, &base_buf);
	if (ret) {
		LOG_ERR("Failed to get encoded BASE: %d", ret);
		return ret;
	}

	per_ad.type = BT_DATA_SVC_DATA16;
	per_ad.data_len = base_buf.len;
	per_ad.data = base_buf.data;

	ret = bt_le_per_adv_set_data(adv, &per_ad, 1);
	if (ret) {
		LOG_ERR("Failed to set periodic advertising data: %d", ret);
		return ret;
	}

	return 0;
}

static int initialize(void)
{
	int ret;
	static bool initialized;
	struct bt_codec_data bis_codec_data =
		BT_CODEC_DATA(BT_CODEC_CONFIG_LC3_FREQ, BT_AUDIO_CODEC_CONFIG_FREQ);
	struct bt_audio_broadcast_source_stream_param stream_params[ARRAY_SIZE(audio_streams)];
	struct bt_audio_broadcast_source_subgroup_param
		subgroup_params[CONFIG_BT_AUDIO_BROADCAST_SRC_SUBGROUP_COUNT];
	struct bt_audio_broadcast_source_create_param create_param;

	if (initialized) {
		LOG_WRN("Already initialized");
		return -EALREADY;
	}

	(void)memset(audio_streams, 0, sizeof(audio_streams));

	for (size_t i = 0; i < ARRAY_SIZE(stream_params); i++) {
		stream_params[i].stream = &audio_streams[i];
		bt_audio_stream_cb_register(stream_params[i].stream, &stream_ops);
		stream_params[i].data_count = 1U;
		stream_params[i].data = &bis_codec_data;
	}

	for (size_t i = 0U; i < ARRAY_SIZE(subgroup_params); i++) {
		subgroup_params[i].params_count = ARRAY_SIZE(stream_params);
		subgroup_params[i].params = &stream_params[i];
		subgroup_params[i].codec = &lc3_preset.codec;
#if (CONFIG_BT_AUDIO_BROADCAST_IMMEDIATE_FLAG)
		/* Immediate rendering flag */
		subgroup_params[i].codec->meta[0].data.type = 0x09;
		subgroup_params[i].codec->meta[0].data.data_len = 0;
		subgroup_params[i].codec->meta_count = 1;
#endif /* (CONFIG_BT_AUDIO_BROADCAST_IMMEDIATE_FLAG) */
	}

	create_param.params_count = ARRAY_SIZE(subgroup_params);
	create_param.params = subgroup_params;
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

	ret = bt_audio_broadcast_source_create(&create_param, &broadcast_source);

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
	return 0;
}

int le_audio_user_defined_button_press(enum le_audio_user_defined_action action)
{
	return 0;
}

int le_audio_config_get(uint32_t *bitrate, uint32_t *sampling_rate)
{
	LOG_WRN("Not possible to get config on broadcast source");
	return -ENXIO;
}

int le_audio_volume_up(void)
{
	LOG_WRN("Not possible to increase volume on/from broadcast source");
	return -ENXIO;
}

int le_audio_volume_down(void)
{
	LOG_WRN("Not possible to decrease volume on/from broadcast source");
	return -ENXIO;
}

int le_audio_volume_mute(void)
{
	LOG_WRN("Not possible to mute volume on/from broadcast source");
	return -ENXIO;
}

int le_audio_play_pause(void)
{
	int ret;

	/* All streams in a broadcast source is in the same state,
	 * so we can just check the first stream
	 */
	if (audio_streams[0].ep == NULL) {
		LOG_ERR("stream->ep is NULL");
		return -ECANCELED;
	}

	if (audio_streams[0].ep->status.state == BT_AUDIO_EP_STATE_STREAMING) {
		ret = bt_audio_broadcast_source_stop(broadcast_source);
		if (ret) {
			LOG_WRN("Failed to stop broadcast, ret: %d", ret);
		}
	} else {
		ret = bt_audio_broadcast_source_start(broadcast_source, adv);
		if (ret) {
			LOG_WRN("Failed to start broadcast, ret: %d", ret);
		}
	}

	return ret;
}

int le_audio_send(struct encoded_audio enc_audio)
{
	int ret;
	static bool wrn_printed[CONFIG_BT_AUDIO_BROADCAST_SRC_STREAM_COUNT];
	struct net_buf *buf;
	size_t num_streams = ARRAY_SIZE(audio_streams);
	size_t data_size_pr_stream;

	if ((enc_audio.num_ch == 1) || (enc_audio.num_ch == num_streams)) {
		data_size_pr_stream = enc_audio.size / enc_audio.num_ch;
	} else {
		LOG_ERR("Num encoded channels must be 1 or equal to num streams");
		return -EINVAL;
	}

	if (data_size_pr_stream != LE_AUDIO_SDU_SIZE_OCTETS(CONFIG_LC3_BITRATE)) {
		LOG_ERR("The encoded data size does not match the SDU size");
		return -ECANCELED;
	}

	for (int i = 0; i < num_streams; i++) {
		if (audio_streams[i].ep->status.state != BT_AUDIO_EP_STATE_STREAMING) {
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

		ret = bt_audio_stream_send(&audio_streams[i], buf, seq_num[i]++,
					   BT_ISO_TIMESTAMP_NONE);
		if (ret < 0) {
			LOG_WRN("Failed to send audio data: %d", ret);
			net_buf_unref(buf);
			atomic_dec(&iso_tx_pool_alloc[i]);
			return ret;
		}
	}

#if (CONFIG_AUDIO_SOURCE_I2S)
	struct bt_iso_tx_info tx_info = { 0 };

	ret = bt_iso_chan_get_tx_sync(&audio_streams[0].ep->iso->chan, &tx_info);

	if (ret) {
		LOG_DBG("Error getting ISO TX anchor point: %d", ret);
	} else {
		audio_datapath_sdu_ref_update(tx_info.ts);
	}
#endif

	return 0;
}

int le_audio_enable(le_audio_receive_cb recv_cb)
{
	int ret;

	ARG_UNUSED(recv_cb);

	LOG_INF("Starting broadcast gateway %s", CONFIG_BT_AUDIO_BROADCAST_NAME);

	ret = initialize();
	if (ret) {
		LOG_ERR("Failed to initialize");
		return ret;
	}

	/* Start extended advertising */
	ret = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (ret) {
		LOG_ERR("Failed to start extended advertising: %d", ret);
		return ret;
	}

	/* Enable Periodic Advertising */
	ret = bt_le_per_adv_start(adv);
	if (ret) {
		LOG_ERR("Failed to enable periodic advertising: %d", ret);
		return ret;
	}

	LOG_DBG("Starting broadcast source");

	ret = bt_audio_broadcast_source_start(broadcast_source, adv);
	if (ret) {
		return ret;
	}

	LOG_DBG("LE Audio enabled");

	return 0;
}

int le_audio_disable(void)
{
	int ret;

	if (audio_streams[0].ep->status.state == BT_AUDIO_EP_STATE_STREAMING) {
		/* Deleting broadcast source in stream_stopped_cb() */
		delete_broadcast_src = true;

		ret = bt_audio_broadcast_source_stop(broadcast_source);
		if (ret) {
			return ret;
		}
	} else if (broadcast_source != NULL) {
		ret = bt_audio_broadcast_source_delete(broadcast_source);
		if (ret) {
			return ret;
		}

		broadcast_source = NULL;
	}

	LOG_DBG("LE Audio disabled");

	return 0;
}
