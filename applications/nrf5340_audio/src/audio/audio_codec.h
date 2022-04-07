/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _AUDIO_CODEC_H_
#define _AUDIO_CODEC_H_

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Toggle a test tone on and off
 *
 * @param[in] freq Desired frequency of tone, 0 = off
 *
 * @note A stream must already be running to use this feature
 *
 * @return 0 on success, and -ENOMEM if the frequency is too low (buffer overflow)
 */
int audio_encode_test_tone_set(uint32_t freq);

/**
 * @brief Decode data and then add it to TX FIFO buffer
 *
 * @param[in]	encoded_data		Pointer to encoded data
 * @param[in]	encoded_data_size	Size of encoded data
 * @param[in]	bad_frame		Indication on missed or incomplete frame
 *
 * @return 0 on success, error otherwise
 */
int audio_decode(void const *const encoded_data, size_t encoded_data_size, bool bad_frame);

/**
 * @brief Initialize and start audio on gateway
 */
void audio_gateway_start(void);

/**
 * @brief Initialize and start audio on headset
 */
void audio_headset_start(void);

/**
 * @brief Stop all activities related to audio
 */
void audio_stop(void);

#endif /* _AUDIO_CODEC_H_ */
