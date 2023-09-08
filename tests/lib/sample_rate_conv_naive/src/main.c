/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <errno.h>
#include <zephyr/tc_util.h>
#include "sample_rate_conv_naive.h"

static uint8_t input[] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11,
			  12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23};

static uint8_t input_short[] = {0, 1, 2, 3};

ZTEST(suite_sample_rate_conv_naive, test_one_to_one)
{
	int ret;
	uint8_t output[16];
	size_t output_size;

	ret = sample_rate_conv_naive(input, sizeof(input), 48000, output, &output_size, 48000, 8);

	zassert_equal(ret, 0, "Wrong ret code returned");
	zassert_equal(output_size, sizeof(input));
	zassert_mem_equal(input, output, sizeof(input));

	ret = sample_rate_conv_naive(input, sizeof(input), 48000, output, &output_size, 48000, 16);

	zassert_equal(ret, 0, "Wrong ret code returned");
	zassert_equal(output_size, sizeof(input));
	zassert_mem_equal(input, output, sizeof(input));

	ret = sample_rate_conv_naive(input, sizeof(input), 48000, output, &output_size, 48000, 24);

	zassert_equal(ret, 0, "Wrong ret code returned");
	zassert_equal(output_size, sizeof(input));
	zassert_mem_equal(input, output, sizeof(input));
}

ZTEST(suite_sample_rate_conv_naive, test_downsample_48_to_24_8bit)
{
	int ret;
	uint8_t output[16];
	uint8_t expected_output[] = {0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22};
	size_t output_size;

	ret = sample_rate_conv_naive(input, sizeof(input), 48000, output, &output_size, 24000, 8);
	zassert_equal(ret, 0, "Wrong ret code returned");
	zassert_equal(output_size, sizeof(expected_output));
	zassert_mem_equal(output, expected_output, sizeof(expected_output));
}

ZTEST(suite_sample_rate_conv_naive, test_downsample_48_to_24_16bit)
{
	int ret;
	uint8_t output[16];
	uint8_t expected_output[] = {0, 1, 4, 5, 8, 9, 12, 13, 16, 17, 20, 21};
	size_t output_size;

	ret = sample_rate_conv_naive(input, sizeof(input), 48000, output, &output_size, 24000, 16);
	zassert_equal(ret, 0, "Wrong ret code returned");
	zassert_equal(output_size, sizeof(expected_output));
	zassert_mem_equal(output, expected_output, sizeof(expected_output));
}

ZTEST(suite_sample_rate_conv_naive, test_downsample_48_to_16_8bit)
{
	int ret;
	uint8_t output[16];
	uint8_t expected_output[] = {0, 3, 6, 9, 12, 15, 18, 21};
	size_t output_size;

	ret = sample_rate_conv_naive(input, sizeof(input), 48000, output, &output_size, 16000, 8);
	zassert_equal(ret, 0, "Wrong ret code returned");
	zassert_equal(output_size, sizeof(expected_output));
	zassert_mem_equal(output, expected_output, sizeof(expected_output));
}

ZTEST(suite_sample_rate_conv_naive, test_upsample_16_to_48_8bit)
{
	int ret;
	uint8_t output[16];
	uint8_t expected_output[] = {0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3};
	size_t output_size;

	ret = sample_rate_conv_naive(input_short, sizeof(input_short), 16000, output, &output_size,
				     48000, 8);
	zassert_equal(ret, 0, "Wrong ret code returned");
	zassert_equal(output_size, sizeof(expected_output));
	zassert_mem_equal(output, expected_output, sizeof(expected_output));
}

ZTEST(suite_sample_rate_conv_naive, test_upsample_24_to_48_8bit)
{
	int ret;
	uint8_t output[16];
	uint8_t expected_output[] = {0, 0, 1, 1, 2, 2, 3, 3};
	size_t output_size;

	ret = sample_rate_conv_naive(input_short, sizeof(input_short), 24000, output, &output_size,
				     48000, 8);
	zassert_equal(ret, 0, "Wrong ret code returned");
	zassert_equal(output_size, sizeof(expected_output));
	zassert_mem_equal(output, expected_output, sizeof(expected_output));
}

ZTEST(suite_sample_rate_conv_naive, test_upsample_24_to_48_16bit)
{
	int ret;
	uint8_t output[16];
	uint8_t expected_output[] = {0, 1, 0, 1, 2, 3, 2, 3};
	size_t output_size;

	ret = sample_rate_conv_naive(input_short, sizeof(input_short), 24000, output, &output_size,
				     48000, 16);
	zassert_equal(ret, 0, "Wrong ret code returned");
	zassert_equal(output_size, sizeof(expected_output));
	zassert_mem_equal(output, expected_output, sizeof(expected_output));
}

ZTEST(suite_sample_rate_conv_naive, test_invalid_arguments)
{
	int ret;
	uint8_t output[16];
	size_t output_size;

	/* Invalid bit depth */
	ret = sample_rate_conv_naive(input_short, sizeof(input_short), 24000, output, &output_size,
				     48000, 17);
	zassert_equal(ret, -EINVAL, "Wrong ret code returned");

	ret = sample_rate_conv_naive(input_short, sizeof(input_short), 24001, output, &output_size,
				     48000, 16);
	zassert_equal(ret, -EINVAL, "Wrong ret code returned");

	ret = sample_rate_conv_naive(input_short, sizeof(input_short), 24000, output, &output_size,
				     48001, 16);
	zassert_equal(ret, -EINVAL, "Wrong ret code returned");

	ret = sample_rate_conv_naive(input_short, sizeof(input_short), 16000, output, &output_size,
				     24000, 16);
	zassert_equal(ret, -EINVAL, "Wrong ret code returned");

	ret = sample_rate_conv_naive(input_short, sizeof(input_short), 24000, output, &output_size,
				     16000, 16);
	zassert_equal(ret, -EINVAL, "Wrong ret code returned");
}

ZTEST_SUITE(suite_sample_rate_conv_naive, NULL, NULL, NULL, NULL, NULL);
