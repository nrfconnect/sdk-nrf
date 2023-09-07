/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Tone generator header.
 */

#ifndef __SAMPLE_RATE_CONV_NAIVE__
#define __SAMPLE_RATE_CONV_NAIVE__

/**
 * @defgroup tone_gen Tone generator
 * @{
 * @brief Library for generating PCM-based period tones.
 *
 */

#include <stdint.h>
#include <stddef.h>

int sample_rate_conv_naive(void *input, size_t input_size, uint32_t input_sample_rate, void *output,
			   size_t *output_size, uint32_t output_sample_rate, uint8_t pcm_bit_depth);
/**
 * @}
 */

#endif /* __TONE_H__ */
