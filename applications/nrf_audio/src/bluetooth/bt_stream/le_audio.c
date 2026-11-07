/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "le_audio.h"

#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <../subsys/bluetooth/audio/bap_endpoint.h>
/* TODO: Can be removed when access is granted to internal structures */
#include <../subsys/bluetooth/audio/cap_internal.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(le_audio, CONFIG_BLE_LOG_LEVEL);

#if CONFIG_SW_CODEC_G722
/**
 * @brief Check complete LTV records before calling the codec field getters.
 *
 * @param[in]	data		LTV bytes; non-NULL when @p size is nonzero.
 * @param[in]	size		Number of available bytes.
 * @param[in]	capability	Select capability field widths instead of configuration widths.
 *
 * @retval	true	All five profile fields occur exactly once with correct widths.
 * @retval	false	Truncated record, malformed/duplicate profile field or missing field.
 *
 * @note Unknown types are permitted if structurally valid, but do not satisfy required fields.
 */
static bool g722_ltv_check(const uint8_t *data, size_t size, bool capability)
{
	/* Bluetooth audio field widths, also used by this vendor-specific profile.
	 * Entries count value bytes only, excluding the LTV length and type bytes.
	 */
	static const uint8_t cap_lengths[] = {
		[BT_AUDIO_CODEC_CAP_TYPE_FREQ] = sizeof(uint16_t),
		[BT_AUDIO_CODEC_CAP_TYPE_DURATION] = sizeof(uint8_t),
		[BT_AUDIO_CODEC_CAP_TYPE_CHAN_COUNT] = sizeof(uint8_t),
		/* Minimum and maximum octets per codec frame are separate 16-bit values. */
		[BT_AUDIO_CODEC_CAP_TYPE_FRAME_LEN] = 2 * sizeof(uint16_t),
		[BT_AUDIO_CODEC_CAP_TYPE_FRAME_COUNT] = sizeof(uint8_t),
	};

	static const uint8_t cfg_lengths[] = {
		[BT_AUDIO_CODEC_CFG_FREQ] = sizeof(uint8_t),
		[BT_AUDIO_CODEC_CFG_DURATION] = sizeof(uint8_t),
		[BT_AUDIO_CODEC_CFG_CHAN_ALLOC] = sizeof(uint32_t),
		[BT_AUDIO_CODEC_CFG_FRAME_LEN] = sizeof(uint16_t),
		[BT_AUDIO_CODEC_CFG_FRAME_BLKS_PER_SDU] = sizeof(uint8_t),
	};

	uint8_t seen = 0;

	for (size_t offset = 0; offset < size;) {
		uint8_t len = data[offset];
		uint8_t type;

		/* L includes T and V but excludes its own byte; require the entire record. */
		if (len < 1 || len >= size - offset) {
			return false;
		}

		type = data[offset + 1];

		if (type > 0 && type < ARRAY_SIZE(cap_lengths)) {
			if ((seen & BIT(type)) ||
			    len - 1 != (capability ? cap_lengths : cfg_lengths)[type]) {
				return false;
			}

			seen |= BIT(type);
		}

		offset += len + 1;
	}

	/* Vendor codecs cannot rely on the omitted-field defaults supplied only for LC3. */
	return seen == GENMASK(5, 1);
}

int le_audio_g722_config_check(const struct bt_audio_codec_cfg *codec)
{
	enum bt_audio_location location;
	int value;
	bool invalid = false;

	if (codec == NULL) {
		LOG_WRN("G.722 codec configuration is NULL");
		return -EINVAL;
	}

	if (!LE_AUDIO_CODEC_IS_G722(codec)) {
		LOG_WRN("G.722 codec identity mismatch: id 0x%02x, company 0x%04x, vendor 0x%04x",
			codec->id, codec->cid, codec->vid);
		invalid = true;
	}

	if (codec->data_len > sizeof(codec->data)) {
		LOG_WRN("G.722 codec configuration data exceeds buffer capacity: %u",
			codec->data_len);
		return -EINVAL;
	}

	if (!g722_ltv_check(codec->data, codec->data_len, false)) {
		LOG_WRN("G.722 codec configuration has malformed, duplicate or missing LTV fields");
		return -EINVAL;
	}

	value = bt_audio_codec_cfg_get_freq(codec);
	if (value != BT_AUDIO_CODEC_CFG_FREQ_16KHZ) {
		LOG_WRN("G.722 requires the 16 kHz frequency value %d (got %d)",
			BT_AUDIO_CODEC_CFG_FREQ_16KHZ, value);
		invalid = true;
	}

	value = bt_audio_codec_cfg_get_frame_dur(codec);
	if (value != BT_AUDIO_CODEC_CFG_DURATION_10) {
		LOG_WRN("G.722 requires the 10 ms frame duration value %d (got %d)",
			BT_AUDIO_CODEC_CFG_DURATION_10, value);
		invalid = true;
	}

	value = bt_audio_codec_cfg_get_octets_per_frame(codec);
	if (value != LE_AUDIO_G722_OCTETS_PER_FRAME) {
		LOG_WRN("G.722 requires %d octets per frame (got %d)",
			LE_AUDIO_G722_OCTETS_PER_FRAME, value);
		invalid = true;
	}

	value = bt_audio_codec_cfg_get_frame_blocks_per_sdu(codec, false);
	if (value != 1) {
		LOG_WRN("G.722 requires one frame block per SDU (got %d)", value);
		invalid = true;
	}

	value = bt_audio_codec_cfg_get_chan_allocation(codec, &location, false);
	if (value != 0) {
		LOG_WRN("Failed to get G.722 channel allocation: %d", value);
		invalid = true;
	} else {
		if (location & ~BT_AUDIO_LOCATION_ANY) {
			LOG_WRN("G.722 channel allocation contains unsupported location bits: "
				"0x%08x",
				(unsigned int)location);
			invalid = true;
		} else if (POPCOUNT(location) > 1) {
			LOG_WRN("G.722 requires mono channel allocation (got 0x%08x)",
				(unsigned int)location);
			invalid = true;
		}
	}

	return invalid ? -EINVAL : 0;
}

int le_audio_g722_cap_check(const struct bt_audio_codec_cap *codec)
{
	struct bt_audio_codec_octets_per_codec_frame octets;
	int value;
	bool invalid = false;

	if (codec == NULL) {
		LOG_WRN("G.722 codec capabilities are NULL");
		return -EINVAL;
	}

	if (!LE_AUDIO_CODEC_IS_G722(codec)) {
		LOG_WRN("G.722 capability identity mismatch: id 0x%02x, company 0x%04x, "
			"vendor 0x%04x",
			codec->id, codec->cid, codec->vid);
		invalid = true;
	}

	if (codec->data_len > sizeof(codec->data)) {
		LOG_WRN("G.722 codec capability data exceeds buffer capacity: %u", codec->data_len);
		return -EINVAL;
	}

	if (!g722_ltv_check(codec->data, codec->data_len, true)) {
		LOG_WRN("G.722 codec capabilities have malformed, duplicate or missing LTV fields");
		return -EINVAL;
	}

	value = bt_audio_codec_cap_get_freq(codec);
	if (value < 0 || !(value & BT_AUDIO_CODEC_CAP_FREQ_16KHZ)) {
		LOG_WRN("G.722 capabilities must support 16 kHz (got %d)", value);
		invalid = true;
	}

	value = bt_audio_codec_cap_get_frame_dur(codec);
	if (value < 0 || !(value & BT_AUDIO_CODEC_CAP_DURATION_10)) {
		LOG_WRN("G.722 capabilities must support 10 ms frames (got %d)", value);
		invalid = true;
	}

	value = bt_audio_codec_cap_get_supported_audio_chan_counts(codec, false);
	if (value < 0 || !(value & BT_AUDIO_CODEC_CAP_CHAN_COUNT_SUPPORT(1))) {
		LOG_WRN("G.722 capabilities must support mono (got %d)", value);
		invalid = true;
	}

	value = bt_audio_codec_cap_get_octets_per_frame(codec, &octets);
	if (value != 0) {
		LOG_WRN("Failed to get G.722 capability octets per frame: %d", value);
		invalid = true;
	} else if (octets.min > LE_AUDIO_G722_OCTETS_PER_FRAME ||
		   octets.max < LE_AUDIO_G722_OCTETS_PER_FRAME) {
		LOG_WRN("G.722 capabilities must include %d octets per frame (got %u-%u)",
			LE_AUDIO_G722_OCTETS_PER_FRAME, octets.min, octets.max);
		invalid = true;
	}

	value = bt_audio_codec_cap_get_max_codec_frames_per_sdu(codec, false);
	if (value < 1) {
		LOG_WRN("G.722 capabilities must support at least one frame per SDU (got %d)",
			value);
		invalid = true;
	}

	return invalid ? -EINVAL : 0;
}
#endif

int le_audio_metadata_populate(struct audio_metadata *meta, const struct bt_bap_stream *stream,
			       const struct bt_iso_recv_info *info,
			       const struct net_buf *audio_frame)
{
	int ret;

	/* Populate the audio frame structure */
	meta->ref_ts_us = info->ts;
	meta->bad_data = 0;
	le_audio_freq_hz_get(stream->codec_cfg, &meta->sample_rate_hz);
	le_audio_duration_us_get(stream->codec_cfg, &meta->data_len_us);

	ret = bt_audio_codec_cfg_get_chan_allocation(stream->codec_cfg, &meta->locations, true);
	if (ret < 0) {
		LOG_ERR("Failed to get channel allocation: %d", ret);
		return ret;
	}

	uint32_t octets_per_frame = 0;

	ret = le_audio_octets_per_frame_get(stream->codec_cfg, &octets_per_frame);
	if (ret < 0) {
		LOG_ERR("Failed to get octets per frame: %d", ret);
		return ret;
	}

	meta->bytes_per_location = octets_per_frame / audio_metadata_num_loc_get(meta);
	if (meta->bytes_per_location == 0) {
		LOG_ERR("Failed to get bytes per location");
		return -EINVAL;
	}

	if (stream->codec_cfg->id == BT_HCI_CODING_FORMAT_LC3) {
		meta->data_coding = LC3;
	} else if (LE_AUDIO_CODEC_IS_G722(stream->codec_cfg)) {
		meta->data_coding = G722;
	} else {
		LOG_ERR("Unsupported codec ID: %d", stream->codec_cfg->id);
		return -EINVAL;
	}

	/* A valid packet will always have a valid timestamp from the Softdevice Controller*/

	if (!(info->flags & BT_ISO_FLAGS_VALID) ||
	    audio_frame->len != bt_audio_codec_cfg_get_octets_per_frame(stream->codec_cfg)) {
		/* Location zero means mono, but a zero bad-data mask would mean no loss. */
		meta->bad_data = meta->locations ? meta->locations : 1;
	}

	return 0;
}

int le_audio_ep_state_get(struct bt_bap_ep const *const ep, uint8_t *state)
{
	int ret;
	struct bt_bap_ep_info ep_info;

	if (ep == NULL) {
		/* If an endpoint is NULL it is not in any of the states */
		return -EINVAL;
	}

	ret = bt_bap_ep_get_info(ep, &ep_info);
	if (ret) {
		LOG_WRN("Unable to get info for ep");
		return ret;
	}

	*state = ep_info.state;

	return 0;
}

/*TODO: Create helper function in host to perform this action. */
bool le_audio_ep_state_check(struct bt_bap_ep const *const ep, enum bt_bap_ep_state state)
{
	int ret;
	uint8_t ep_state;

	ret = le_audio_ep_state_get(ep, &ep_state);
	if (ret) {
		return false;
	}

	if (ep_state == state) {
		return true;
	}

	return false;
}

bool le_audio_ep_qos_configured(struct bt_bap_ep const *const ep)
{
	int ret;
	struct bt_bap_ep_info ep_info;

	if (ep == NULL) {
		LOG_DBG("EP is NULL");
		/* If an endpoint is NULL it is not in any of the states */
		return false;
	}

	ret = bt_bap_ep_get_info(ep, &ep_info);
	if (ret) {
		LOG_WRN("Unable to get info for ep");
		return false;
	}

	if (ep_info.state == BT_BAP_EP_STATE_QOS_CONFIGURED ||
	    ep_info.state == BT_BAP_EP_STATE_ENABLING ||
	    ep_info.state == BT_BAP_EP_STATE_STREAMING ||
	    ep_info.state == BT_BAP_EP_STATE_DISABLING) {
		return true;
	}

	return false;
}

int le_audio_freq_hz_get(const struct bt_audio_codec_cfg *codec, int *freq_hz)
{
	int ret;

	ret = bt_audio_codec_cfg_get_freq(codec);
	if (ret < 0) {
		return ret;
	}

	ret = bt_audio_codec_cfg_freq_to_freq_hz(ret);
	if (ret < 0) {
		return ret;
	}

	*freq_hz = ret;

	return 0;
}

int le_audio_duration_us_get(const struct bt_audio_codec_cfg *codec, int *frame_dur_us)
{
	int ret;

	ret = bt_audio_codec_cfg_get_frame_dur(codec);
	if (ret < 0) {
		return ret;
	}

	ret = bt_audio_codec_cfg_frame_dur_to_frame_dur_us(ret);
	if (ret < 0) {
		return ret;
	}

	*frame_dur_us = ret;

	return 0;
}

int le_audio_octets_per_frame_get(const struct bt_audio_codec_cfg *codec, uint32_t *octets_per_sdu)
{
	int ret;

	ret = bt_audio_codec_cfg_get_octets_per_frame(codec);
	if (ret < 0) {
		return ret;
	}

	*octets_per_sdu = ret;

	return 0;
}

int le_audio_frame_blocks_per_sdu_get(const struct bt_audio_codec_cfg *codec,
				      uint32_t *frame_blks_per_sdu)
{
	int ret;

	ret = bt_audio_codec_cfg_get_frame_blocks_per_sdu(codec, true);
	if (ret < 0) {
		return ret;
	}

	*frame_blks_per_sdu = ret;

	return 0;
}

int le_audio_bitrate_get(const struct bt_audio_codec_cfg *const codec, uint32_t *bitrate)
{
	int ret;
	int dur_us;

	ret = le_audio_duration_us_get(codec, &dur_us);
	if (ret) {
		return ret;
	}

	float frames_per_sec = (float)USEC_PER_SEC / dur_us;
	uint32_t octets_per_sdu;

	ret = le_audio_octets_per_frame_get(codec, &octets_per_sdu);
	if (ret) {
		return ret;
	}

	*bitrate = frames_per_sec * (octets_per_sdu * 8);

	return 0;
}

int le_audio_stream_dir_get(struct bt_bap_stream const *const stream)
{
	int ret;
	struct bt_bap_ep_info ep_info;

	if (unlikely(stream->ep == NULL)) {
		LOG_DBG("Stream has no endpoint");
		return -EINVAL;
	}

	ret = bt_bap_ep_get_info(stream->ep, &ep_info);
	if (ret) {
		LOG_ERR("Failed to get ep_info %d", ret);
		return ret;
	}

	return ep_info.dir;
}

bool le_audio_bitrate_check(const struct bt_audio_codec_cfg *codec)
{
	int ret;
	uint32_t octets_per_sdu;
	int dur_us;

	ret = le_audio_octets_per_frame_get(codec, &octets_per_sdu);
	if (ret) {
		LOG_ERR("Error retrieving octets per frame: %d", ret);
		return false;
	}

	ret = le_audio_duration_us_get(codec, &dur_us);
	if (ret) {
		LOG_ERR("Error retrieving frame duration: %d", ret);
		return false;
	}

	if (octets_per_sdu < LE_AUDIO_SDU_SIZE_OCTETS(CONFIG_LC3_BITRATE_MIN, dur_us)) {
		LOG_WRN("Bitrate too low");
		return false;
	} else if (octets_per_sdu > LE_AUDIO_SDU_SIZE_OCTETS(CONFIG_LC3_BITRATE_MAX, dur_us)) {
		LOG_WRN("Bitrate too high");
		return false;
	}

	return true;
}

bool le_audio_freq_check(const struct bt_audio_codec_cfg *codec)
{
	int ret;
	uint32_t frequency_hz;

	ret = le_audio_freq_hz_get(codec, &frequency_hz);
	if (ret) {
		LOG_ERR("Error retrieving sampling rate: %d", ret);
		return false;
	}

	switch (frequency_hz) {
	case 8000U:
		return (BT_AUDIO_CODEC_CAP_FREQ_8KHZ & (BT_AUDIO_CODEC_CAPABILIY_FREQ));
	case 11025U:
		return (BT_AUDIO_CODEC_CAP_FREQ_11KHZ & (BT_AUDIO_CODEC_CAPABILIY_FREQ));
	case 16000U:
		return (BT_AUDIO_CODEC_CAP_FREQ_16KHZ & (BT_AUDIO_CODEC_CAPABILIY_FREQ));
	case 22050U:
		return (BT_AUDIO_CODEC_CAP_FREQ_22KHZ & (BT_AUDIO_CODEC_CAPABILIY_FREQ));
	case 24000U:
		return (BT_AUDIO_CODEC_CAP_FREQ_24KHZ & (BT_AUDIO_CODEC_CAPABILIY_FREQ));
	case 32000U:
		return (BT_AUDIO_CODEC_CAP_FREQ_32KHZ & (BT_AUDIO_CODEC_CAPABILIY_FREQ));
	case 44100U:
		return (BT_AUDIO_CODEC_CAP_FREQ_44KHZ & (BT_AUDIO_CODEC_CAPABILIY_FREQ));
	case 48000U:
		return (BT_AUDIO_CODEC_CAP_FREQ_48KHZ & (BT_AUDIO_CODEC_CAPABILIY_FREQ));
	case 88200U:
		return (BT_AUDIO_CODEC_CAP_FREQ_88KHZ & (BT_AUDIO_CODEC_CAPABILIY_FREQ));
	case 96000U:
		return (BT_AUDIO_CODEC_CAP_FREQ_96KHZ & (BT_AUDIO_CODEC_CAPABILIY_FREQ));
	case 176400U:
		return (BT_AUDIO_CODEC_CAP_FREQ_176KHZ & (BT_AUDIO_CODEC_CAPABILIY_FREQ));
	case 192000U:
		return (BT_AUDIO_CODEC_CAP_FREQ_192KHZ & (BT_AUDIO_CODEC_CAPABILIY_FREQ));
	case 384000U:
		return (BT_AUDIO_CODEC_CAP_FREQ_384KHZ & (BT_AUDIO_CODEC_CAPABILIY_FREQ));
	default:
		return false;
	}
}

void le_audio_print_codec(const struct bt_audio_codec_cfg *codec, enum bt_audio_dir dir)
{
	if (codec->id == BT_HCI_CODING_FORMAT_LC3 || LE_AUDIO_CODEC_IS_G722(codec)) {
		/* LC3 uses the generic LTV format - other codecs might do as well */
		int ret;
		enum bt_audio_location chan_allocation;
		int freq_hz;
		int dur_us;
		uint32_t octets_per_sdu;
		int frame_blks_per_sdu;
		uint32_t bitrate;

		ret = le_audio_freq_hz_get(codec, &freq_hz);
		if (ret) {
			LOG_ERR("Error retrieving sampling frequency: %d", ret);
			return;
		}

		ret = le_audio_duration_us_get(codec, &dur_us);
		if (ret) {
			LOG_ERR("Error retrieving frame duration: %d", ret);
			return;
		}

		ret = le_audio_octets_per_frame_get(codec, &octets_per_sdu);
		if (ret) {
			LOG_ERR("Error retrieving octets per frame: %d", ret);
			return;
		}

		ret = le_audio_frame_blocks_per_sdu_get(codec, &frame_blks_per_sdu);
		if (ret) {
			LOG_ERR("Error retrieving frame blocks per SDU: %d", ret);
			return;
		}

		ret = bt_audio_codec_cfg_get_chan_allocation(codec, &chan_allocation, false);
		if (ret == -ENODATA) {
			/* Codec channel allocation not set, defaulting to 0 */
			chan_allocation = 0;
		} else if (ret) {
			LOG_ERR("Error retrieving channel allocation: %d", ret);
			return;
		}

		ret = le_audio_bitrate_get(codec, &bitrate);
		if (ret) {
			LOG_ERR("Unable to calculate bitrate: %d", ret);
			return;
		}

		const char *codec_name = LE_AUDIO_CODEC_IS_G722(codec) ? "G.722" : "LC3";

		if (dir == BT_AUDIO_DIR_SINK) {
			LOG_INF("%s codec config for sink:", codec_name);
		} else if (dir == BT_AUDIO_DIR_SOURCE) {
			LOG_INF("%s codec config for source:", codec_name);
		} else {
			LOG_ERR("%s codec config for <unknown dir>:", codec_name);
		}

		LOG_INF("\tFrequency:\t %d Hz", freq_hz);
		LOG_INF("\tDuration:\t %d us", dur_us);
		LOG_INF("\tChan alloc:\t 0x%x", chan_allocation);
		LOG_INF("\tOct pr frame:\t %d (%d bps)", octets_per_sdu, bitrate);
		LOG_INF("\tFrames pr SDU:\t %d", frame_blks_per_sdu);
	} else {
		LOG_WRN("Codec is not LC3, codec_id: 0x%2x", codec->id);
	}
}

int le_audio_print_qos_from_stream(struct bt_bap_stream const *const stream)
{
	if (stream == NULL || stream->qos == NULL) {
		LOG_WRN("Invalid parameters to print QoS");
		return -EINVAL;
	}

	LOG_INF("BAP stream (%p) QoS:", (void *)stream);
	LOG_INF("\tPres dly:\t %d us", stream->qos->pd);
	if (stream->qos->framing == BT_BAP_QOS_CFG_FRAMING_UNFRAMED) {
		LOG_INF("\tFraming:\t Unframed");
	} else if (stream->qos->framing == BT_BAP_QOS_CFG_FRAMING_FRAMED) {
		LOG_INF("\tFraming:\t Framed");
	} else {
		LOG_ERR("\tFraming:\t Unknown (%d)", stream->qos->framing);
	}

	if (stream->qos->phy == BT_BAP_QOS_CFG_1M) {
		LOG_INF("\tRadio PHY:\t 1M");
	} else if (stream->qos->phy == BT_BAP_QOS_CFG_2M) {
		LOG_INF("\tRadio PHY:\t 2M");
	} else if (stream->qos->phy == BT_BAP_QOS_CFG_CODED) {
		LOG_INF("\tRadio PHY:\t Coded");
	} else {
		LOG_ERR("\tRadio PHY:\t Unknown (%d)", stream->qos->phy);
	}

	LOG_INF("\tRecmd. ret.:\t %d", stream->qos->rtn);
	LOG_INF("\tMax SDU size:\t %d", stream->qos->sdu);

#if defined(CONFIG_BT_BAP_BROADCAST_SOURCE) || defined(CONFIG_BT_BAP_UNICAST)
	LOG_INF("\tMax trans lat:\t %d ms", stream->qos->latency);
#endif /*  CONFIG_BT_BAP_BROADCAST_SOURCE || CONFIG_BT_BAP_UNICAST */
	LOG_INF("\tSDU interval:\t %d us", stream->qos->interval);

	return 0;
}

int le_audio_print_unicast_group(struct bt_cap_unicast_group const *const unicast_group)
{
	if (unicast_group == NULL) {
		LOG_WRN("Invalid parameters to print unicast group");
		return -EINVAL;
	}

	LOG_INF("Unicast Group Index: %d", unicast_group->bap_unicast_group->index);

	if (unicast_group->bap_unicast_group->sink_pd != BT_BAP_PD_UNSET) {
		LOG_INF("\tSink pres dly:\t %u us", unicast_group->bap_unicast_group->sink_pd);
	} else {
		LOG_INF("\tSink pres dly:\t Not set");
	}

	if (unicast_group->bap_unicast_group->source_pd != BT_BAP_PD_UNSET) {
		LOG_INF("\t Src pres dly:\t %u us", unicast_group->bap_unicast_group->source_pd);
	} else {
		LOG_INF("\t Src pres dly:\t Not set");
	}

	if (!unicast_group->bap_unicast_group->cig_param.c_to_p_interval) {
		LOG_INF("\tC->P interval:\t Not set (%d)",
			unicast_group->bap_unicast_group->cig_param.c_to_p_interval);
	} else {
		LOG_INF("\tC->P interval:\t %d us",
			unicast_group->bap_unicast_group->cig_param.c_to_p_interval);
	}

	if (!unicast_group->bap_unicast_group->cig_param.c_to_p_latency) {
		LOG_INF("\tC->P latency:\t Not set (%d)",
			unicast_group->bap_unicast_group->cig_param.c_to_p_latency);
	} else {
		LOG_INF("\tC->P latency:\t %d ms",
			unicast_group->bap_unicast_group->cig_param.c_to_p_latency);
	}

	if (!unicast_group->bap_unicast_group->cig_param.p_to_c_interval) {
		LOG_INF("\tP->C interval:\t Not set (%d)",
			unicast_group->bap_unicast_group->cig_param.p_to_c_interval);
	} else {
		LOG_INF("\tP->C interval:\t %d us",
			unicast_group->bap_unicast_group->cig_param.p_to_c_interval);
	}

	if (!unicast_group->bap_unicast_group->cig_param.p_to_c_latency) {
		LOG_INF("\tP->C latency:\t Not set (%d)",
			unicast_group->bap_unicast_group->cig_param.p_to_c_latency);
	} else {
		LOG_INF("\tP->C latency:\t %d ms",
			unicast_group->bap_unicast_group->cig_param.p_to_c_latency);
	}

	return 0;
}
