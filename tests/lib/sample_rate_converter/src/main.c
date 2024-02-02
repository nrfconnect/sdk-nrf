/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <zephyr/tc_util.h>
#include <sample_rate_converter.h>
#include <stdlib.h>

struct sample_rate_converter_ctx conv_ctx;

static void test_setup(void *f)
{
	sample_rate_converter_open(&conv_ctx);
}

#ifdef CONFIG_SAMPLE_RATE_CONVERTER_BIT_DEPTH_16
ZTEST(suite_sample_rate_converter, test_init_valid_decimate_24khz_16bit)
{
	int ret;

	uint32_t input_sample_rate = 48000;
	uint32_t output_sample_rate = 24000;
	uint32_t conversion_ratio = input_sample_rate / output_sample_rate;

	uint16_t input_samples[] = {1000, 2000, 3000, 4000,  5000,  6000,
				    7000, 8000, 9000, 10000, 11000, 12000};
	size_t num_samples = ARRAY_SIZE(input_samples);
	size_t expected_output_samples = num_samples / conversion_ratio;
	uint16_t output_samples[expected_output_samples];

	enum sample_rate_converter_filter filter = SAMPLE_RATE_FILTER_TEST;
	size_t output_written;

	ret = sample_rate_converter_process(
		&conv_ctx, filter, input_samples, num_samples * sizeof(uint16_t), input_sample_rate,
		output_samples, expected_output_samples * sizeof(uint16_t), &output_written,
		output_sample_rate);

	zassert_equal(ret, 0, "Sample rate conversion process failed");
	zassert_equal(conv_ctx.sample_rate_input, input_sample_rate,
		      "Input sample rate not as expected");
	zassert_equal(conv_ctx.sample_rate_output, output_sample_rate,
		      "Output sample rate not as expected");
	zassert_equal(abs(conv_ctx.conversion_ratio), conversion_ratio,
		      "Conversion ratio not as expected");
	zassert_true(conv_ctx.conversion_ratio < 0, "Conversion direction is not as expected");
	zassert_equal(conv_ctx.filter_type, filter, "Filter not as expected");
	zassert_equal(conv_ctx.input_buf.bytes_in_buf, 0, "Bytes in input buffer not as expected");
	zassert_equal(ring_buf_size_get(&conv_ctx.output_ringbuf), 0,
		      "Number of bytes in output ringbuffer not as expected %d",
		      ring_buf_size_get(&conv_ctx.output_ringbuf));

	zassert_equal(output_written, expected_output_samples * sizeof(uint16_t),
		      "Output size was not as expected (%d)", output_written);

	/* Verify output bytes, expected two input samples per output sample */
	for (int i = 0; i < expected_output_samples; i++) {

		uint32_t sample_avg;

		if (i == 0) {
			sample_avg = input_samples[i] / 2;
		} else {
			sample_avg = (input_samples[(i * 2) - 1] + input_samples[i * 2]) / 2;
		}

		zassert_within(output_samples[i], sample_avg, 1);
	}
}

ZTEST(suite_sample_rate_converter, test_init_valid_decimate_16khz_16bit)
{
	int ret;

	uint32_t input_sample_rate = 48000;
	uint32_t output_sample_rate = 16000;
	uint32_t conversion_ratio = input_sample_rate / output_sample_rate;

	int16_t input_samples[] = {1000, 2000, 3000, 4000,  5000,  6000,
				   7000, 8000, 9000, 10000, 11000, 12000};
	size_t num_samples = ARRAY_SIZE(input_samples);
	size_t expected_output_samples = num_samples / conversion_ratio;
	int16_t output_samples[expected_output_samples];

	enum sample_rate_converter_filter filter = SAMPLE_RATE_FILTER_TEST;
	size_t output_written;

	ret = sample_rate_converter_process(
		&conv_ctx, filter, input_samples, num_samples * sizeof(uint16_t), input_sample_rate,
		output_samples, expected_output_samples * sizeof(uint16_t), &output_written,
		output_sample_rate);

	zassert_equal(ret, 0, "Sample rate conversion process failed");
	zassert_equal(conv_ctx.sample_rate_input, input_sample_rate,
		      "Input sample rate not as expected");
	zassert_equal(conv_ctx.sample_rate_output, output_sample_rate,
		      "Output sample rate not as expected");
	zassert_equal(abs(conv_ctx.conversion_ratio), conversion_ratio,
		      "Conversion ratio not as expected");
	zassert_true(conv_ctx.conversion_ratio < 0, "Conversion direction is not as expected");
	zassert_equal(conv_ctx.filter_type, filter, "Filter set incorrectly");
	zassert_equal(conv_ctx.input_buf.bytes_in_buf, 0, "Bytes in input buffer not as expected");
	zassert_equal(ring_buf_size_get(&conv_ctx.output_ringbuf), 0,
		      "Number of bytes in output ringbuffer not as expected %d",
		      ring_buf_size_get(&conv_ctx.output_ringbuf));

	zassert_equal(output_written, expected_output_samples * sizeof(uint16_t),
		      "Output size was not as expected (%d)", output_written);

	/* Verify output bytes, expected three input samples per output sample */
	for (int i = 0; i < expected_output_samples; i++) {
		uint32_t sample_avg;

		if (i == 0) {
			sample_avg = input_samples[i] / 3;
		} else {
			sample_avg = (input_samples[(i * 3) - 2] + input_samples[(i * 3) - 1] +
				      input_samples[i * 3]) /
				     3;
		}

		zassert_within(output_samples[i], sample_avg, 1);
	}
}

ZTEST(suite_sample_rate_converter, test_init_valid_interpolate_24khz_16bit)
{
	int ret;

	uint32_t input_sample_rate = 24000;
	uint32_t output_sample_rate = 48000;
	uint32_t conversion_ratio = output_sample_rate / input_sample_rate;
	enum sample_rate_converter_filter filter = SAMPLE_RATE_FILTER_TEST;

	uint16_t input_samples[] = {2000, 4000, 6000, 8000, 10000, 12000};
	size_t num_samples = ARRAY_SIZE(input_samples);
	size_t expected_output_samples = num_samples * conversion_ratio;
	uint16_t output_samples[expected_output_samples];
	size_t output_written;

	ret = sample_rate_converter_process(
		&conv_ctx, filter, input_samples, num_samples * sizeof(uint16_t), input_sample_rate,
		output_samples, expected_output_samples * sizeof(uint16_t), &output_written,
		output_sample_rate);

	zassert_equal(ret, 0, "Sample rate conversion process failed");
	zassert_equal(conv_ctx.sample_rate_input, input_sample_rate,
		      "Input sample rate not as expected");
	zassert_equal(conv_ctx.sample_rate_output, output_sample_rate,
		      "Output sample rate not as expected");
	zassert_equal(abs(conv_ctx.conversion_ratio), conversion_ratio,
		      "Conversion ratio not as expected");
	zassert_true(conv_ctx.conversion_ratio > 0, "Conversion direction is not as expected");
	zassert_equal(conv_ctx.filter_type, filter, "Filter set incorrectly");
	zassert_equal(conv_ctx.input_buf.bytes_in_buf, 0, "Bytes in input buffer not as expected");
	zassert_equal(ring_buf_size_get(&conv_ctx.output_ringbuf), 0,
		      "Number of bytes in output ringbuffer not as expected %d",
		      ring_buf_size_get(&conv_ctx.output_ringbuf));

	zassert_equal(output_written, expected_output_samples * sizeof(uint16_t),
		      "Output size was not as expected (%d)", output_written);

	/* Verify output bytes, expect two samples in output per input */
	for (int i = 0; i < expected_output_samples; i++) {
		zassert_within(output_samples[i], input_samples[(i / 2)], 1,
			       "Output samples not within expected range from input samples");
	}
}

ZTEST(suite_sample_rate_converter, test_init_valid_interpolate_16khz_16bit)
{
	int ret;

	uint32_t input_sample_rate = 16000;
	uint32_t output_sample_rate = 48000;
	uint32_t conversion_ratio = output_sample_rate / input_sample_rate;
	enum sample_rate_converter_filter filter = SAMPLE_RATE_FILTER_TEST;

	size_t num_samples = 4;
	uint16_t input_one[] = {1000, 2000, 3000, 4000};
	uint16_t input_two[] = {5000, 6000, 7000, 8000};
	uint16_t input_three[] = {9000, 10000, 11000, 12000};
	uint16_t input_four[] = {13000, 14000, 15000, 16000};

	/* As this conversion inputs and produces samples that is not a multiple of the conversion
	 * ratio or the desired output size, the process will be initialized with two 0 samples
	 * in the input buffer. This will cause a delay in the samples processed, which is
	 * reflected by these arrays used for the final comparison.
	 */
	uint16_t compare_one[] = {0, 0, 1000, 2000};
	uint16_t compare_two[] = {3000, 4000, 5000, 6000};
	uint16_t compare_three[] = {7000, 8000, 9000, 10000};
	uint16_t compare_four[] = {11000, 12000, 13000, 14000};

	size_t output_written;
	size_t expected_output_samples = num_samples * conversion_ratio;
	uint16_t output_samples[expected_output_samples];

	/* First run */
	ret = sample_rate_converter_process(
		&conv_ctx, filter, input_one, num_samples * sizeof(uint16_t), input_sample_rate,
		output_samples, expected_output_samples * sizeof(uint16_t), &output_written,
		output_sample_rate);

	zassert_equal(ret, 0, "Sample rate conversion process failed");
	zassert_equal(conv_ctx.sample_rate_input, input_sample_rate,
		      "Input sample rate not as expected %d", conv_ctx.sample_rate_input);
	zassert_equal(conv_ctx.sample_rate_output, output_sample_rate,
		      "Output sample rate not as expected");
	zassert_equal(abs(conv_ctx.conversion_ratio), conversion_ratio,
		      "Conversion ratio not as expected");
	zassert_true(conv_ctx.conversion_ratio > 0, "Conversion direction is not as expected");
	zassert_equal(conv_ctx.filter_type, filter, "Filter set incorrectly");
	zassert_equal(conv_ctx.input_buf.bytes_in_buf, 0,
		      "Bytes in input buffer not as expected %d", conv_ctx.input_buf.bytes_in_buf);
	zassert_equal(ring_buf_size_get(&conv_ctx.output_ringbuf), 12,
		      "Number of bytes in output ringbuffer not as expected %d",
		      ring_buf_size_get(&conv_ctx.output_ringbuf));

	zassert_equal(output_written, expected_output_samples * sizeof(uint16_t),
		      "Output size was not as expected (%d)", output_written);

	/* Verify output bytes, expected three samples in output per input */
	for (int i = 0; i < expected_output_samples; i++) {
		zassert_within(output_samples[i], compare_one[(i / 3)], 1,
			       "Output samples not within expected range from input samples");
	}

	/* Second run */
	ret = sample_rate_converter_process(
		&conv_ctx, filter, input_two, num_samples * sizeof(uint16_t), input_sample_rate,
		output_samples, expected_output_samples * sizeof(uint16_t), &output_written,
		output_sample_rate);

	zassert_equal(ret, 0, "Sample rate conversion process failed");
	zassert_equal(conv_ctx.sample_rate_input, input_sample_rate,
		      "Input sample rate not as expected %d", conv_ctx.sample_rate_input);
	zassert_equal(conv_ctx.sample_rate_output, output_sample_rate,
		      "Output sample rate not as expected");
	zassert_equal(abs(conv_ctx.conversion_ratio), conversion_ratio,
		      "Conversion ratio not as expected");
	zassert_true(conv_ctx.conversion_ratio > 0, "Conversion direction is not as expected");
	zassert_equal(conv_ctx.filter_type, filter, "Filter set incorrectly");
	zassert_equal(conv_ctx.input_buf.bytes_in_buf, 2,
		      "Bytes in input buffer not as expected %d", conv_ctx.input_buf.bytes_in_buf);
	zassert_equal(ring_buf_size_get(&conv_ctx.output_ringbuf), 6,
		      "Number of bytes in output ringbuffer not as expected %d",
		      ring_buf_size_get(&conv_ctx.output_ringbuf));

	uint16_t expected_input_buf_two[] = {8000};

	zassert_mem_equal(conv_ctx.input_buf.buf, expected_input_buf_two,
			  sizeof(expected_input_buf_two));

	zassert_equal(output_written, expected_output_samples * sizeof(uint16_t),
		      "Output size was not as expected (%d)", output_written);

	/* Verify output bytes, expected three samples in output per input */
	for (int i = 0; i < expected_output_samples; i++) {
		zassert_within(output_samples[i], compare_two[(i / 3)], 1,
			       "Output samples not within expected range from input samples");
	}

	/* Third run */
	ret = sample_rate_converter_process(
		&conv_ctx, filter, input_three, num_samples * sizeof(uint16_t), input_sample_rate,
		output_samples, expected_output_samples * sizeof(uint16_t), &output_written,
		output_sample_rate);

	zassert_equal(ret, 0, "Sample rate conversion process failed");
	zassert_equal(conv_ctx.sample_rate_input, input_sample_rate,
		      "Input sample rate not as expected %d", conv_ctx.sample_rate_input);
	zassert_equal(conv_ctx.sample_rate_output, output_sample_rate,
		      "Output sample rate not as expected");
	zassert_equal(abs(conv_ctx.conversion_ratio), conversion_ratio,
		      "Conversion ratio not as expected");
	zassert_true(conv_ctx.conversion_ratio > 0, "Conversion direction is not as expected");
	zassert_equal(conv_ctx.filter_type, filter, "Filter set incorrectly");
	zassert_equal(conv_ctx.input_buf.bytes_in_buf, 4,
		      "Bytes in input buffer not as expected %d", conv_ctx.input_buf.bytes_in_buf);
	zassert_equal(ring_buf_size_get(&conv_ctx.output_ringbuf), 0,
		      "Number of bytes in output ringbuffer not as expected %d",
		      ring_buf_size_get(&conv_ctx.output_ringbuf));

	uint16_t expected_input_buf_three[] = {11000, 12000};

	zassert_mem_equal(conv_ctx.input_buf.buf, expected_input_buf_three,
			  sizeof(expected_input_buf_three));

	zassert_equal(output_written, expected_output_samples * sizeof(uint16_t),
		      "Output size was not as expected (%d)", output_written);

	/* Verify output bytes, expected three samples in output per input */
	for (int i = 0; i < expected_output_samples; i++) {
		zassert_within(output_samples[i], compare_three[(i / 3)], 1,
			       "Output samples not within expected range from input samples");
	}

	/* Fourth run */
	ret = sample_rate_converter_process(
		&conv_ctx, filter, input_four, num_samples * sizeof(uint16_t), input_sample_rate,
		output_samples, expected_output_samples * sizeof(uint16_t), &output_written,
		output_sample_rate);

	zassert_equal(ret, 0, "Sample rate conversion process failed");
	zassert_equal(conv_ctx.sample_rate_input, input_sample_rate,
		      "Input sample rate not as expected %d", conv_ctx.sample_rate_input);
	zassert_equal(conv_ctx.sample_rate_output, output_sample_rate,
		      "Output sample rate not as expected");
	zassert_equal(abs(conv_ctx.conversion_ratio), conversion_ratio,
		      "Conversion ratio not as expected");
	zassert_true(conv_ctx.conversion_ratio > 0, "Conversion direction is not as expected");
	zassert_equal(conv_ctx.filter_type, filter, "Filter set incorrectly");
	zassert_equal(conv_ctx.input_buf.bytes_in_buf, 0,
		      "Bytes in input buffer not as expected %d", conv_ctx.input_buf.bytes_in_buf);
	zassert_equal(ring_buf_size_get(&conv_ctx.output_ringbuf), 12,
		      "Number of bytes in output ringbuffer not as expected %d",
		      ring_buf_size_get(&conv_ctx.output_ringbuf));

	zassert_equal(output_written, expected_output_samples * sizeof(uint16_t),
		      "Output size was not as expected (%d)", output_written);

	/* Verify output bytes, expected three samples in output per input */
	for (int i = 0; i < expected_output_samples; i++) {
		zassert_within(output_samples[i], compare_four[(i / 3)], 1,
			       "Output samples not within expected range from input samples");
	}
}

/* Number of samples must be a define so the large array becomes a fixed size array that can be
 * initialized to all 0's
 */
#define BYTES_PRODUCED_TOO_LARGE_FOR_INTERNAL_BUF_NUM_SAMPLES 300
ZTEST(suite_sample_rate_converter, test_invalid_bytes_produced_too_large_for_internal_buf_16bit)
{
	int ret;

	uint32_t input_sample_rate = 24000;
	uint32_t output_sample_rate = 48000;
	uint32_t conversion_ratio = output_sample_rate / input_sample_rate;

	uint16_t input_samples[BYTES_PRODUCED_TOO_LARGE_FOR_INTERNAL_BUF_NUM_SAMPLES] = {0};
	size_t expected_output_samples =
		BYTES_PRODUCED_TOO_LARGE_FOR_INTERNAL_BUF_NUM_SAMPLES * conversion_ratio;
	uint16_t output_samples[expected_output_samples];

	enum sample_rate_converter_filter filter = SAMPLE_RATE_FILTER_TEST;
	size_t output_written;

	ret = sample_rate_converter_process(
		&conv_ctx, filter, input_samples,
		BYTES_PRODUCED_TOO_LARGE_FOR_INTERNAL_BUF_NUM_SAMPLES * sizeof(uint16_t),
		input_sample_rate, output_samples, expected_output_samples * sizeof(uint16_t),
		&output_written, output_sample_rate);

	zassert_equal(ret, -EINVAL,
		      "Sample rate conversion did not fail when the number of produced output "
		      "samples is larger than the internal output buf");
}

ZTEST(suite_sample_rate_converter, test_valid_process_input_one_sample_interpolate)
{
	int ret;

	uint32_t input_sample_rate = 24000;
	uint32_t output_sample_rate = 48000;
	uint32_t conversion_ratio = output_sample_rate / input_sample_rate;
	enum sample_rate_converter_filter filter = SAMPLE_RATE_FILTER_TEST;

	uint16_t input_samples[] = {1000};
	size_t num_samples = ARRAY_SIZE(input_samples);
	size_t expected_output_samples = num_samples * conversion_ratio;
	uint16_t output_samples[expected_output_samples];
	size_t output_written;

	ret = sample_rate_converter_process(
		&conv_ctx, filter, input_samples, num_samples * sizeof(uint16_t), input_sample_rate,
		output_samples, expected_output_samples * sizeof(uint16_t), &output_written,
		output_sample_rate);

	zassert_equal(ret, 0, "Sample rate conversion process failed");
	zassert_equal(conv_ctx.sample_rate_input, input_sample_rate,
		      "Input sample rate not as expected");
	zassert_equal(conv_ctx.sample_rate_output, output_sample_rate,
		      "Output sample rate not as expected");
	zassert_equal(abs(conv_ctx.conversion_ratio), conversion_ratio,
		      "Conversion ratio not as expected");
	zassert_true(conv_ctx.conversion_ratio > 0, "Conversion direction is not as expected");
	zassert_equal(conv_ctx.filter_type, filter, "Filter not as expected");
	zassert_equal(conv_ctx.input_buf.bytes_in_buf, 0, "Bytes in input buffer not as expected");
	zassert_equal(ring_buf_size_get(&conv_ctx.output_ringbuf), 0,
		      "Number of bytes in output ringbuffer not as expected %d",
		      ring_buf_size_get(&conv_ctx.output_ringbuf));

	zassert_equal(output_written, expected_output_samples * sizeof(uint16_t),
		      "Received %d output samples when none was expected", output_written);

	zassert_within(output_samples[0], input_samples[0], 1);
	zassert_within(output_samples[1], input_samples[0], 1);
}

ZTEST(suite_sample_rate_converter, test_valid_process_input_two_samples_decimate_16bit)
{
	int ret;

	uint32_t input_sample_rate = 48000;
	uint32_t output_sample_rate = 24000;
	uint32_t conversion_ratio = input_sample_rate / output_sample_rate;
	enum sample_rate_converter_filter filter = SAMPLE_RATE_FILTER_TEST;

	uint16_t input_samples[] = {1000, 2000};
	size_t num_samples = ARRAY_SIZE(input_samples);
	size_t expected_output_samples = num_samples / conversion_ratio;
	uint16_t output_samples[expected_output_samples];
	size_t output_written;

	ret = sample_rate_converter_process(
		&conv_ctx, filter, input_samples, num_samples * sizeof(uint16_t), input_sample_rate,
		output_samples, expected_output_samples * sizeof(uint16_t), &output_written,
		output_sample_rate);

	zassert_equal(ret, 0, "Sample rate conversion process failed");
	zassert_equal(conv_ctx.sample_rate_input, input_sample_rate,
		      "Input sample rate not as expected");
	zassert_equal(conv_ctx.sample_rate_output, output_sample_rate,
		      "Output sample rate not as expected");
	zassert_equal(abs(conv_ctx.conversion_ratio), conversion_ratio,
		      "Conversion ratio not as expected");
	zassert_true(conv_ctx.conversion_ratio < 0, "Conversion direction is not as expected");
	zassert_equal(conv_ctx.filter_type, filter, "Filter not as expected");
	zassert_equal(conv_ctx.input_buf.bytes_in_buf, 0, "Bytes in input buffer not as expected");
	zassert_equal(ring_buf_size_get(&conv_ctx.output_ringbuf), 0,
		      "Number of bytes in output ringbuffer not as expected %d",
		      ring_buf_size_get(&conv_ctx.output_ringbuf));

	zassert_equal(output_written, expected_output_samples * sizeof(uint16_t),
		      "Received %d output samples when none was expected", output_written);

	zassert_within(output_samples[0], input_samples[0] / 2, 1);
}
#endif /* CONFIG_SAMPLE_RATE_CONVERTER_BIT_DEPTH_16 */

#if CONFIG_SAMPLE_RATE_CONVERTER_BIT_DEPTH_32
ZTEST(suite_sample_rate_converter, test_init_valid_decimate_24khz_32bit)
{
	int ret;

	uint32_t input_sample_rate = 48000;
	uint32_t output_sample_rate = 24000;
	uint32_t conversion_ratio = input_sample_rate / output_sample_rate;

	uint32_t input_samples[] = {1000, 2000, 3000, 4000,  5000,  6000,
				    7000, 8000, 9000, 10000, 11000, 12000};
	size_t num_samples = ARRAY_SIZE(input_samples);
	size_t expected_output_samples = num_samples / conversion_ratio;
	uint32_t output_samples[expected_output_samples];

	enum sample_rate_converter_filter filter = SAMPLE_RATE_FILTER_TEST;
	size_t output_written;

	ret = sample_rate_converter_process(
		&conv_ctx, filter, input_samples, num_samples * sizeof(uint32_t), input_sample_rate,
		output_samples, expected_output_samples * sizeof(uint32_t), &output_written,
		output_sample_rate);

	zassert_equal(ret, 0, "Sample rate conversion process failed");
	zassert_equal(conv_ctx.sample_rate_input, input_sample_rate,
		      "Input sample rate not as expected");
	zassert_equal(conv_ctx.sample_rate_output, output_sample_rate,
		      "Output sample rate not as expected");
	zassert_equal(abs(conv_ctx.conversion_ratio), conversion_ratio,
		      "Conversion ratio not as expected");
	zassert_true(conv_ctx.conversion_ratio < 0, "Conversion direction is not as expected");
	zassert_equal(conv_ctx.filter_type, filter, "Filter set incorrectly");
	zassert_equal(conv_ctx.input_buf.bytes_in_buf, 0, "Bytes in input buffer not as expected");
	zassert_equal(ring_buf_size_get(&conv_ctx.output_ringbuf), 0,
		      "Number of bytes in output ringbuffer not as expected %d",
		      ring_buf_size_get(&conv_ctx.output_ringbuf));

	zassert_equal(output_written, expected_output_samples * sizeof(uint32_t),
		      "Output size was not as expected (%d)", output_written);

	/* Verify output bytes, expected two input samples per output sample */
	for (int i = 0; i < expected_output_samples; i++) {
		uint32_t sample_avg;

		if (i == 0) {
			sample_avg = input_samples[i] / 2;
		} else {
			sample_avg = (input_samples[(i * 2) - 1] + input_samples[i * 2]) / 2;
		}

		zassert_within(output_samples[i], sample_avg, 1);
	}
}

ZTEST(suite_sample_rate_converter, test_init_valid_decimate_16khz_32bit)
{
	int ret;

	uint32_t input_sample_rate = 48000;
	uint32_t output_sample_rate = 16000;
	uint32_t conversion_ratio = input_sample_rate / output_sample_rate;

	int32_t input_samples[] = {1000, 2000, 3000, 4000,  5000,  6000,
				   7000, 8000, 9000, 10000, 11000, 12000};
	size_t num_samples = ARRAY_SIZE(input_samples);
	size_t expected_output_samples = num_samples / conversion_ratio;
	int32_t output_samples[expected_output_samples];

	enum sample_rate_converter_filter filter = SAMPLE_RATE_FILTER_TEST;
	size_t output_written;

	ret = sample_rate_converter_process(
		&conv_ctx, filter, input_samples, num_samples * sizeof(uint32_t), input_sample_rate,
		output_samples, expected_output_samples * sizeof(uint32_t), &output_written,
		output_sample_rate);

	zassert_equal(ret, 0, "Sample rate conversion process failed");
	zassert_equal(conv_ctx.sample_rate_input, input_sample_rate,
		      "Input sample rate not as expected");
	zassert_equal(conv_ctx.sample_rate_output, output_sample_rate,
		      "Output sample rate not as expected");
	zassert_equal(abs(conv_ctx.conversion_ratio), conversion_ratio,
		      "Conversion ratio not as expected");
	zassert_true(conv_ctx.conversion_ratio < 0, "Conversion direction is not as expected");
	zassert_equal(conv_ctx.filter_type, filter, "Filter set incorrectly");
	zassert_equal(conv_ctx.input_buf.bytes_in_buf, 0, "Bytes in input buffer not as expected");
	zassert_equal(ring_buf_size_get(&conv_ctx.output_ringbuf), 0,
		      "Number of bytes in output ringbuffer not as expected %d",
		      ring_buf_size_get(&conv_ctx.output_ringbuf));

	zassert_equal(output_written, expected_output_samples * sizeof(uint32_t),
		      "Output size was not as expected (%d)", output_written);

	/* Verify output bytes, expected three input samples per output sample */
	for (int i = 0; i < expected_output_samples; i++) {
		uint32_t sample_avg;

		if (i == 0) {
			sample_avg = input_samples[i] / 3;
		} else {
			sample_avg = (input_samples[(i * 3) - 2] + input_samples[(i * 3) - 1] +
				      input_samples[i * 3]) /
				     3;
		}

		zassert_within(output_samples[i], sample_avg, 1);
	}
}

ZTEST(suite_sample_rate_converter, test_init_valid_interpolate_24khz_32bit)
{
	int ret;

	uint32_t input_sample_rate = 24000;
	uint32_t output_sample_rate = 48000;
	uint32_t conversion_ratio = output_sample_rate / input_sample_rate;
	enum sample_rate_converter_filter filter = SAMPLE_RATE_FILTER_TEST;

	uint32_t input_samples[] = {2000, 4000, 6000, 8000, 10000, 12000};
	size_t num_samples = ARRAY_SIZE(input_samples);
	size_t expected_output_samples = num_samples * conversion_ratio;
	uint32_t output_samples[expected_output_samples];

	size_t output_written;

	ret = sample_rate_converter_process(
		&conv_ctx, filter, input_samples, num_samples * sizeof(uint32_t), input_sample_rate,
		output_samples, expected_output_samples * sizeof(uint32_t), &output_written,
		output_sample_rate);

	zassert_equal(ret, 0, "Sample rate conversion process failed");
	zassert_equal(conv_ctx.sample_rate_input, input_sample_rate,
		      "Input sample rate not as expected");
	zassert_equal(conv_ctx.sample_rate_output, output_sample_rate,
		      "Output sample rate not as expected");
	zassert_equal(abs(conv_ctx.conversion_ratio), conversion_ratio,
		      "Conversion ratio not as expected");
	zassert_true(conv_ctx.conversion_ratio > 0, "Conversion direction is not as expected");
	zassert_equal(conv_ctx.filter_type, filter, "Filter set incorrectly");
	zassert_equal(conv_ctx.input_buf.bytes_in_buf, 0, "Bytes in input buffer not as expected");
	zassert_equal(ring_buf_size_get(&conv_ctx.output_ringbuf), 0,
		      "Number of bytes in output ringbuffer not as expected %d",
		      ring_buf_size_get(&conv_ctx.output_ringbuf));

	zassert_equal(output_written, expected_output_samples * sizeof(uint32_t),
		      "Output size was not as expected (%d)", output_written);

	/* Verify output bytes, expected two samples in output per input */
	for (int i = 0; i < expected_output_samples; i++) {
		zassert_within(output_samples[i], input_samples[(i / 2)], 1,
			       "Output samples not within expected range from input samples");
	}
}

ZTEST(suite_sample_rate_converter, test_init_valid_interpolate_16khz_32bit)
{
	int ret;

	uint32_t input_sample_rate = 16000;
	uint32_t output_sample_rate = 48000;
	uint32_t conversion_ratio = output_sample_rate / input_sample_rate;
	enum sample_rate_converter_filter filter = SAMPLE_RATE_FILTER_TEST;

	size_t num_samples = 4;
	int32_t input_one[] = {1000, 2000, 3000, 4000};
	int32_t input_two[] = {5000, 6000, 7000, 8000};
	int32_t input_three[] = {9000, 10000, 11000, 12000};
	int32_t input_four[] = {13000, 14000, 15000, 16000};

	/* As this conversion inputs and produces samples that is not a multiple of the conversion
	 * ratio or the desired output size, the process will be initialized with two 0 samples
	 * in the input buffer. This will cause a delay in the samples processed, which is
	 * reflected by these arrays used for the final comparison.
	 */
	uint16_t compare_one[] = {0, 0, 1000, 2000};
	uint16_t compare_two[] = {3000, 4000, 5000, 6000};
	uint16_t compare_three[] = {7000, 8000, 9000, 10000};
	uint16_t compare_four[] = {11000, 12000, 13000, 14000};

	size_t output_written;
	size_t expected_output_samples = num_samples * conversion_ratio;
	int32_t output_samples[expected_output_samples];

	/* First run */
	ret = sample_rate_converter_process(
		&conv_ctx, filter, input_one, num_samples * sizeof(uint32_t), input_sample_rate,
		output_samples, expected_output_samples * sizeof(uint32_t), &output_written,
		output_sample_rate);

	zassert_equal(ret, 0, "Sample rate conversion process failed");
	zassert_equal(conv_ctx.sample_rate_input, input_sample_rate,
		      "Input sample rate not as expected %d", conv_ctx.sample_rate_input);
	zassert_equal(conv_ctx.sample_rate_output, output_sample_rate,
		      "Output sample rate not as expected");
	zassert_equal(abs(conv_ctx.conversion_ratio), conversion_ratio,
		      "Conversion ratio not as expected");
	zassert_true(conv_ctx.conversion_ratio > 0, "Conversion direction is not as expected");
	zassert_equal(conv_ctx.filter_type, filter, "Filter set incorrectly");
	zassert_equal(conv_ctx.input_buf.bytes_in_buf, 0,
		      "Bytes in input buffer not as expected (%d)",
		      conv_ctx.input_buf.bytes_in_buf);
	zassert_equal(ring_buf_size_get(&conv_ctx.output_ringbuf), 24,
		      "Number of bytes in output ringbuffer not as expected %d",
		      ring_buf_size_get(&conv_ctx.output_ringbuf));

	zassert_equal(output_written, expected_output_samples * sizeof(uint32_t),
		      "Output size was not as expected (%d)", output_written);

	/* Verify output bytes, expected three samples in output per input */
	for (int i = 0; i < expected_output_samples; i++) {
		zassert_within(output_samples[i], compare_one[(i / 3)], 1,
			       "Output samples not within expected range from input samples");
	}

	/* Second run */
	ret = sample_rate_converter_process(
		&conv_ctx, filter, input_two, num_samples * sizeof(uint32_t), input_sample_rate,
		output_samples, expected_output_samples * sizeof(uint32_t), &output_written,
		output_sample_rate);

	zassert_equal(ret, 0, "Sample rate conversion process failed");
	zassert_equal(conv_ctx.sample_rate_input, input_sample_rate,
		      "Input sample rate not as expected %d", conv_ctx.sample_rate_input);
	zassert_equal(conv_ctx.sample_rate_output, output_sample_rate,
		      "Output sample rate not as expected");
	zassert_equal(abs(conv_ctx.conversion_ratio), conversion_ratio,
		      "Conversion ratio not as expected");
	zassert_true(conv_ctx.conversion_ratio > 0, "Conversion direction is not as expected");
	zassert_equal(conv_ctx.filter_type, filter, "Filter set incorrectly");
	zassert_equal(conv_ctx.input_buf.bytes_in_buf, 4,
		      "Bytes in input buffer not as expected %d", conv_ctx.input_buf.bytes_in_buf);
	zassert_equal(ring_buf_size_get(&conv_ctx.output_ringbuf), 12,
		      "Number of bytes in output ringbuffer not as expected %d",
		      ring_buf_size_get(&conv_ctx.output_ringbuf));

	uint32_t expected_input_buf_two[] = {8000};

	zassert_mem_equal(conv_ctx.input_buf.buf, expected_input_buf_two, 1 * sizeof(uint32_t));

	zassert_equal(output_written, expected_output_samples * sizeof(uint32_t),
		      "Output size was not as expected (%d)", output_written);

	/* Verify output bytes, expected three samples in output per input */
	for (int i = 0; i < expected_output_samples; i++) {
		zassert_within(output_samples[i], compare_two[(i / 3)], 1,
			       "Output samples not within expected range from input samples");
	}

	/* Third run */
	ret = sample_rate_converter_process(
		&conv_ctx, filter, input_three, num_samples * sizeof(uint32_t), input_sample_rate,
		output_samples, expected_output_samples * sizeof(uint32_t), &output_written,
		output_sample_rate);

	zassert_equal(ret, 0, "Sample rate conversion process failed");
	zassert_equal(conv_ctx.sample_rate_input, input_sample_rate,
		      "Input sample rate not as expected %d", conv_ctx.sample_rate_input);
	zassert_equal(conv_ctx.sample_rate_output, output_sample_rate,
		      "Output sample rate not as expected");
	zassert_equal(abs(conv_ctx.conversion_ratio), conversion_ratio,
		      "Conversion ratio not as expected");
	zassert_true(conv_ctx.conversion_ratio > 0, "Conversion direction is not as expected");
	zassert_equal(conv_ctx.filter_type, filter, "Filter set incorrectly");
	zassert_equal(conv_ctx.input_buf.bytes_in_buf, 8,
		      "Bytes in input buffer not as expected %d", conv_ctx.input_buf.bytes_in_buf);
	zassert_equal(ring_buf_size_get(&conv_ctx.output_ringbuf), 0,
		      "Number of bytes in output ringbuffer not as expected %d",
		      ring_buf_size_get(&conv_ctx.output_ringbuf));

	uint32_t expected_input_buf_three[] = {11000, 12000};

	zassert_mem_equal(conv_ctx.input_buf.buf, expected_input_buf_three, 2 * sizeof(uint32_t));

	zassert_equal(output_written, expected_output_samples * sizeof(uint32_t),
		      "Output size was not as expected (%d)", output_written);

	/* Verify output bytes, expected three samples in output per input */
	for (int i = 0; i < expected_output_samples; i++) {
		zassert_within(output_samples[i], compare_three[(i / 3)], 1,
			       "Output samples not within expected range from input samples");
	}

	/* Fourth run */
	ret = sample_rate_converter_process(
		&conv_ctx, filter, input_four, num_samples * sizeof(uint32_t), input_sample_rate,
		output_samples, expected_output_samples * sizeof(uint32_t), &output_written,
		output_sample_rate);

	zassert_equal(ret, 0, "Sample rate conversion process failed");
	zassert_equal(conv_ctx.sample_rate_input, input_sample_rate,
		      "Input sample rate not as expected %d", conv_ctx.sample_rate_input);
	zassert_equal(conv_ctx.sample_rate_output, output_sample_rate,
		      "Output sample rate not as expected");
	zassert_equal(abs(conv_ctx.conversion_ratio), conversion_ratio,
		      "Conversion ratio not as expected");
	zassert_true(conv_ctx.conversion_ratio > 0, "Conversion direction is not as expected");
	zassert_equal(conv_ctx.filter_type, filter, "Filter set incorrectly");
	zassert_equal(conv_ctx.input_buf.bytes_in_buf, 0,
		      "Bytes in input buffer not as expected %d", conv_ctx.input_buf.bytes_in_buf);
	zassert_equal(ring_buf_size_get(&conv_ctx.output_ringbuf), 24,
		      "Number of bytes in output ringbuffer not as expected %d",
		      ring_buf_size_get(&conv_ctx.output_ringbuf));

	zassert_equal(output_written, expected_output_samples * sizeof(uint32_t),
		      "Output size was not as expected (%d)", output_written);

	/* Verify output bytes, expected three samples in output per input */
	for (int i = 0; i < expected_output_samples; i++) {
		zassert_within(output_samples[i], compare_four[(i / 3)], 1,
			       "Output samples not within expected range from input samples");
	}
}

/* Number of samples must be a define so the large array becomes a fixed size array that can be
 * initialized to all 0's
 */
#define BYTES_PRODUCED_TOO_LARGE_FOR_INTERNAL_BUF_NUM_SAMPLES 300
ZTEST(suite_sample_rate_converter, test_invalid_bytes_produced_too_large_for_internal_buf_32bit)
{
	int ret;

	uint32_t input_sample_rate = 24000;
	uint32_t output_sample_rate = 48000;
	uint32_t conversion_ratio = output_sample_rate / input_sample_rate;

	uint32_t input_samples[BYTES_PRODUCED_TOO_LARGE_FOR_INTERNAL_BUF_NUM_SAMPLES] = {0};
	size_t expected_output_samples =
		BYTES_PRODUCED_TOO_LARGE_FOR_INTERNAL_BUF_NUM_SAMPLES * conversion_ratio;
	uint32_t output_samples[expected_output_samples];

	enum sample_rate_converter_filter filter = SAMPLE_RATE_FILTER_TEST;
	size_t output_written;

	ret = sample_rate_converter_process(
		&conv_ctx, filter, input_samples,
		BYTES_PRODUCED_TOO_LARGE_FOR_INTERNAL_BUF_NUM_SAMPLES * sizeof(uint32_t),
		input_sample_rate, output_samples, expected_output_samples * sizeof(uint32_t),
		&output_written, output_sample_rate);

	zassert_equal(ret, -EINVAL,
		      "Sample rate conversion did not fail when the number of produced output "
		      "samples is larger than the internal output buf");
}
#endif /* CONFIG_SAMPLE_RATE_CONVERTER_BIT_DEPTH_32 */

ZTEST(suite_sample_rate_converter, test_init_valid_sample_rates_changed)
{
	int ret;

	enum sample_rate_converter_filter filter = SAMPLE_RATE_FILTER_TEST;

	uint32_t original_input_rate = 48000;
	uint32_t original_output_rate = 16000;

	uint32_t new_input_rate = 16000;
	uint32_t new_output_rate = 48000;
	uint32_t new_conversion_ratio = new_output_rate / new_input_rate;

	uint16_t input_samples[] = {1000, 2000, 3000, 4000,  5000,  6000,
				    7000, 8000, 9000, 10000, 11000, 12000};
	size_t num_samples = ARRAY_SIZE(input_samples);
	size_t expected_output_samples = num_samples * new_conversion_ratio;
	uint16_t output_bytes[expected_output_samples];
	size_t output_written;

	conv_ctx.sample_rate_input = original_input_rate;
	conv_ctx.sample_rate_output = original_output_rate;

	ret = sample_rate_converter_process(
		&conv_ctx, filter, input_samples, num_samples * sizeof(uint16_t), new_input_rate,
		output_bytes, expected_output_samples * sizeof(uint16_t), &output_written,
		new_output_rate);

	zassert_equal(ret, 0, "Sample rate conversion process failed");
	zassert_equal(conv_ctx.sample_rate_input, new_input_rate,
		      "Input sample rate not as expected");
	zassert_equal(conv_ctx.sample_rate_output, new_output_rate,
		      "Output sample rate not as expected");
}

ZTEST(suite_sample_rate_converter, test_init_invalid_sample_rates)
{
	int ret;

	uint16_t input_samples[] = {1000, 2000, 3000, 4000,  5000,  6000,
				    7000, 8000, 9000, 10000, 11000, 12000};
	size_t num_samples = ARRAY_SIZE(input_samples);
	size_t expected_output_samples = num_samples / 3;
	uint16_t output_samples[expected_output_samples];
	size_t output_written;

	uint32_t input_rate;
	uint32_t output_rate;

	/* Downsampling from NOT 48000 */
	input_rate = 24000;
	output_rate = 16000;

	ret = sample_rate_converter_process(
		&conv_ctx, SAMPLE_RATE_FILTER_TEST, input_samples, num_samples * sizeof(uint16_t),
		input_rate, output_samples, expected_output_samples, &output_written, output_rate);

	zassert_equal(ret, -EINVAL, "Sample rate conversion process did not fail");

	/* Downsampling to invalid rate */
	input_rate = 48000;
	output_rate = 20000;

	ret = sample_rate_converter_process(
		&conv_ctx, SAMPLE_RATE_FILTER_TEST, input_samples, num_samples * sizeof(uint16_t),
		input_rate, output_samples, expected_output_samples, &output_written, output_rate);

	zassert_equal(ret, -EINVAL, "Sample rate conversion process did not fail");

	/* Upsampling to NOT 48000 */
	input_rate = 16000;
	output_rate = 24000;

	ret = sample_rate_converter_process(
		&conv_ctx, SAMPLE_RATE_FILTER_TEST, input_samples, num_samples * sizeof(uint16_t),
		input_rate, output_samples, expected_output_samples, &output_written, output_rate);

	zassert_equal(ret, -EINVAL, "Sample rate conversion process did not fail");

	/* Upsampling to invalid rate */
	input_rate = 24000;
	output_rate = 30000;

	ret = sample_rate_converter_process(
		&conv_ctx, SAMPLE_RATE_FILTER_TEST, input_samples, num_samples * sizeof(uint16_t),
		input_rate, output_samples, expected_output_samples, &output_written, output_rate);

	zassert_equal(ret, -EINVAL, "Sample rate conversion process did not fail");
}

ZTEST(suite_sample_rate_converter, test_init_invalid_sample_rates_equal)
{
	int ret;

	uint16_t input_samples[] = {1000, 2000, 3000, 4000,  5000,  6000,
				    7000, 8000, 9000, 10000, 11000, 12000};
	size_t num_samples = ARRAY_SIZE(input_samples);
	size_t expected_output_samples = num_samples / 3;
	uint16_t output_samples[expected_output_samples];
	size_t output_written;

	uint32_t sample_rate = 48000;

	ret = sample_rate_converter_process(
		&conv_ctx, SAMPLE_RATE_FILTER_TEST, input_samples, num_samples * sizeof(uint16_t),
		sample_rate, output_samples, expected_output_samples, &output_written, sample_rate);

	zassert_equal(-EINVAL, ret,
		      "Process did not fail when input and out sample rate is the same");
}

ZTEST(suite_sample_rate_converter, test_init_valid_filter_changed)
{
	int ret;

	uint32_t input_sample_rate = 48000;
	uint32_t output_sample_rate = 24000;
	uint32_t conversion_ratio = input_sample_rate / output_sample_rate;

	uint16_t input_samples[] = {1000, 2000, 3000, 4000,  5000,  6000,
				    7000, 8000, 9000, 10000, 11000, 12000};
	size_t num_samples = ARRAY_SIZE(input_samples);
	size_t expected_output_samples = num_samples / conversion_ratio;
	uint16_t output_samples[expected_output_samples];
	size_t output_written;

	enum sample_rate_converter_filter original_filter = SAMPLE_RATE_FILTER_SIMPLE;
	enum sample_rate_converter_filter new_filter = SAMPLE_RATE_FILTER_TEST;

	conv_ctx.sample_rate_input = input_sample_rate;
	conv_ctx.sample_rate_output = output_sample_rate;
	conv_ctx.filter_type = original_filter;

	ret = sample_rate_converter_process(
		&conv_ctx, new_filter, input_samples, num_samples * sizeof(uint16_t),
		input_sample_rate, output_samples, expected_output_samples * sizeof(uint16_t),
		&output_written, output_sample_rate);

	zassert_equal(ret, 0, "Sample rate conversion process failed");
	zassert_equal(conv_ctx.sample_rate_input, input_sample_rate,
		      "Input sample rate not as expected");
	zassert_equal(conv_ctx.sample_rate_output, output_sample_rate,
		      "Output sample rate not as expected");
	zassert_equal(conv_ctx.filter_type, new_filter, "Filter set incorrectly");
}

ZTEST(suite_sample_rate_converter, test_invalid_process_ctx_null_ptr)
{
	int ret;

	uint16_t input_samples[] = {1000, 2000, 3000, 4000};
	size_t num_samples = ARRAY_SIZE(input_samples);
	size_t expected_output_samples = num_samples / 3;
	uint16_t output_samples[expected_output_samples];
	size_t output_written;

	uint32_t input_sample_rate = 48000;
	uint32_t output_sample_rate = 24000;
	enum sample_rate_converter_filter filter = SAMPLE_RATE_FILTER_TEST;

	ret = sample_rate_converter_process(
		NULL, filter, input_samples, num_samples * sizeof(uint16_t), input_sample_rate,
		output_samples, expected_output_samples, &output_written, output_sample_rate);

	zassert_equal(ret, -EINVAL, "Sample rate conversion process did not fail");
}

ZTEST(suite_sample_rate_converter, test_invalid_process_buffer_null_ptr)
{
	int ret;

	uint16_t input_samples[] = {1000, 2000, 3000, 4000};
	size_t num_samples = ARRAY_SIZE(input_samples);
	size_t expected_output_samples = num_samples / 3;
	uint16_t output_samples[expected_output_samples];
	size_t output_written;

	uint32_t input_sample_rate = 48000;
	uint32_t output_sample_rate = 24000;
	enum sample_rate_converter_filter filter = SAMPLE_RATE_FILTER_TEST;

	ret = sample_rate_converter_process(
		&conv_ctx, filter, NULL, num_samples * sizeof(uint16_t), input_sample_rate,
		output_samples, expected_output_samples, &output_written, output_sample_rate);

	zassert_equal(ret, -EINVAL,
		      "Sample rate conversion process did not fail when input pointer is NULL");

	ret = sample_rate_converter_process(&conv_ctx, filter, NULL, num_samples * sizeof(uint16_t),
					    input_sample_rate, NULL, expected_output_samples,
					    &output_written, output_sample_rate);

	zassert_equal(ret, -EINVAL,
		      "Sample rate conversion process did not fail when output pointer is NULL");
}

ZTEST(suite_sample_rate_converter, test_invalid_process_output_written_null_ptr)
{
	int ret;

	uint16_t input_samples[] = {1000, 2000, 3000, 4000};
	size_t num_samples = ARRAY_SIZE(input_samples);
	size_t expected_output_samples = num_samples / 3;
	uint16_t output_samples[expected_output_samples];

	uint32_t input_sample_rate = 48000;
	uint32_t output_sample_rate = 24000;
	enum sample_rate_converter_filter filter = SAMPLE_RATE_FILTER_TEST;

	ret = sample_rate_converter_process(
		&conv_ctx, filter, input_samples, num_samples * sizeof(uint16_t), input_sample_rate,
		output_samples, expected_output_samples, NULL, output_sample_rate);

	zassert_equal(
		ret, -EINVAL,
		"Sample rate conversion process did not fail when output size pouinter is NULL");
}

ZTEST(suite_sample_rate_converter, test_invalid_open_null_ptr)
{
	int ret;

	ret = sample_rate_converter_open(NULL);

	zassert_equal(ret, -EINVAL, "Call to open did not fail");
}

ZTEST(suite_sample_rate_converter, test_valid_process_zero_size_input)
{
	int ret;

	uint16_t input_samples[] = {1000, 2000, 3000, 4000};
	size_t num_samples = ARRAY_SIZE(input_samples);
	size_t expected_output_samples = num_samples * 2;
	uint16_t output_samples[expected_output_samples];
	size_t output_written;

	uint32_t input_sample_rate = 24000;
	uint32_t output_sample_rate = 48000;
	uint32_t conversion_ratio = output_sample_rate / input_sample_rate;
	enum sample_rate_converter_filter filter = SAMPLE_RATE_FILTER_TEST;

	ret = sample_rate_converter_process(&conv_ctx, filter, input_samples, 0, input_sample_rate,
					    output_samples, expected_output_samples,
					    &output_written, output_sample_rate);

	zassert_equal(ret, 0, "Sample rate conversion process failed");
	zassert_equal(conv_ctx.sample_rate_input, input_sample_rate,
		      "Input sample rate not as expected");
	zassert_equal(conv_ctx.sample_rate_output, output_sample_rate,
		      "Output sample rate not as expected");
	zassert_equal(abs(conv_ctx.conversion_ratio), conversion_ratio,
		      "Conversion ratio not as expected");
	zassert_true(conv_ctx.conversion_ratio > 0, "Conversion direction is not as expected");
	zassert_equal(conv_ctx.filter_type, filter, "Filter not as expected");
	zassert_equal(conv_ctx.input_buf.bytes_in_buf, 0, "Bytes in input buffer not as expected");
	zassert_equal(ring_buf_size_get(&conv_ctx.output_ringbuf), 0,
		      "Number of bytes in output ringbuffer not as expected %d",
		      ring_buf_size_get(&conv_ctx.output_ringbuf));

	zassert_equal(output_written, 0, "Received %d output samples when none was expected",
		      output_written);
}

ZTEST(suite_sample_rate_converter, test_invalid_process_input_samples_less_than_ratio)
{
	int ret;

	uint32_t input_sample_rate = 48000;
	uint32_t output_sample_rate = 24000;
	uint32_t conversion_ratio = input_sample_rate / output_sample_rate;
	enum sample_rate_converter_filter filter = SAMPLE_RATE_FILTER_TEST;

	uint16_t input_samples[] = {1000};
	size_t num_samples = ARRAY_SIZE(input_samples);
	size_t expected_output_samples = num_samples / conversion_ratio;
	uint16_t output_samples[expected_output_samples];
	size_t output_written;

	ret = sample_rate_converter_process(
		&conv_ctx, filter, input_samples, num_samples * sizeof(uint16_t), input_sample_rate,
		output_samples, expected_output_samples, &output_written, output_sample_rate);

	zassert_equal(ret, -EINVAL, "Sample rate conversion process failed");
}

ZTEST(suite_sample_rate_converter, test_invalid_process_input_not_multiple)
{
	int ret;

	uint32_t input_sample_rate = 48000;
	uint32_t output_sample_rate = 24000;
	enum sample_rate_converter_filter filter = SAMPLE_RATE_FILTER_TEST;

	uint8_t input_samples[] = {1, 2, 3, 4, 5};
	size_t num_bytes = ARRAY_SIZE(input_samples);
	size_t expected_output_samples = 1;
	uint16_t output_samples[expected_output_samples];
	size_t output_written;

	ret = sample_rate_converter_process(
		&conv_ctx, filter, input_samples, num_bytes, input_sample_rate, output_samples,
		expected_output_samples, &output_written, output_sample_rate);

	zassert_equal(ret, -EINVAL,
		      "Sample rate conversion process did not fail when number of input is not a "
		      "16- or 32-bit multiple");
}

/* Number of samples must be a define so the large array becomes a fixed size array that can be
 * initialized to all 0's
 */
#define INPUT_ARRAY_TOO_LARGE_NUM_SAMPLES 500
ZTEST(suite_sample_rate_converter, test_invalid_process_input_array_too_large)
{
	int ret;

	uint32_t input_sample_rate = 48000;
	uint32_t output_sample_rate = 24000;
	uint32_t conversion_ratio = input_sample_rate / output_sample_rate;

	uint16_t input_samples[INPUT_ARRAY_TOO_LARGE_NUM_SAMPLES] = {0};
	size_t expected_output_samples = INPUT_ARRAY_TOO_LARGE_NUM_SAMPLES / conversion_ratio;
	uint16_t output_samples[expected_output_samples];

	enum sample_rate_converter_filter filter = SAMPLE_RATE_FILTER_TEST;
	size_t output_written;

	ret = sample_rate_converter_process(
		&conv_ctx, filter, input_samples,
		INPUT_ARRAY_TOO_LARGE_NUM_SAMPLES * sizeof(uint16_t), input_sample_rate,
		output_samples, expected_output_samples, &output_written, output_sample_rate);

	zassert_equal(
		ret, -EINVAL,
		"Sample rate conversion did not fail when number of input samples is too large");
}

ZTEST(suite_sample_rate_converter, test_invalid_process_output_buf_too_small)
{
	int ret;

	uint32_t input_sample_rate = 48000;
	uint32_t output_sample_rate = 24000;
	uint32_t conversion_ratio = input_sample_rate / output_sample_rate;

	uint16_t input_samples[] = {1000, 2000, 3000, 4000,  5000,  6000,
				    7000, 8000, 9000, 10000, 11000, 12000};
	size_t num_samples = ARRAY_SIZE(input_samples);
	size_t expected_output_samples = num_samples / conversion_ratio;
	/* expected_output_samples is 6, so this is set to 5 */
	uint16_t output_samples[5];

	enum sample_rate_converter_filter filter = SAMPLE_RATE_FILTER_TEST;
	size_t output_written;

	ret = sample_rate_converter_process(
		&conv_ctx, filter, input_samples, num_samples * sizeof(uint16_t), input_sample_rate,
		output_samples, expected_output_samples, &output_written, output_sample_rate);

	zassert_equal(ret, -EINVAL,
		      "Sample rate conversion process did not fail when output buffer is to small");
}

ZTEST_SUITE(suite_sample_rate_converter, NULL, NULL, test_setup, NULL, NULL);
