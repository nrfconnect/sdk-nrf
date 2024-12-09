/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Tone generator header.
 */

#ifndef __TONE_H__
#define __TONE_H__

/**
 * @defgroup tone_gen Tone generator
 * @{
 * @brief Library for generating PCM-based period tones.
 *
 */

#include <stdint.h>
#include <stddef.h>

/**
 * @brief               Generates one full pulse-code modulation (PCM) period of a tone with the
 *                      given parameters.
 *
 * @note                The returned frequency of the tone is the quotient of the sample_freq_hz
 *                      divided by the tone_freq_hz.
 *
 * @param tone          User provided buffer. Must be large enough to hold
 *                      the generated PCM tone, depending on settings.
 * @param tone_size     Resulting tone size in bytes.
 * @param tone_freq_hz  The desired tone frequency in the range [100..10000] Hz.
 * @param smpl_freq_hz  Sampling frequency.
 * @param amplitude     Amplitude in the range [0..1].
 *
 * @retval 0            Tone generated.
 * @retval -ENXIO       If tone or tone_size is NULL.
 * @retval -EINVAL      If smpl_freq_hz == 0 or tone_freq_hz is out of range.
 * @retval -EPERM       If amplitude is out of range.
 */
int tone_gen(int16_t *tone, size_t *tone_size, uint16_t tone_freq_hz, uint32_t smpl_freq_hz,
	     float amplitude);

/**
 * @brief                 Generates one full pulse-code modulation (PCM) period of a tone with the
 *                        given parameters.
 *
 * @note                  The returned frequency of the tone is the quotient of the sample_freq_hz
 *                        divided by the tone_freq_hz.
 * @note                  The routine returns the tone samples left aligned in the carrier.
 *
 * @param tone            User provided buffer. Must be large enough to hold
 *                        the generated PCM tone, depending on settings.
 * @param tone_size       Resulting tone size in bytes.
 * @param tone_freq_hz    The desired tone frequency in the range [100..10000] Hz.
 * @param sample_freq_hz  Sampling frequency.
 * @param sample_bits     Number of bits to represent a sample (i.e. 8, 16, 24 or 32 bits).
 * @param carrier_bits    Number of bits to carry a sample (i.e. 8, 16 or 32 bit).
 * @param amplitude       Amplitude in the range [0..1].
 *
 * @return 0 if successful, error otherwise.
 */
int tone_gen_size(void *tone, size_t *tone_size, uint16_t tone_freq_hz, uint32_t sample_freq_hz,
		  uint8_t sample_bits, uint8_t carrier_bits, float amplitude);

/**
 * @}
 */

#endif /* __TONE_H__ */
