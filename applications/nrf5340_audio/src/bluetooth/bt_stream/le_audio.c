/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "le_audio.h"

#include <zephyr/bluetooth/audio/bap.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(le_audio, CONFIG_BLE_LOG_LEVEL);

/*TODO: Create helper function in host to perform this action. */
bool le_audio_ep_state_check(struct bt_bap_ep *ep, enum bt_bap_ep_state state)
{
	int ret;
	struct bt_bap_ep_info ep_info;

	if (ep == NULL) {
		/* If an endpoint is NULL it is not in any of the states */
		return false;
	}

	ret = bt_bap_ep_get_info(ep, &ep_info);
	if (ret) {
		LOG_WRN("Unable to get info for ep");
		return false;
	}

	if (ep_info.state == state) {
		return true;
	}

	return false;
}

int le_audio_freq_hz_get(const struct bt_audio_codec_cfg *codec, int *freq_hz)
{
	int ret;

	ret = bt_audio_codec_cfg_get_freq(codec);
	if (ret < 0) {
		*freq_hz = 0;
		return ret;
	}

	ret = bt_audio_codec_cfg_freq_to_freq_hz(ret);
	if (ret < 0) {
		*freq_hz = 0;
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
		*frame_dur_us = 0;
		return ret;
	}

	ret = bt_audio_codec_cfg_frame_dur_to_frame_dur_us(ret);
	if (ret < 0) {
		*frame_dur_us = 0;
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
		*octets_per_sdu = 0;
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
		*frame_blks_per_sdu = 0;
		return ret;
	}

	*frame_blks_per_sdu = ret;

	return 0;
}
