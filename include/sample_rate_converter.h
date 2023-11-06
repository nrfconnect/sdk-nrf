/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Sample Rate Converter library header.
 */

#ifndef _SAMPLE_RATE_CONVERTER_H_
#define _SAMPLE_RATE_CONVERTER_H_

/**
 * @defgroup sample_rate_converter Sample Rate Converter library
 * @brief Implements functionality to increase or decrease the sample rate of a data array using
 * the CMSIS DSP filtering library.
 *
 * @{
 */

#include <zephyr/sys/ring_buffer.h>
#include <dsp/filtering_functions.h>

/**
 * Maximum size for the internal state buffers.
 *
 * The internal state buffer must for each context fulfill the following equations:
 *	Interpolation:
 *		number of filter taps + block size - 1
 *	Decimation:
 *		(number of filter taps / conversion ratio) + block size - 1
 *
 * The equation for interpolation is used as size as this gives the largest number.
 */
#define SAMPLE_RATE_CONVERTER_STATE_BUFFER_SIZE                                                    \
	(CONFIG_SAMPLE_RATE_CONVERTER_BLOCK_SIZE_MAX +                                             \
	 CONFIG_SAMPLE_RATE_CONVERTER_MAX_FILTER_SIZE - 1)

/** Filter types supported by the sample rate converter */
enum sample_rate_converter_filter {
	SAMPLE_RATE_FILTER_TEST = 1
};

/**
 * To maintain filter requirements the input buffer must in some cases store two samples between
 * each block processed.
 */
#define SAMPLE_RATE_CONVERTER_INPUT_BUFFER_NUMBER_OVERFLOW_SAMPLES 2

/**
 * To maintain filter requirements the output buffer must in some cases store six samples between
 * each block processed. The output buffer must be able to store a full block in addition to these
 * overflow samples.
 */
#define SAMPLE_RATE_CONVERTER_OUTPUT_BUFFER_NUMBER_OVERFLOW_SAMPLES 6

#ifdef CONFIG_SAMPLE_RATE_CONVERTER_BIT_DEPTH_16
#define SAMPLE_RATE_CONVERTER_INPUT_BUF_SIZE                                                       \
	(SAMPLE_RATE_CONVERTER_INPUT_BUFFER_NUMBER_OVERFLOW_SAMPLES * sizeof(uint16_t))
#define SAMPLE_RATE_CONVERTER_RINGBUF_SIZE                                                         \
	((CONFIG_SAMPLE_RATE_CONVERTER_BLOCK_SIZE_MAX +                                            \
	  SAMPLE_RATE_CONVERTER_OUTPUT_BUFFER_NUMBER_OVERFLOW_SAMPLES) *                           \
	 sizeof(uint16_t))
#elif CONFIG_SAMPLE_RATE_CONVERTER_BIT_DEPTH_32
#define SAMPLE_RATE_CONVERTER_INPUT_BUF_SIZE                                                       \
	(SAMPLE_RATE_CONVERTER_INPUT_BUFFER_NUMBER_OVERFLOW_SAMPLES * sizeof(uint32_t))
#define SAMPLE_RATE_CONVERTER_RINGBUF_SIZE                                                         \
	((CONFIG_SAMPLE_RATE_CONVERTER_BLOCK_SIZE_MAX +                                            \
	  SAMPLE_RATE_CONVERTER_OUTPUT_BUFFER_NUMBER_OVERFLOW_SAMPLES) *                           \
	 sizeof(uint32_t))
#endif

/** Buffer used for storing input bytes to the sample rate converter */
struct buf_ctx {
	uint8_t buf[SAMPLE_RATE_CONVERTER_INPUT_BUF_SIZE];
	size_t bytes_in_buf;
};

/** Context for the sample rate conversion */
struct sample_rate_converter_ctx {
	/* Input and output sample rate to be used for the conversion. */
	uint32_t sample_rate_input;
	uint32_t sample_rate_output;

	/* The ratio for the current conversion. When the conversion is upsampling the ratio is
	 * positive and negative when downsampling.
	 */
	int conversion_ratio;

	/* Filter type to be used for the conversion. */
	enum sample_rate_converter_filter filter_type;

	/* Buffer used to store input samples between process calls. */
	struct buf_ctx input_buf;

	/* Ring buffer used to store output samples between process calls. */
	struct ring_buf output_ringbuf;
	uint8_t output_ringbuf_data[SAMPLE_RATE_CONVERTER_RINGBUF_SIZE];

	/* Contexts for the CMSIS DSP filter functions. */
	union {
#ifdef CONFIG_SAMPLE_RATE_CONVERTER_BIT_DEPTH_16
		arm_fir_interpolate_instance_q15 fir_interpolate_q15;
		arm_fir_decimate_instance_q15 fir_decimate_q15;
#elif CONFIG_SAMPLE_RATE_CONVERTER_BIT_DEPTH_32
		arm_fir_interpolate_instance_q31 fir_interpolate_q31;
		arm_fir_decimate_instance_q31 fir_decimate_q31;
#endif
	};

	/* State buffers used by the CMSIS DSP filters to keep history of the stream between process
	 * calls.
	 */
#ifdef CONFIG_SAMPLE_RATE_CONVERTER_BIT_DEPTH_16
	q15_t state_buf_15[SAMPLE_RATE_CONVERTER_STATE_BUFFER_SIZE];
#elif CONFIG_SAMPLE_RATE_CONVERTER_BIT_DEPTH_32
	q31_t state_buf_31[SAMPLE_RATE_CONVERTER_STATE_BUFFER_SIZE];
#endif
};

/**
 * @brief	Open the sample rate converter for a new context.
 *
 * @details	Sets the entire context to 0 to ensure it is ready for a new sample rate conversion
 *		context. This should be done before a context is used with a new stream, and does
 *		not need to be called if the parameters for a conversion context changes during the
 *		stream.
 *
 * @param[out]	ctx	Pointer to the sample rate conversion context.
 *
 * @retval	0	On success.
 * @retval	-EINVAL	NULL pointer given for context.
 */
int sample_rate_converter_open(struct sample_rate_converter_ctx *ctx);

/**
 * @brief	Process input samples and produce output samples with new sample rate.
 *
 * @details	Takes samples with the input sample rate, and converts them to the new requested
 *		sample rate by filtering the samples before adding or removing samples. The context
 *		for the sample rate conversion does not need to be initialized before calling
 *		process, and if any parameters change between calls the context will be
 *		re-inititalized. As the process has requirements for the number of input samples
 *		based on the conversion ratio, the module will buffer both input and output bytes
 *		when needed to meet this criteria.
 *
 * @param[in,out]	ctx			Pointer to the sample rate conversion context.
 * @param[in]		filter			Filter type to be used for the conversion.
 * @param[in]		input			Pointer to samples to process.
 * @param[in]		input_size		Size of the input in bytes.
 * @param[in]		input_sample_rate	Sample rate of the input bytes.
 * @param[out]		output			Array that output will be written.
 * @param[in]		output_size		Size of the output array in bytes.
 * @param[out]		output_written		Number of bytes written to output.
 * @param[in]		output_sample_rate	Sample rate of output.
 *
 * @retval	0	On success.
 * @retval	-EINVAL	Invalid parameters for sample rate conversion.
 * @retval	-EFAULT	Output ring buffer has either not enough bytes to output, or not enough
 *			space to store bytes.
 */
int sample_rate_converter_process(struct sample_rate_converter_ctx *ctx,
				  enum sample_rate_converter_filter filter, void const *const input,
				  size_t input_size, uint32_t input_sample_rate, void *const output,
				  size_t output_size, size_t *output_written,
				  uint32_t output_sample_rate);

/**
 * @}
 */

#endif /* _SAMPLE_RATE_CONVERTER_H_ */
