/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _CODEC_HELPER_H_
#define _CODEC_HELPER_H_

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <audio_defines.h>

/**
 * @brief Audio codec information
 * Container for codec information which parsed from
 * codec configuration structure.
 */
struct audio_codec_info {
	uint8_t id;
	uint16_t cid;
	uint16_t vid;
	int frequency;
	int frame_duration_us;
	int chan_allocation;
	int octets_per_sdu;
	int bitrate;
	int blocks_per_sdu;
};

/**
 *  @brief Helper macro to convert bitrate to octets per SDU.
 */
#define LE_AUDIO_SDU_SIZE_OCTETS(bitrate) (bitrate / (1000000 / CONFIG_AUDIO_FRAME_DURATION_US) / 8)

/**
 * @brief Print codec information.
 *
 * @param codec	Pointer to the codec information.
 */
void codec_helper_print_codec(const struct audio_codec_info *codec);

/**
 * @brief Check if the bitrate is within the supported range.
 *
 * @param codec Pointer to the codec configuration structure.
 *
 * @retval true If the bitrate is within the supported range.
 */
bool codec_helper_bitrate_check(const struct bt_codec *codec);

/**
 * @brief Get the codec information from codec configuration.
 *
 * @param codec Pointer to the codec configuration structure.
 * @param codec_info Pointer to the codec information.
 */
void codec_helper_get_codec_info(const struct bt_codec *codec, struct audio_codec_info *codec_info);

#endif /* _CODEC_HELPER_H_ */
