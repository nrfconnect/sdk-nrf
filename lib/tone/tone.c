/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <tone.h>

#include <zephyr/kernel.h>
#include <math.h>
#include <stdio.h>
#include <arm_math.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tone, CONFIG_TONE_LOG_LEVEL);

#define FREQ_LIMIT_LOW	100
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

	for (size_t i = 0; i < samples_for_one_period; i++) {
		float curr_val = i * 2 * PI / samples_for_one_period;
		float32_t res = arm_sin_f32(curr_val);
		/* Generate one sine wave */
		tone[i] = amplitude * res * INT16_MAX;
	}

	/* Configured for bit depth 16 */
	*tone_size = (size_t)samples_for_one_period * 2;

	return 0;
}

static int tone_gen_8(int8_t *tone, size_t *tone_size, uint16_t tone_freq_hz, uint32_t smpl_freq_hz,
		      uint8_t scale, uint32_t max, float amplitude)
{
	uint32_t samples_for_one_period = smpl_freq_hz / tone_freq_hz;

	tone[0] = 0;

	for (size_t i = 1, j = samples_for_one_period - 1; i <= samples_for_one_period / 2;
	     i++, j--) {
		float curr_val = i * 2 * PI / samples_for_one_period;
		float32_t res = arm_sin_f32(curr_val);
		/* Generate one sine wave */
		tone[i] = amplitude * res * max;
		tone[i] <<= scale;
		tone[j] = -tone[i];
	}

	*tone_size = (size_t)samples_for_one_period * sizeof(int8_t);

	return 0;
}

static int tone_gen_16(int16_t *tone, size_t *tone_size, uint16_t tone_freq_hz,
		       uint32_t smpl_freq_hz, uint8_t scale, uint32_t max, float amplitude)
{
	uint32_t samples_for_one_period = smpl_freq_hz / tone_freq_hz;

	tone[0] = 0;

	for (size_t i = 1, j = samples_for_one_period - 1; i <= samples_for_one_period / 2;
	     i++, j--) {
		float curr_val = i * 2 * PI / samples_for_one_period;
		float32_t res = arm_sin_f32(curr_val);
		/* Generate one sine wave */
		tone[i] = amplitude * res * max;
		tone[i] <<= scale;
		tone[j] = -tone[i];
	}

	*tone_size = (size_t)samples_for_one_period * sizeof(int16_t);

	return 0;
}

static int tone_gen_32(int32_t *tone, size_t *tone_size, uint16_t tone_freq_hz,
		       uint32_t smpl_freq_hz, uint8_t scale, int32_t max, float amplitude)
{
	uint32_t samples_for_one_period = smpl_freq_hz / tone_freq_hz;

	tone[0] = 0;

	for (size_t i = 1, j = samples_for_one_period - 1; i <= samples_for_one_period / 2;
	     i++, j--) {
		float curr_val = i * 2 * PI / samples_for_one_period;
		float32_t res = arm_sin_f32(curr_val);
		/* Generate one sine wave */
		tone[i] = amplitude * res * max;
		tone[i] <<= scale;
		tone[j] = -tone[i];
	}

	*tone_size = (size_t)samples_for_one_period * sizeof(int32_t);

	return 0;
}

int tone_gen_size(void *tone, size_t *tone_size, uint16_t tone_freq_hz, uint32_t sample_freq_hz,
		  uint8_t sample_bits, uint8_t carrier_bits, float amplitude)
{
	int ret;
	uint8_t scale;
	int32_t max;

	if (tone == NULL || tone_size == NULL) {
		LOG_DBG("Invalid input parameters\n");
		return -ENXIO;
	}

	if (!sample_freq_hz || tone_freq_hz < FREQ_LIMIT_LOW || tone_freq_hz > FREQ_LIMIT_HIGH ||
	    sample_freq_hz % tone_freq_hz) {
		LOG_DBG("Invalid input parameters\n");
		return -EINVAL;
	}

	if (amplitude > 1 || amplitude <= 0) {
		return -EPERM;
	}

	if (!sample_bits || !carrier_bits || sample_bits > carrier_bits) {
		return -EINVAL;
	}

	if (sample_bits == 8) {
		max = INT8_MAX;
	} else if (sample_bits == 16) {
		max = INT16_MAX;
	} else if (sample_bits == 24) {
		max = 0x007FFFFF;
	} else if (sample_bits == 32) {
		max = INT32_MAX;
	} else {
		return -EINVAL;
	}

	scale = carrier_bits - sample_bits;

	if (carrier_bits == 8) {
		ret = tone_gen_8((int8_t *)tone, tone_size, tone_freq_hz, sample_freq_hz, scale,
				 max, amplitude);
	} else if (carrier_bits == 16) {
		ret = tone_gen_16((int16_t *)tone, tone_size, tone_freq_hz, sample_freq_hz, scale,
				  max, amplitude);
	} else if (carrier_bits == 32) {
		ret = tone_gen_32((int32_t *)tone, tone_size, tone_freq_hz, sample_freq_hz, scale,
				  max, amplitude);
	} else {
		return -EINVAL;
	}

	return 0;
}
