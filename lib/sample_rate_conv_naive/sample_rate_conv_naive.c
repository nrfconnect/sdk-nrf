/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "sample_rate_conv_naive.h"

#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sample_rate_conv_naive, 4);

int valid_conversion_check(uint32_t input_sample_rate, uint32_t output_sample_rate)
{
	if (input_sample_rate > output_sample_rate) {
		/* Downsampling */
		if (input_sample_rate != 48000) {
			LOG_ERR("Invalid input sample rate for downsampling %d", input_sample_rate);
			return -EINVAL;
		}

		if ((output_sample_rate != 24000) && (output_sample_rate != 16000)) {
			LOG_ERR("Invalid output sample rate for downsampling %d",
				output_sample_rate);
			return -EINVAL;
		}
	}

	if (input_sample_rate < output_sample_rate) {
		/* Upsampling */
		if (output_sample_rate != 48000) {
			LOG_ERR("Invalid output sample rate for downsampling: %d",
				output_sample_rate);
			return -EINVAL;
		}

		if ((input_sample_rate != 24000) && (input_sample_rate != 16000)) {
			LOG_ERR("Invalid input sample rate for downsampling: %d",
				input_sample_rate);
			return -EINVAL;
		}
	}
	return 0;
}

int sample_rate_conv_naive(void *input, size_t input_size, uint32_t input_sample_rate, void *output,
			   size_t *output_size, uint32_t output_sample_rate, uint8_t pcm_bit_depth)

{
	int ret;
	int ratio;
	size_t i, j, k;
	uint8_t bytes_per_sample = pcm_bit_depth / 8;
	char *pointer_input = (char *)input;
	char *pointer_output = (char *)output;
	char *sample_input = NULL;

	ret = valid_conversion_check(input_sample_rate, output_sample_rate);
	if (ret) {
		return ret;
	}

	if (input_sample_rate < output_sample_rate) {
		/* up */
		ratio = output_sample_rate / input_sample_rate;

		for (i = 0; i < input_size; i += bytes_per_sample) {
			for (j = 0; j < ratio; j++) {
				sample_input = pointer_input;
				for (k = 0; k < bytes_per_sample; k++) {
					*pointer_output++ = *sample_input++;
				}
			}
			pointer_input = sample_input;
		}

		*output_size = input_size * ratio;
	} else if (input_sample_rate > output_sample_rate) {
		/* down */
		ratio = input_sample_rate / output_sample_rate;

		for (i = 0; i < input_size; i += (bytes_per_sample * ratio)) {
			for (j = 0; j < bytes_per_sample; j++) {
				*pointer_output++ = *pointer_input++;
			}

			pointer_input += (bytes_per_sample * (ratio - 1));
		}

		*output_size = input_size / ratio;
	} else {
		/* unchanged */
		LOG_ERR("1 to 1");
		memcpy(output, input, input_size);
		*output_size = input_size;
	}

	return 0;
}
