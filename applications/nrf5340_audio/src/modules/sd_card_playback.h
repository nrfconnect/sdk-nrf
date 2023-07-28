/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _SD_CARD_PLAYBACK_H_
#define _SD_CARD_PLAYBACK_H_

/**
 * @file
 * @defgroup sd_card_playback SD card playback.
 * @{
 * @brief The SD card playback module for nRF5340 Audio.
 */

#include <zephyr/kernel.h>

/**
 * @brief	Check whether or not the SD card playback module is active.
 *
 * @retval	true    Active.
 * @retval	false   Not active.
 */
bool sd_card_playback_is_active(void);

/**
 * @brief	Play audio from a WAV file from the SD card. Only support for mono files.
 *
 * @note	Supports only 48k mono files.
 *
 * @param[in]	filename	Name of file to be played. Path from the root of the SD card is
 *                              accepted.
 *
 * @retval	0       Success.
 * @retval	-EACCES SW codec is not initialized.
 */
int sd_card_playback_wav(char *filename);

/**
 * @brief	Play audio from an LC3 file from the SD card.
 *
 * @note	Supports only mono files.
 *
 * @param[in]	filename	Name of file to be played. Path from the root of the SD card is
 * accepted.
 *
 * @retval	0       Success.
 * @retval	-EACCES SW codec is not initialized.
 */
int sd_card_playback_lc3(char *filename);

/**
 * @brief	Mix the PCM data from the SD card playback module with the audio stream out.
 *
 * @param[in, out]	pcm_a           Buffer into which to mix PCM data from the LC3 module.
 * @param[in]		pcm_a_size	Size of the input buffer.
 *
 * @retval	0       Success.
 * @retval      -EACCES SD card playback is not active.
 * @retval      Otherwise, error from underlying drivers.
 */
int sd_card_playback_mix_with_stream(void *const pcm_a, size_t pcm_a_size);

/**
 * @brief	Initialize the SD card playback module. Create the SD card playback thread.
 *
 * @return	0 on success, otherwise, error from underlying drivers.
 */
int sd_card_playback_init(void);

/**
 * @}
 */

#endif /* _SD_CARD_PLAYBACK_H_ */
