/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "sample_rate_converter.h"
#include "sample_rate_converter_filter.h"

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sample_rate_converter, CONFIG_SAMPLE_RATE_CONVERTER_LOG_LEVEL);

/**
 * The input buffer must be able to store two samples in addition to the block size to meet
 * filter requirements.
 */
#define INTERNAL_INPUT_BUF_NUMBER_SAMPLES                                                          \
	(CONFIG_SAMPLE_RATE_CONVERTER_BLOCK_SIZE_MAX +                                             \
	 SAMPLE_RATE_CONVERTER_INPUT_BUFFER_NUMBER_OVERFLOW_SAMPLES)

#ifdef CONFIG_SAMPLE_RATE_CONVERTER_BIT_DEPTH_16
#define SAMPLE_RATE_CONVERTER_INTERNAL_INPUT_BUF_SIZE                                              \
	(INTERNAL_INPUT_BUF_NUMBER_SAMPLES * sizeof(uint16_t))
#define SAMPLE_RATE_CONVERTER_INTERNAL_OUTPUT_BUF_SIZE                                             \
	(CONFIG_SAMPLE_RATE_CONVERTER_BLOCK_SIZE_MAX * sizeof(uint16_t))
#elif CONFIG_SAMPLE_RATE_CONVERTER_BIT_DEPTH_32
#define SAMPLE_RATE_CONVERTER_INTERNAL_INPUT_BUF_SIZE                                              \
	(INTERNAL_INPUT_BUF_NUMBER_SAMPLES * sizeof(uint32_t))
#define SAMPLE_RATE_CONVERTER_INTERNAL_OUTPUT_BUF_SIZE                                             \
	(CONFIG_SAMPLE_RATE_CONVERTER_BLOCK_SIZE_MAX * sizeof(uint32_t))
#endif

static int validate_sample_rates(uint32_t sample_rate_input, uint32_t sample_rate_output)
{
	if (sample_rate_input > sample_rate_output) {
		if (sample_rate_input != 48000) {
			LOG_ERR("Invalid input sample rate for downsampling %d", sample_rate_input);
			return -EINVAL;
		}

		if ((sample_rate_output != 24000) && (sample_rate_output != 16000)) {
			LOG_ERR("Invalid output sample rate for downsampling %d",
				sample_rate_output);
			return -EINVAL;
		}
	} else if (sample_rate_input < sample_rate_output) {
		if (sample_rate_output != 48000) {
			LOG_ERR("Invalid output sample rate for upsampling: %d",
				sample_rate_output);
			return -EINVAL;
		}

		if ((sample_rate_input != 24000) && (sample_rate_input != 16000)) {
			LOG_ERR("Invalid input sample rate for upsampling: %d", sample_rate_input);
			return -EINVAL;
		}
	} else {
		LOG_ERR("Input and out sample rates are the same");
		return -EINVAL;
	}
	return 0;
}

static inline int calculate_conversion_ratio(uint32_t sample_rate_input,
					     uint32_t sample_rate_output)
{
	if (sample_rate_input > sample_rate_output) {
		return -(sample_rate_input / sample_rate_output);
	} else {
		return sample_rate_output / sample_rate_input;
	}
}

/**
 * @brief Reconfigures the sample rate converter context.
 *
 * @details Validates and sets all sample rate conversion parameters for the context. If buffering
 *	    is needed for the conversion, the input buffer will be padded with two samples to
 *	    ensure there will always be enough samples for a valid conversion.
 *
 * @param[in,out]	ctx			Pointer to the sample rate conversion context.
 * @param[in]		sample_rate_input	Sample rate of the input samples.
 * @param[in]		sample_rate_output	Sample rate of the output samples.
 * @param[in]		filter			Filter type to use in the conversion.
 *
 * @retval 0 On success.
 * @retval -EINVAL Invalid parameters used to initialize the conversion.
 */
static int sample_rate_converter_reconfigure(struct sample_rate_converter_ctx *ctx,
					     uint32_t sample_rate_input,
					     uint32_t sample_rate_output,
					     enum sample_rate_converter_filter filter)
{
	int ret;
	arm_status arm_err;
	const uint8_t *filter_coeffs;
	size_t filter_size;

	__ASSERT(ctx != NULL, "Context cannot be NULL");

	ret = validate_sample_rates(sample_rate_input, sample_rate_output);
	if (ret) {
		LOG_ERR("Invalid sample rate given (%d)", ret);
		return ret;
	}

	ctx->sample_rate_input = sample_rate_input;
	ctx->sample_rate_output = sample_rate_output;

	ctx->conversion_ratio = calculate_conversion_ratio(sample_rate_input, sample_rate_output);

	ctx->filter_type = filter;
	ret = sample_rate_converter_filter_get(filter, ctx->conversion_ratio,
					       (void const **)&filter_coeffs, &filter_size);
	if (ret) {
		LOG_ERR("Failed to get filter (%d)", ret);
		return ret;
	}

	if (filter_size > CONFIG_SAMPLE_RATE_CONVERTER_MAX_FILTER_SIZE) {
		LOG_ERR("Filter is larger than max size");
		return -EINVAL;
	}

	if (ctx->conversion_ratio == 3) {
		LOG_DBG("Conversion needs buffering, start with the input buffer filled");
		if (IS_ENABLED(CONFIG_SAMPLE_RATE_CONVERTER_BIT_DEPTH_16)) {
			ctx->input_buf.bytes_in_buf =
				SAMPLE_RATE_CONVERTER_INPUT_BUFFER_NUMBER_OVERFLOW_SAMPLES *
				sizeof(uint16_t);
		} else if (IS_ENABLED(CONFIG_SAMPLE_RATE_CONVERTER_BIT_DEPTH_32)) {
			ctx->input_buf.bytes_in_buf =
				SAMPLE_RATE_CONVERTER_INPUT_BUFFER_NUMBER_OVERFLOW_SAMPLES *
				sizeof(uint32_t);
		}
		memset(ctx->input_buf.buf, 0, ctx->input_buf.bytes_in_buf);
	} else {
		ctx->input_buf.bytes_in_buf = 0;
	}

	/* Ring buffer will be used to store spillover output samples */
	ring_buf_init(&ctx->output_ringbuf, ARRAY_SIZE(ctx->output_ringbuf_data),
		      ctx->output_ringbuf_data);

#ifdef CONFIG_SAMPLE_RATE_CONVERTER_BIT_DEPTH_16
	if (ctx->conversion_ratio > 0) {
		arm_err = arm_fir_interpolate_init_q15(&ctx->fir_interpolate_q15,
						       abs(ctx->conversion_ratio), filter_size,
						       (q15_t *)filter_coeffs, ctx->state_buf_15,
						       CONFIG_SAMPLE_RATE_CONVERTER_BLOCK_SIZE_MAX);

	} else {
		arm_err = arm_fir_decimate_init_q15(&ctx->fir_decimate_q15, filter_size,
						    abs(ctx->conversion_ratio),
						    (q15_t *)filter_coeffs, ctx->state_buf_15,
						    CONFIG_SAMPLE_RATE_CONVERTER_BLOCK_SIZE_MAX);
	}
#elif CONFIG_SAMPLE_RATE_CONVERTER_BIT_DEPTH_32
	if (ctx->conversion_ratio > 0) {
		arm_err = arm_fir_interpolate_init_q31(&ctx->fir_interpolate_q31,
						       abs(ctx->conversion_ratio), filter_size,
						       (q31_t *)filter_coeffs, ctx->state_buf_31,
						       CONFIG_SAMPLE_RATE_CONVERTER_BLOCK_SIZE_MAX);
	} else {
		arm_err = arm_fir_decimate_init_q31(&ctx->fir_decimate_q31, filter_size,
						    abs(ctx->conversion_ratio),
						    (q31_t *)filter_coeffs, ctx->state_buf_31,
						    CONFIG_SAMPLE_RATE_CONVERTER_BLOCK_SIZE_MAX);
	}
#endif
	if (arm_err == ARM_MATH_LENGTH_ERROR) {
		LOG_ERR("Filter size is not a multiple of conversion ratio");
		return -EINVAL;
	} else if (arm_err != ARM_MATH_SUCCESS) {
		LOG_ERR("Unknown error during interpolator/decimator initialization (%d)", arm_err);
		return -EINVAL;
	}

	LOG_DBG("Sample rate converter initialized. Input sample rate: %d, Output sample rate: %d, "
		"conversion ratio: %d, filter type: %d",
		ctx->sample_rate_input, ctx->sample_rate_output, ctx->conversion_ratio,
		ctx->filter_type);
	return 0;
}

int sample_rate_converter_open(struct sample_rate_converter_ctx *ctx)
{
	if (ctx == NULL) {
		LOG_ERR("Context cannot be NULL");
		return -EINVAL;
	}

	memset(ctx, 0, sizeof(struct sample_rate_converter_ctx));

	return 0;
}

int sample_rate_converter_process(struct sample_rate_converter_ctx *ctx,
				  enum sample_rate_converter_filter filter, void const *const input,
				  size_t input_size, uint32_t sample_rate_input, void *const output,
				  size_t output_size, size_t *output_written,
				  uint32_t sample_rate_output)
{
	int ret;
	const uint8_t *read_ptr;
	uint8_t *write_ptr;
	size_t samples_to_process;

	uint8_t internal_input_buf[SAMPLE_RATE_CONVERTER_INTERNAL_INPUT_BUF_SIZE];
	uint8_t internal_output_buf[SAMPLE_RATE_CONVERTER_INTERNAL_OUTPUT_BUF_SIZE];

#if CONFIG_SAMPLE_RATE_CONVERTER_BIT_DEPTH_16
	size_t bytes_per_sample = sizeof(uint16_t);
#elif CONFIG_SAMPLE_RATE_CONVERTER_BIT_DEPTH_32
	size_t bytes_per_sample = sizeof(uint32_t);
#endif

	if (input_size % bytes_per_sample != 0) {
		LOG_ERR("Size of input is not a byte multiple");
		return -EINVAL;
	}

	size_t samples_in = input_size / bytes_per_sample;

	if (samples_in > CONFIG_SAMPLE_RATE_CONVERTER_BLOCK_SIZE_MAX) {
		LOG_ERR("Too many samples given as input");
		return -EINVAL;
	}

	if ((ctx == NULL) || (input == NULL) || (output == NULL) || (output_written == NULL)) {
		LOG_ERR("Null pointer received");
		return -EINVAL;
	}

	if ((ctx->sample_rate_input != sample_rate_input) ||
	    (ctx->sample_rate_output != sample_rate_output) || (ctx->filter_type != filter)) {
		LOG_DBG("State has changed, re-initializing filter");
		ret = sample_rate_converter_reconfigure(ctx, sample_rate_input, sample_rate_output,
							filter);
		if (ret) {
			LOG_ERR("Failed to initialize converter (%d)", ret);
			return ret;
		}
	}

	if ((ctx->conversion_ratio < 0) && (samples_in < abs(ctx->conversion_ratio))) {
		LOG_ERR("Number of samples in can not be less than the conversion ratio (%d) when "
			"downsampling",
			ctx->conversion_ratio);
		return -EINVAL;
	}

	if (ctx->conversion_ratio > 0) {
		*output_written = input_size * ctx->conversion_ratio;
	} else {
		*output_written = input_size / abs(ctx->conversion_ratio);
	}

	if (*output_written > output_size) {
		LOG_ERR("Conversion process will produce more bytes than the output buffer can "
			"hold");
		return -EINVAL;
	}

	if (*output_written > SAMPLE_RATE_CONVERTER_INTERNAL_OUTPUT_BUF_SIZE) {
		LOG_ERR("Conversion process will produce more bytes than the internal output "
			"buffer can hold");
		return -EINVAL;
	}

	if (ctx->conversion_ratio == 3) {
		read_ptr = internal_input_buf;
		write_ptr = internal_output_buf;

		if (((samples_in + (ctx->input_buf.bytes_in_buf * bytes_per_sample)) %
		     ctx->conversion_ratio) == 0) {
			size_t extra_samples =
				ctx->conversion_ratio - (samples_in % ctx->conversion_ratio);

			LOG_DBG("Using %d extra samples from input buffer", extra_samples);
			samples_to_process = samples_in + extra_samples;
		} else {
			size_t extra_samples = (samples_in % ctx->conversion_ratio);

			LOG_DBG("Storing %d samples in input buffer for next iteration",
				extra_samples);
			samples_to_process = samples_in - extra_samples;
		}

		/* Merge bytes in input buffer and incoming bytes into the internal buffer
		 * for processing
		 */
		memcpy(internal_input_buf, ctx->input_buf.buf, ctx->input_buf.bytes_in_buf);
		memcpy(internal_input_buf + ctx->input_buf.bytes_in_buf, input, input_size);
	} else {
		write_ptr = output;
		read_ptr = input;
		samples_to_process = samples_in;
	}

#ifdef CONFIG_SAMPLE_RATE_CONVERTER_BIT_DEPTH_16
	if (ctx->conversion_ratio > 0) {
		arm_fir_interpolate_q15(&ctx->fir_interpolate_q15, (q15_t *)read_ptr,
					(q15_t *)write_ptr, samples_to_process);
	} else {
		arm_fir_decimate_q15(&ctx->fir_decimate_q15, (q15_t *)read_ptr, (q15_t *)write_ptr,
				     samples_to_process);
	}
#elif CONFIG_SAMPLE_RATE_CONVERTER_BIT_DEPTH_32
	if (ctx->conversion_ratio > 0) {
		arm_fir_interpolate_q31(&ctx->fir_interpolate_q31, (q31_t *)read_ptr,
					(q31_t *)write_ptr, samples_to_process);
	} else {
		arm_fir_decimate_q31(&ctx->fir_decimate_q31, (q31_t *)read_ptr, (q31_t *)write_ptr,
				     samples_to_process);
	}
#endif

	if (ctx->conversion_ratio != 3) {
		/* Nothing needs to be done in output buffer */
		return 0;
	}

	if (samples_to_process < samples_in) {
		size_t number_overflow_samples = samples_in - samples_to_process;

		ctx->input_buf.bytes_in_buf += number_overflow_samples * bytes_per_sample;
		memcpy(ctx->input_buf.buf, read_ptr + (samples_to_process * bytes_per_sample),
		       ctx->input_buf.bytes_in_buf);
		LOG_DBG("%d overflow samples stored in buffer", number_overflow_samples);
	} else if ((ctx->input_buf.bytes_in_buf) &&
		   (((samples_in + (ctx->input_buf.bytes_in_buf * bytes_per_sample)) %
		     ctx->conversion_ratio) == 0)) {
		uint8_t overflow_samples_used =
			ctx->conversion_ratio - (samples_in % ctx->conversion_ratio);

		ctx->input_buf.bytes_in_buf -= overflow_samples_used * bytes_per_sample;
		LOG_DBG("%d overflow samples has been used", overflow_samples_used);
	}

	int bytes_to_write = samples_to_process * ctx->conversion_ratio * bytes_per_sample;
	uint8_t *ringbuf_write_ptr = (uint8_t *)internal_output_buf;

	LOG_DBG("Writing %d bytes to output buffer", bytes_to_write);
	while (bytes_to_write) {
		uint8_t *data;
		size_t ringbuf_write_size =
			ring_buf_put_claim(&ctx->output_ringbuf, &data, bytes_to_write);
		if (ringbuf_write_size == 0) {
			LOG_ERR("Ring buffer storage exhausted");
			return -EFAULT;
		}

		memcpy(data, ringbuf_write_ptr, ringbuf_write_size);

		ret = ring_buf_put_finish(&ctx->output_ringbuf, ringbuf_write_size);
		if (ret) {
			LOG_ERR("Ringbuf err: %d", ret);
			return -EFAULT;
		}

		ringbuf_write_ptr += ringbuf_write_size;
		bytes_to_write -= ringbuf_write_size;
	}
	int bytes_to_read = input_size * ctx->conversion_ratio;
	uint8_t *ringbuf_output_ptr = (uint8_t *)output;

	LOG_DBG("Reading %d bytes from output_buffer", bytes_to_read);
	while (bytes_to_read) {
		uint8_t *data;
		size_t ringbuf_read_size =
			ring_buf_get_claim(&ctx->output_ringbuf, &data, bytes_to_read);

		if (ringbuf_read_size == 0) {
			LOG_ERR("Ring buffer storage empty");
			return -EFAULT;
		}

		memcpy(ringbuf_output_ptr, data, ringbuf_read_size);
		ringbuf_output_ptr += ringbuf_read_size;
		bytes_to_read -= ringbuf_read_size;

		ret = ring_buf_get_finish(&ctx->output_ringbuf, ringbuf_read_size);
		if (ret) {
			LOG_ERR("Ring buf read err: %d", ret);
			return -EFAULT;
		}
	}

	return 0;
}
