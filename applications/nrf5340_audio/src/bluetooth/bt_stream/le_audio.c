/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "le_audio.h"

#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/audio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(le_audio, CONFIG_BLE_LOG_LEVEL);

int le_audio_ep_state_get(struct bt_bap_ep *ep, uint8_t *state)
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
bool le_audio_ep_state_check(struct bt_bap_ep *ep, enum bt_bap_ep_state state)
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

	int frames_per_sec = 1000000 / dur_us;
	int octets_per_sdu;

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

	ret = bt_bap_ep_get_info(stream->ep, &ep_info);

	if (ret) {
		LOG_WRN("Failed to get ep_info");
		return ret;
	}

	return ep_info.dir;
}

bool le_audio_bitrate_check(const struct bt_audio_codec_cfg *codec)
{
	int ret;
	uint32_t octets_per_sdu;

	ret = le_audio_octets_per_frame_get(codec, &octets_per_sdu);
	if (ret) {
		LOG_ERR("Error retrieving octets per frame: %d", ret);
		return false;
	}

	if (octets_per_sdu < LE_AUDIO_SDU_SIZE_OCTETS(CONFIG_LC3_BITRATE_MIN)) {
		LOG_WRN("Bitrate too low");
		return false;
	} else if (octets_per_sdu > LE_AUDIO_SDU_SIZE_OCTETS(CONFIG_LC3_BITRATE_MAX)) {
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
	if (codec->id == BT_HCI_CODING_FORMAT_LC3) {
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

		if (dir == BT_AUDIO_DIR_SINK) {
			LOG_INF("LC3 codec config for sink:");
		} else if (dir == BT_AUDIO_DIR_SOURCE) {
			LOG_INF("LC3 codec config for source:");
		} else {
			LOG_INF("LC3 codec config for <unknown dir>:");
		}

		LOG_INF("\tFrequency: %d Hz", freq_hz);
		LOG_INF("\tDuration: %d us", dur_us);
		LOG_INF("\tChannel allocation: 0x%x", chan_allocation);
		LOG_INF("\tOctets per frame: %d (%d bps)", octets_per_sdu, bitrate);
		LOG_INF("\tFrames per SDU: %d", frame_blks_per_sdu);
	} else {
		LOG_WRN("Codec is not LC3, codec_id: 0x%2x", codec->id);
	}
}
