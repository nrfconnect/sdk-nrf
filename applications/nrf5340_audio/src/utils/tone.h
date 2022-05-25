/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __TONE_H__
#define __TONE_H__

#include <stdint.h>
#include <stddef.h>

/**
 * @brief               Generates one full PCM period of a tone with the
 *                      given parameters.
 *
 * @param tone          User provided buffer. Must be large enough to hold
 *                      the generated PCM tone, depending on settings
 * @param tone_size     Resulting tone size
 * @param tone_freq_hz  The desired tone frequency [100..10000] Hz
 * @param smpl_freq_hz  Sampling frequency
 * @param amplitude     Amplitude in the range (0..1]
 *
 * @retval 0            Tone generated
 * @retval -ENXIO       If tone or tone_size is NULL
 * @retval -EINVAL      If smpl_freq_hz == 0 or tone_freq_hz is out of range
 * @retval -EPERM       If amplitude is out of range
 */
int tone_gen(int16_t *tone, size_t *tone_size, uint16_t tone_freq_hz, uint32_t smpl_freq_hz,
	     float amplitude);

#endif /* __TONE_H__ */
