/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <errno.h>
#include <zephyr/tc_util.h>
#include <tone.h>

static int32_t tone_sum_8(int8_t *tone, size_t size)
{
	int32_t sum = 0;

	for (size_t i = 0; i < size / sizeof(int8_t); i++) {
		sum += tone[i];
	}

	return sum;
}

static int32_t tone_sum_16(int16_t *tone, size_t size)
{
	int32_t sum = 0;

	for (size_t i = 0; i < size / sizeof(int16_t); i++) {
		sum += tone[i];
	}

	return sum;
}

static int64_t tone_sum_32(int32_t *tone, size_t size)
{
	int64_t sum = 0;

	for (size_t i = 0; i < size / sizeof(int32_t); i++) {
		sum += tone[i];
	}

	return sum;
}

static void tone_high_low_idx_8(int8_t *tone, size_t size, uint32_t *idx_low, uint32_t *idx_high)
{
	int8_t highest = 0;
	int8_t lowest = 0;

	for (size_t i = 0; i < size / sizeof(int8_t); i++) {
		if (tone[i] < lowest) {
			lowest = tone[i];
			*idx_low = i;
		}
		if (tone[i] > highest) {
			highest = tone[i];
			*idx_high = i;
		}
	}
}

static void tone_high_low_idx_16(int16_t *tone, size_t size, uint32_t *idx_low, uint32_t *idx_high)
{
	int16_t highest = 0;
	int16_t lowest = 0;

	for (size_t i = 0; i < size / sizeof(int16_t); i++) {
		if (tone[i] < lowest) {
			lowest = tone[i];
			*idx_low = i;
		}
		if (tone[i] > highest) {
			highest = tone[i];
			*idx_high = i;
		}
	}
}

static void tone_high_low_idx_32(int32_t *tone, size_t size, uint32_t *idx_low, uint32_t *idx_high)
{
	int32_t highest = 0;
	int32_t lowest = 0;

	for (size_t i = 0; i < size / sizeof(int32_t); i++) {
		if (tone[i] < lowest) {
			lowest = tone[i];
			*idx_low = i;
		}
		if (tone[i] > highest) {
			highest = tone[i];
			*idx_high = i;
		}
	}
}

/* Get the index of the lowest and highest sample */
ZTEST(suite_tone, test_tone_gen_valid)
{
#define NUM_TESTS 3
	uint16_t freq[] = {100, 480, 960};
	uint32_t smpl_freq[] = {10000, 48000, 48000};
	size_t tone_size_desired[] = {200, 200, 100};
	float amplitude[] = {1, 1, 1};

	for (uint8_t i = 0; i < NUM_TESTS; i++) {
		int16_t tone[400] = {0};
		size_t tone_size = 0;
		uint32_t idx_low = 0;
		uint32_t idx_high = 0;

		zassert_equal(tone_gen(tone, &tone_size, freq[i], smpl_freq[i], amplitude[i]), 0,
			      "Err code returned");
		zassert_equal(tone_size, tone_size_desired[i], "Incorrect tone size");

		/* Since a single period is generated, the center sample should always be zero */
		zassert_equal(tone[tone_size / 2], 0, "Center sample not zero");
		zassert_equal(tone_sum_16(tone, tone_size), 0, "The sum of samples are not zero");

		tone_high_low_idx_16(tone, tone_size, &idx_low, &idx_high);
		zassert_equal(idx_low, 3 * (tone_size / 2) / 4,
			      "Lowest sample not at the 3/4 mark");
		zassert_equal(idx_high, (tone_size / 2) / 4, "Highest sample not at the 1/4 mark");
	}
}

ZTEST(suite_tone, test_illegal_args)
{
	int16_t tone[200] = {0};
	size_t tone_size;
	uint16_t freq = 100;
	uint32_t smpl_freq = 10000;
	float amplitude = 1;

	/* NULL ptr */
	zassert_equal(tone_gen(NULL, &tone_size, freq, smpl_freq, amplitude), -ENXIO,
		      "Wrong code returned");
	/* NULL ptr */
	zassert_equal(tone_gen(tone, NULL, freq, smpl_freq, amplitude), -ENXIO,
		      "Wrong code returned");
	/* 0 freq */
	zassert_equal(tone_gen(tone, &tone_size, 0, smpl_freq, amplitude), -EINVAL,
		      "Wrong code returned");
	/* freq too low */
	zassert_equal(tone_gen(tone, &tone_size, 10, smpl_freq, amplitude), -EINVAL,
		      "Wrong code returned");
	/* freq too high */
	zassert_equal(tone_gen(tone, &tone_size, 10001, smpl_freq, amplitude), -EINVAL,
		      "Wrong code returned");
	/* smpl_freq 0 */
	zassert_equal(tone_gen(tone, &tone_size, freq, 0, amplitude), -EINVAL, "Err code returned");
	/* 0 Amplitude */
	zassert_equal(tone_gen(tone, &tone_size, freq, smpl_freq, 0), -EPERM, "Err code returned");
	/* Amplitude too high*/
	zassert_equal(tone_gen(tone, &tone_size, freq, smpl_freq, 1.1), -EPERM,
		      "Err code returned");
}

ZTEST(suite_tone_gen_size, test_tone_gen_size_8_valid)
{
	uint16_t freq[] = {100, 480, 960};
	uint32_t sample_freq[] = {10000, 48000, 48000};
	float amplitude = 1;
	uint8_t sample_width = 8;
	uint8_t carrier_bits = 8;
	uint32_t idx_low = 0;
	uint32_t idx_high = 0;

	for (size_t i = 0; i < ARRAY_SIZE(freq); i++) {
		printk("Testing sample width %d bits\n", sample_width);

		uint8_t tone[100];
		size_t tone_size = 0;
		size_t tone_size_desired = (sample_freq[i] / freq[i]) * (carrier_bits / 8);

		memset(tone, 0, sizeof(tone));

		zassert_equal(tone_gen_size(tone, &tone_size, freq[i], sample_freq[i], sample_width,
					    carrier_bits, amplitude),
			      0, "Err code returned");
		zassert_equal(tone_size, tone_size_desired, "Incorrect tone size");

		/* Since a single period is generated, the center sample should always be
		 * zero
		 */
		zassert_equal(tone[tone_size / (2 * sizeof(int8_t))], 0, "Center sample not zero");
		zassert_equal(tone_sum_8(tone, tone_size), 0, "The sum of samples are not zero");

		tone_high_low_idx_8(tone, tone_size, &idx_low, &idx_high);
		zassert_equal(idx_low, 3 * tone_size / 4, "Lowest sample not at the 3/4 mark");
		zassert_equal(idx_high, (tone_size / sizeof(int8_t)) / 4,
			      "Highest sample not at the 1/4 mark");
	}
}

ZTEST(suite_tone_gen_size, test_tone_gen_size_16_valid)
{
	uint16_t freq[] = {100, 480, 960};
	uint32_t sample_freq[] = {10000, 48000, 48000};
	uint8_t sample_width[] = {8, 16};
	float amplitude = 1;
	uint8_t carrier_bits = 16;
	uint32_t idx_low = 0;
	uint32_t idx_high = 0;

	for (size_t j = 0; j < ARRAY_SIZE(sample_width); j++) {
		printk("Testing sample width %d bits\n", sample_width[j]);

		for (size_t i = 0; i < ARRAY_SIZE(freq); i++) {
			uint16_t tone[100];
			size_t tone_size = 0;
			size_t tone_size_desired = (sample_freq[i] / freq[i]) * (carrier_bits / 8);

			memset(tone, 0, sizeof(tone));

			zassert_equal(tone_gen_size(tone, &tone_size, freq[i], sample_freq[i],
						    sample_width[j], carrier_bits, amplitude),
				      0, "Err code returned");
			zassert_equal(tone_size, tone_size_desired, "Incorrect tone size");

			/* Since a single period is generated, the center sample should
			 * always be zero
			 */
			zassert_equal(tone[tone_size / (2 * sizeof(int16_t))], 0,
				      "Center sample not zero");
			zassert_equal(tone_sum_16(tone, tone_size), 0,
				      "The sum of samples are not zero");

			tone_high_low_idx_16(tone, tone_size, &idx_low, &idx_high);
			zassert_equal(idx_low, 3 * (tone_size / 2) / 4,
				      "Lowest sample not at the 3/4 mark");
			zassert_equal(idx_high, (tone_size / sizeof(int16_t)) / 4,
				      "Highest sample not at the 1/4 mark");
		}
	}
}

ZTEST(suite_tone_gen_size, test_tone_gen_size_32_valid)
{
	uint16_t freq[] = {100, 480, 960};
	uint32_t sample_freq[] = {24000, 48000, 48000};
	uint8_t sample_width[] = {8, 16, 24, 32};
	uint8_t carrier_bits = 32;
	float amplitude = 1;
	uint32_t idx_low = 0;
	uint32_t idx_high = 0;

	for (size_t j = 0; j < ARRAY_SIZE(sample_width); j++) {
		printk("Testing sample width %d bits\n", sample_width[j]);

		for (size_t i = 0; i < ARRAY_SIZE(freq); i++) {
			uint32_t tone[300];
			size_t tone_size = 0;
			size_t tone_size_desired = (sample_freq[i] / freq[i]) * (carrier_bits / 8);

			memset(tone, 0, sizeof(tone));

			zassert_equal(tone_gen_size(tone, &tone_size, freq[i], sample_freq[i],
						    sample_width[j], carrier_bits, amplitude),
				      0, "Err code returned");
			zassert_equal(tone_size, tone_size_desired, "Incorrect tone size: %d (%d)",
				      tone_size, tone_size_desired);

			/* Since a single period is generated, the center sample should always be
			 * zero
			 */
			zassert_equal(tone[tone_size / (2 * sizeof(int32_t))], 0,
				      "Center sample not zero");
			zassert_equal(tone_sum_32(tone, tone_size), (int64_t)0,
				      "The sum of samples are not zero");

			tone_high_low_idx_32(tone, tone_size, &idx_low, &idx_high);
			zassert_equal(idx_low, 3 * (tone_size / sizeof(int32_t)) / 4,
				      "Lowest sample not at the 3/4 mark");
			zassert_equal(idx_high, (tone_size / sizeof(int32_t)) / 4,
				      "Highest sample not at the 1/4 mark");
		}
	}
}

ZTEST(suite_tone_gen_size, test_tone_gen_size_illegal_args)
{
	int16_t tone[200] = {0};
	size_t tone_size;
	uint16_t freq = 100;
	uint32_t smpl_freq = 10000;
	uint8_t sample_bits = 16;
	uint8_t carrier_bits = 32;
	float amplitude = 1;

	/* NULL ptr */
	zassert_equal(tone_gen_size(NULL, &tone_size, freq, smpl_freq, sample_bits, carrier_bits,
				    amplitude),
		      -ENXIO, "Wrong code returned");
	/* NULL ptr */
	zassert_equal(
		tone_gen_size(tone, NULL, freq, smpl_freq, sample_bits, carrier_bits, amplitude),
		-ENXIO, "Wrong code returned");
	/* 0 freq */
	zassert_equal(
		tone_gen_size(tone, &tone_size, 0, smpl_freq, sample_bits, carrier_bits, amplitude),
		-EINVAL, "Wrong code returned");
	/* freq too low */
	zassert_equal(tone_gen_size(tone, &tone_size, 10, smpl_freq, sample_bits, carrier_bits,
				    amplitude),
		      -EINVAL, "Wrong code returned");
	/* freq too high */
	zassert_equal(tone_gen_size(tone, &tone_size, 10001, smpl_freq, sample_bits, carrier_bits,
				    amplitude),
		      -EINVAL, "Wrong code returned");
	/* sampling freq not devisable by the tone freq */
	zassert_equal(tone_gen_size(tone, &tone_size, 300, smpl_freq, sample_bits, carrier_bits,
				    amplitude),
		      -EINVAL, "Wrong code returned");
	/* smpl_freq 0 */
	zassert_equal(
		tone_gen_size(tone, &tone_size, freq, 0, sample_bits, carrier_bits, amplitude),
		-EINVAL, "Err code returned");
	/* sample_bits 0 */
	zassert_equal(tone_gen_size(tone, &tone_size, freq, smpl_freq, 0, carrier_bits, amplitude),
		      -EINVAL, "Err code returned");
	/* sample_bits not in set */
	zassert_equal(tone_gen_size(tone, &tone_size, freq, smpl_freq, 12, carrier_bits, amplitude),
		      -EINVAL, "Err code returned");
	/* carrier_bits 0 */
	zassert_equal(tone_gen_size(tone, &tone_size, freq, smpl_freq, sample_bits, 0, amplitude),
		      -EINVAL, "Err code returned");
	/* carrier_bits too small */
	zassert_equal(tone_gen_size(tone, &tone_size, freq, smpl_freq, sample_bits, 8, amplitude),
		      -EINVAL, "Err code returned");
	/* carrier_bits not in set */
	zassert_equal(tone_gen_size(tone, &tone_size, freq, smpl_freq, sample_bits, 26, amplitude),
		      -EINVAL, "Err code returned");
	/* 0 Amplitude */
	zassert_equal(
		tone_gen_size(tone, &tone_size, freq, smpl_freq, sample_bits, carrier_bits, 0),
		-EPERM, "Err code returned");
	/* Amplitude too high*/
	zassert_equal(
		tone_gen_size(tone, &tone_size, freq, smpl_freq, sample_bits, carrier_bits, 1.1),
		-EPERM, "Err code returned");
}

ZTEST_SUITE(suite_tone, NULL, NULL, NULL, NULL, NULL);
ZTEST_SUITE(suite_tone_gen_size, NULL, NULL, NULL, NULL, NULL);
