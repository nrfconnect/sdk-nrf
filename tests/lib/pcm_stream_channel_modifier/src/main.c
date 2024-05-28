/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <errno.h>
#include <audio_defines.h>
#include <pcm_stream_channel_modifier.h>

#define ZEQ(a, b) zassert_equal(b, a, "fail")

void verify_array_eq(void *p1, void *p2, size_t size)
{
	ZEQ(memcmp(p1, p2, size), 0);
}

/* Input arrays */
uint8_t unpadded_left[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
uint8_t unpadded_right[] = { 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24 };
uint8_t stereo_split[] = { 1, 13, 2, 14, 3, 15, 4,  16, 5,  17, 6,  18,
			   7, 19, 8, 20, 9, 21, 10, 22, 11, 23, 12, 24 };

/* Result arrays */
uint8_t left_zero_padded_16[] = { 1, 2, 0, 0, 3, 4,  0, 0, 5,  6,  0, 0,
				  7, 8, 0, 0, 9, 10, 0, 0, 11, 12, 0, 0 };
uint8_t right_zero_padded_16[] = { 0, 0, 1, 2, 0, 0, 3, 4,  0, 0, 5,  6,
				   0, 0, 7, 8, 0, 0, 9, 10, 0, 0, 11, 12 };
uint8_t copy_padded_16[] = { 1, 2, 1, 2, 3, 4,	3, 4,  5,  6,  5,  6,
			     7, 8, 7, 8, 9, 10, 9, 10, 11, 12, 11, 12 };
uint8_t combine_16[] = { 1, 2, 13, 14, 3, 4,  15, 16, 5,  6,  17, 18,
			 7, 8, 19, 20, 9, 10, 21, 22, 11, 12, 23, 24 };
uint8_t stereo_split_left_16[] = { 1, 13, 3, 15, 5, 17, 7, 19, 9, 21, 11, 23 };
uint8_t stereo_split_right_16[] = { 2, 14, 4, 16, 6, 18, 8, 20, 10, 22, 12, 24 };

uint8_t left_zero_padded_24[] = { 1, 2, 3, 0, 0, 0, 4,	5,  6,	0, 0, 0,
				  7, 8, 9, 0, 0, 0, 10, 11, 12, 0, 0, 0 };
uint8_t right_zero_padded_24[] = { 0, 0, 0, 1, 2, 3, 0, 0, 0, 4,  5,  6,
				   0, 0, 0, 7, 8, 9, 0, 0, 0, 10, 11, 12 };
uint8_t copy_padded_24[] = { 1, 2, 3, 1, 2, 3, 4,  5,  6,  4,  5,  6,
			     7, 8, 9, 7, 8, 9, 10, 11, 12, 10, 11, 12 };
uint8_t combine_24[] = { 1, 2, 3, 13, 14, 15, 4,  5,  6,  16, 17, 18,
			 7, 8, 9, 19, 20, 21, 10, 11, 12, 22, 23, 24 };
uint8_t stereo_split_left_24[] = { 1, 13, 2, 4, 16, 5, 7, 19, 8, 10, 22, 11 };
uint8_t stereo_split_right_24[] = { 14, 3, 15, 17, 6, 18, 20, 9, 21, 23, 12, 24 };

uint8_t left_zero_padded_32[] = { 1, 2, 3, 4, 0, 0,  0,	 0,  5, 6, 7, 8,
				  0, 0, 0, 0, 9, 10, 11, 12, 0, 0, 0, 0 };
uint8_t right_zero_padded_32[] = { 0, 0, 0, 0, 1, 2, 3, 4, 0, 0,  0,  0,
				   5, 6, 7, 8, 0, 0, 0, 0, 9, 10, 11, 12 };
uint8_t copy_padded_32[] = { 1, 2, 3, 4, 1, 2,	3,  4,	5, 6,  7,  8,
			     5, 6, 7, 8, 9, 10, 11, 12, 9, 10, 11, 12 };
uint8_t combine_32[] = { 1,  2,	 3,  4,	 13, 14, 15, 16, 5,  6,	 7,  8,
			 17, 18, 19, 20, 9,  10, 11, 12, 21, 22, 23, 24 };
uint8_t stereo_split_left_32[] = { 1, 13, 2, 14, 5, 17, 6, 18, 9, 21, 10, 22 };
uint8_t stereo_split_right_32[] = { 3, 15, 4, 16, 7, 19, 8, 20, 11, 23, 12, 24 };

ZTEST(suite_pscm, test_pscm_zero_pad_16)
{
	uint16_t left_test_list[50];
	uint16_t right_test_list[50];
	size_t output_size;
	int ret;

	ret = pscm_zero_pad(unpadded_left, sizeof(unpadded_left), AUDIO_CH_L, 16, left_test_list,
			    &output_size);
	ZEQ(ret, 0);
	ZEQ(output_size, 2 * sizeof(unpadded_left));
	verify_array_eq(left_test_list, left_zero_padded_16, output_size);

	ret = pscm_zero_pad(unpadded_left, sizeof(unpadded_left), AUDIO_CH_R, 16, right_test_list,
			    &output_size);
	ZEQ(ret, 0);
	ZEQ(output_size, 2 * sizeof(unpadded_left));
	verify_array_eq(right_test_list, right_zero_padded_16, output_size);
}

ZTEST(suite_pscm, test_pscm_zero_pad_24)
{
	uint8_t left_test_list[50];
	uint8_t right_test_list[50];
	size_t output_size;
	int ret;

	ret = pscm_zero_pad(unpadded_left, sizeof(unpadded_left), AUDIO_CH_L, 24, left_test_list,
			    &output_size);
	ZEQ(ret, 0);
	ZEQ(output_size, 2 * sizeof(unpadded_left));
	verify_array_eq(left_test_list, left_zero_padded_24, output_size);

	ret = pscm_zero_pad(unpadded_left, sizeof(unpadded_left), AUDIO_CH_R, 24, right_test_list,
			    &output_size);
	ZEQ(ret, 0);
	ZEQ(output_size, 2 * sizeof(unpadded_left));
	verify_array_eq(right_test_list, right_zero_padded_24, output_size);
}

ZTEST(suite_pscm, test_pscm_zero_pad_32)
{
	uint32_t left_test_list[50];
	uint32_t right_test_list[50];
	size_t output_size;
	int ret;

	ret = pscm_zero_pad(unpadded_left, sizeof(unpadded_left), AUDIO_CH_L, 32, left_test_list,
			    &output_size);
	ZEQ(ret, 0);
	ZEQ(output_size, 2 * sizeof(unpadded_left));
	verify_array_eq(left_test_list, left_zero_padded_32, output_size);

	ret = pscm_zero_pad(unpadded_left, sizeof(unpadded_left), AUDIO_CH_R, 32, right_test_list,
			    &output_size);
	ZEQ(ret, 0);
	ZEQ(output_size, 2 * sizeof(unpadded_left));
	verify_array_eq(right_test_list, right_zero_padded_32, output_size);
}

ZTEST(suite_pscm, test_pscm_copy_pad_16)
{
	uint16_t copy_test_list[50];
	size_t output_size;
	int ret;

	ret = pscm_copy_pad(unpadded_left, sizeof(unpadded_left), 16, copy_test_list, &output_size);
	ZEQ(ret, 0);
	ZEQ(output_size, 2 * sizeof(unpadded_left));
	verify_array_eq(copy_test_list, copy_padded_16, output_size);
}

ZTEST(suite_pscm, test_pscm_copy_pad_24)
{
	uint8_t copy_test_list[50];
	size_t output_size;
	int ret;

	ret = pscm_copy_pad(unpadded_left, sizeof(unpadded_left), 24, copy_test_list, &output_size);
	ZEQ(ret, 0);
	ZEQ(output_size, 2 * sizeof(unpadded_left));
	verify_array_eq(copy_test_list, copy_padded_24, output_size);
}

ZTEST(suite_pscm, test_pscm_copy_pad_32)
{
	uint32_t copy_test_list[50];
	size_t output_size;
	int ret;

	ret = pscm_copy_pad(unpadded_left, sizeof(unpadded_left), 32, copy_test_list, &output_size);
	ZEQ(ret, 0);
	ZEQ(output_size, 2 * sizeof(unpadded_left));
	verify_array_eq(copy_test_list, copy_padded_32, output_size);
}

ZTEST(suite_pscm, test_pscm_combine_16)
{
	uint16_t combine_test_list[50];
	size_t output_size;
	int ret;

	ret = pscm_combine(unpadded_left, unpadded_right, sizeof(unpadded_left), 16,
			   combine_test_list, &output_size);
	ZEQ(ret, 0);
	ZEQ(output_size, 2 * sizeof(unpadded_left));
	verify_array_eq(combine_test_list, combine_16, output_size);
}

ZTEST(suite_pscm, test_pscm_combine_24)
{
	uint8_t combine_test_list[50];
	size_t output_size;
	int ret;

	ret = pscm_combine(unpadded_left, unpadded_right, sizeof(unpadded_left), 24,
			   combine_test_list, &output_size);
	ZEQ(ret, 0);
	ZEQ(output_size, 2 * sizeof(unpadded_left));
	verify_array_eq(combine_test_list, combine_24, output_size);
}

ZTEST(suite_pscm, test_pscm_combine_32)
{
	uint32_t combine_test_list[50];
	size_t output_size;
	int ret;

	ret = pscm_combine(unpadded_left, unpadded_right, sizeof(unpadded_left), 32,
			   combine_test_list, &output_size);
	ZEQ(ret, 0);
	ZEQ(output_size, 2 * sizeof(unpadded_left));
	verify_array_eq(combine_test_list, combine_32, output_size);
}

ZTEST(suite_pscm, test_pscm_one_channel_split_16)
{
	uint8_t left_test_list[50];
	uint8_t right_test_list[50];
	size_t output_size;
	int ret;

	/* Testing splitting */
	ret = pscm_one_channel_split(left_zero_padded_16, sizeof(left_zero_padded_16), AUDIO_CH_L,
				     16, left_test_list, &output_size);
	ZEQ(ret, 0);
	ZEQ(output_size, sizeof(left_zero_padded_16) / 2);
	verify_array_eq(left_test_list, unpadded_left, output_size);

	ret = pscm_one_channel_split(right_zero_padded_16, sizeof(right_zero_padded_16), AUDIO_CH_R,
				     16, right_test_list, &output_size);
	ZEQ(ret, 0);
	ZEQ(output_size, sizeof(right_zero_padded_16) / 2);
	verify_array_eq(right_test_list, unpadded_left, output_size);
}

ZTEST(suite_pscm, test_pscm_one_channel_split_24)
{
	uint8_t left_test_list[50];
	uint8_t right_test_list[50];
	size_t output_size;
	int ret;

	/* Testing splitting */
	ret = pscm_one_channel_split(left_zero_padded_24, sizeof(left_zero_padded_24), AUDIO_CH_L,
				     24, left_test_list, &output_size);
	ZEQ(ret, 0);
	ZEQ(output_size, sizeof(left_zero_padded_24) / 2);
	verify_array_eq(left_test_list, unpadded_left, output_size);

	ret = pscm_one_channel_split(right_zero_padded_24, sizeof(right_zero_padded_24), AUDIO_CH_R,
				     24, right_test_list, &output_size);
	ZEQ(ret, 0);
	ZEQ(output_size, sizeof(right_zero_padded_24) / 2);
	verify_array_eq(right_test_list, unpadded_left, output_size);
}

ZTEST(suite_pscm, test_pscm_one_channel_split_32)
{
	uint8_t left_test_list[50];
	uint8_t right_test_list[50];
	size_t output_size;
	int ret;

	/* Testing splitting */
	ret = pscm_one_channel_split(left_zero_padded_32, sizeof(left_zero_padded_32), AUDIO_CH_L,
				     32, left_test_list, &output_size);
	ZEQ(ret, 0);
	ZEQ(output_size, sizeof(left_zero_padded_32) / 2);
	verify_array_eq(left_test_list, unpadded_left, output_size);

	ret = pscm_one_channel_split(right_zero_padded_32, sizeof(right_zero_padded_32), AUDIO_CH_R,
				     32, right_test_list, &output_size);
	ZEQ(ret, 0);
	ZEQ(output_size, sizeof(right_zero_padded_32) / 2);
	verify_array_eq(right_test_list, unpadded_left, output_size);
}

ZTEST(suite_pscm, test_pscm_two_channel_split_16)
{
	uint16_t left_test_list[50];
	uint16_t right_test_list[50];
	size_t output_size;
	int ret;

	/* Testing splitting */
	ret = pscm_two_channel_split(stereo_split, sizeof(stereo_split), 16, left_test_list,
				     right_test_list, &output_size);
	ZEQ(ret, 0);
	ZEQ(output_size, sizeof(stereo_split) / 2);
	verify_array_eq(left_test_list, stereo_split_left_16, output_size);
	verify_array_eq(right_test_list, stereo_split_right_16, output_size);
}

ZTEST(suite_pscm, test_pscm_two_channel_split_24)
{
	uint8_t left_test_list[50];
	uint8_t right_test_list[50];
	size_t output_size;
	int ret;

	/* Testing splitting */
	ret = pscm_two_channel_split(stereo_split, sizeof(stereo_split), 24, left_test_list,
				     right_test_list, &output_size);
	ZEQ(ret, 0);
	ZEQ(output_size, sizeof(stereo_split) / 2);
	verify_array_eq(left_test_list, stereo_split_left_24, output_size);
	verify_array_eq(right_test_list, stereo_split_right_24, output_size);
}

ZTEST(suite_pscm, test_pscm_two_channel_split_32)
{
	uint32_t left_test_list[50];
	uint32_t right_test_list[50];
	size_t output_size;
	int ret;

	/* Testing splitting */
	ret = pscm_two_channel_split(stereo_split, sizeof(stereo_split), 32, left_test_list,
				     right_test_list, &output_size);
	ZEQ(ret, 0);
	ZEQ(output_size, sizeof(stereo_split) / 2);
	verify_array_eq(left_test_list, stereo_split_left_32, output_size);
	verify_array_eq(right_test_list, stereo_split_right_32, output_size);
}

ZTEST_SUITE(suite_pscm, NULL, NULL, NULL, NULL, NULL);
