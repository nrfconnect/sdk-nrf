/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "tone.h"

#include <zephyr.h>
#include <math.h>
#include <stdio.h>
#include <arm_math.h>

#define FREQ_LIMIT_LOW 100
#define FREQ_LIMIT_HIGH 10000

int tone_gen(int16_t *tone, size_t *tone_size, uint16_t tone_freq_hz, uint32_t smpl_freq_hz,
	     float amplitude)
{
	if (tone == NULL || tone_size == NULL) {
		return -ENXIO;
	}

	if (!smpl_freq_hz || tone_freq_hz < FREQ_LIMIT_LOW || tone_freq_hz > FREQ_LIMIT_HIGH) {
		return -EINVAL;
	}

	if (amplitude > 1 || amplitude <= 0) {
		return -EPERM;
	}

	uint32_t samples_for_one_period = smpl_freq_hz / tone_freq_hz;

	for (uint32_t i = 0; i < samples_for_one_period; i++) {
		float curr_val = i * 2 * PI / samples_for_one_period;
		float32_t res = arm_sin_f32(curr_val);
		/* Generate one sine wave */
		tone[i] = amplitude * res * INT16_MAX;
	}

	/* Configured for bit depth 16 */
	*tone_size = (size_t)samples_for_one_period * 2;

	return 0;
}
