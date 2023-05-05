/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "codec_helper.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(codec_helper, CONFIG_BLE_LOG_LEVEL);

void codec_helper_print_codec(const struct audio_codec_info *codec)
{
	if (codec->id == BT_CODEC_LC3_ID) {
		LOG_INF("Codec config for LC3:");
		LOG_INF("\tFrequency: %d Hz", codec->frequency);
		LOG_INF("\tFrame Duration: %d us", codec->frame_duration_us);
		LOG_INF("\tOctets per frame: %d (%d kbps)", codec->octets_per_sdu, codec->bitrate);
		LOG_INF("\tFrames per SDU: %d", codec->blocks_per_sdu);
		if (codec->chan_allocation >= 0) {
			LOG_INF("\tChannel allocation: 0x%x", codec->chan_allocation);
		}
	} else {
		LOG_WRN("Codec is not LC3, codec_id: 0x%2hhx", codec->id);
	}
}

bool codec_helper_bitrate_check(const struct bt_codec *codec)
{
	uint32_t octets_per_sdu = bt_codec_cfg_get_octets_per_frame(codec);

	if (octets_per_sdu < LE_AUDIO_SDU_SIZE_OCTETS(CONFIG_LC3_BITRATE_MIN)) {
		LOG_WRN("Bitrate too low");
		return false;
	} else if (octets_per_sdu > LE_AUDIO_SDU_SIZE_OCTETS(CONFIG_LC3_BITRATE_MAX)) {
		LOG_WRN("Bitrate too high");
		return false;
	}

	return true;
}

void codec_helper_get_codec_info(const struct bt_codec *codec, struct audio_codec_info *codec_info)
{
	if (codec->id == BT_CODEC_LC3_ID) {
		/* LC3 uses the generic LTV format - other codecs might do as well */
		LOG_DBG("Retrieve the codec configuration for LC3");
		codec_info->id = codec->id;
		codec_info->cid = codec->cid;
		codec_info->vid = codec->vid;
		codec_info->frequency = bt_codec_cfg_get_freq(codec);
		codec_info->frame_duration_us = bt_codec_cfg_get_frame_duration_us(codec);
		bt_codec_cfg_get_chan_allocation_val(codec, &codec_info->chan_allocation);
		codec_info->octets_per_sdu = bt_codec_cfg_get_octets_per_frame(codec);
		codec_info->bitrate =
			(codec_info->octets_per_sdu * 8 * 1000000) / codec_info->frame_duration_us;
		codec_info->blocks_per_sdu = bt_codec_cfg_get_frame_blocks_per_sdu(codec, true);
	} else {
		LOG_WRN("Codec is not LC3, codec_id: 0x%2hhx", codec->id);
	}
}
