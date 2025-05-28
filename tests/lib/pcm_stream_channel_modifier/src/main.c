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

enum test_audio_channel {
	TEST_AUDIO_CH_L = 0,
	TEST_AUDIO_CH_R,
	TEST_AUDIO_CH_C,
	TEST_AUDIO_CH_SL,
	TEST_AUDIO_CH_SR
};

enum test_channel_mode {
	TEST_CHANNELS_1 = 1,
	TEST_CHANNELS_2 = 2,
	TEST_CHANNELS_3 = 3,
	TEST_CHANNELS_4 = 4,
	TEST_CHANNELS_5 = 5
};

#define TEST_SAMPLE_BITS_8  8
#define TEST_SAMPLE_BITS_16 16
#define TEST_SAMPLE_BITS_24 24
#define TEST_SAMPLE_BITS_32 32

void verify_array_eq(void *p1, void *p2, size_t size)
{
	ZEQ(memcmp(p1, p2, size), 0);
}

/* Input arrays */
uint8_t unpadded_left[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
uint8_t unpadded_right[] = {13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24};
uint8_t unpadded_centre[] = {25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36};
uint8_t unpadded_surround_left[] = {37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48};
uint8_t unpadded_surround_right[] = {49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60};
uint8_t stereo_split[] = {1, 13, 2, 14, 3, 15, 4,  16, 5,  17, 6,  18,
			  7, 19, 8, 20, 9, 21, 10, 22, 11, 23, 12, 24};
uint8_t multi_split[] = {1,  13, 25, 37, 49, 2,	 14, 26, 38, 50, 3,  15, 27, 39, 51,
			 4,  16, 28, 40, 52, 5,	 17, 29, 41, 53, 6,  18, 30, 42, 54,
			 7,  19, 31, 43, 55, 8,	 20, 32, 44, 56, 9,  21, 33, 45, 57,
			 10, 22, 34, 46, 58, 11, 23, 35, 47, 59, 12, 24, 36, 48, 60};

/* Result arrays */
uint8_t left_zero_padded_8[] = {1, 0, 2, 0, 3, 0, 4,  0, 5,  0, 6,  0,
				7, 0, 8, 0, 9, 0, 10, 0, 11, 0, 12, 0};
uint8_t right_zero_padded_8[] = {0, 13, 0, 14, 0, 15, 0, 16, 0, 17, 0, 18,
				 0, 19, 0, 20, 0, 21, 0, 22, 0, 23, 0, 24};
uint8_t copy_padded_8[] = {1, 1, 2, 2, 3, 3, 4,	 4,  5,	 5,  6,	 6,
			   7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12};
uint8_t combine_8_ch2[] = {1, 13, 2, 14, 3, 15, 4,  16, 5,  17, 6,  18,
			   7, 19, 8, 20, 9, 21, 10, 22, 11, 23, 12, 24};

uint8_t left_zero_padded_16[] = {1, 2, 0, 0, 3, 4,  0, 0, 5,  6,  0, 0,
				 7, 8, 0, 0, 9, 10, 0, 0, 11, 12, 0, 0};
uint8_t right_zero_padded_16[] = {0, 0, 1, 2, 0, 0, 3, 4,  0, 0, 5,  6,
				  0, 0, 7, 8, 0, 0, 9, 10, 0, 0, 11, 12};
uint8_t copy_padded_16[] = {1, 2, 1, 2, 3, 4,  3, 4,  5,  6,  5,  6,
			    7, 8, 7, 8, 9, 10, 9, 10, 11, 12, 11, 12};
uint8_t combine_16[] = {1, 2, 13, 14, 3, 4,  15, 16, 5,	 6,  17, 18,
			7, 8, 19, 20, 9, 10, 21, 22, 11, 12, 23, 24};
uint8_t stereo_split_left_16[] = {1, 13, 3, 15, 5, 17, 7, 19, 9, 21, 11, 23};
uint8_t stereo_split_right_16[] = {2, 14, 4, 16, 6, 18, 8, 20, 10, 22, 12, 24};

uint8_t left_zero_padded_24[] = {1, 2, 3, 0, 0, 0, 4,  5,  6,  0, 0, 0,
				 7, 8, 9, 0, 0, 0, 10, 11, 12, 0, 0, 0};
uint8_t right_zero_padded_24[] = {0, 0, 0, 1, 2, 3, 0, 0, 0, 4,	 5,  6,
				  0, 0, 0, 7, 8, 9, 0, 0, 0, 10, 11, 12};
uint8_t copy_padded_24[] = {1, 2, 3, 1, 2, 3, 4,  5,  6,  4,  5,  6,
			    7, 8, 9, 7, 8, 9, 10, 11, 12, 10, 11, 12};
uint8_t combine_24[] = {1, 2, 3, 13, 14, 15, 4,	 5,  6,	 16, 17, 18,
			7, 8, 9, 19, 20, 21, 10, 11, 12, 22, 23, 24};
uint8_t stereo_split_left_24[] = {1, 13, 2, 4, 16, 5, 7, 19, 8, 10, 22, 11};
uint8_t stereo_split_right_24[] = {14, 3, 15, 17, 6, 18, 20, 9, 21, 23, 12, 24};

uint8_t left_zero_padded_32[] = {1, 2, 3, 4, 0, 0,  0,	0,  5, 6, 7, 8,
				 0, 0, 0, 0, 9, 10, 11, 12, 0, 0, 0, 0};
uint8_t right_zero_padded_32[] = {0, 0, 0, 0, 1, 2, 3, 4, 0, 0,	 0,  0,
				  5, 6, 7, 8, 0, 0, 0, 0, 9, 10, 11, 12};
uint8_t copy_padded_32[] = {1, 2, 3, 4, 1, 2,  3,  4,  5, 6,  7,  8,
			    5, 6, 7, 8, 9, 10, 11, 12, 9, 10, 11, 12};
uint8_t combine_32[] = {1,  2,	3,  4,	13, 14, 15, 16, 5,  6,	7,  8,
			17, 18, 19, 20, 9,  10, 11, 12, 21, 22, 23, 24};
uint8_t stereo_split_left_32[] = {1, 13, 2, 14, 5, 17, 6, 18, 9, 21, 10, 22};
uint8_t stereo_split_right_32[] = {3, 15, 4, 16, 7, 19, 8, 20, 11, 23, 12, 24};

#define PCM_INT_TEST_SIZE	50
#define TEST_PCM_INT_SIZE	sizeof(multi_split)
#define TEST_PCM_DEINT_SIZE	sizeof(unpadded_left)
#define TEST_PCM_INT_MULTI_SIZE sizeof(multi_split)

ZTEST(suite_pscm_int, test_pscm_interleave_api_parameters)
{
	int ret;
	uint8_t input[2], output[2];
	size_t input_size = sizeof(input);
	size_t output_size = sizeof(output);
	uint8_t channel = 0;
	uint8_t output_channels = 2;
	uint8_t pcm_bit_depth = 8;

	ret = pscm_interleave(NULL, input_size, channel, pcm_bit_depth, &output, output_size,
			      output_channels);
	zassert_equal(ret, -EINVAL, "Failed interleave for input NULL: ret %d", ret);

	ret = pscm_interleave(input, 0, channel, pcm_bit_depth, &output, output_size,
			      output_channels);
	zassert_equal(ret, -EINVAL, "Failed interleave for input size 0: ret %d", ret);

	ret = pscm_interleave(input, input_size, 10, pcm_bit_depth, &output, output_size,
			      output_channels);
	zassert_equal(ret, -EINVAL, "Failed interleave channel > output_channels: ret %d", ret);

	ret = pscm_interleave(input, input_size, channel, 0, &output, output_size, output_channels);
	zassert_equal(ret, -EINVAL, "Failed interleave pcm_bit_depth == 0: ret %d", ret);

	ret = pscm_interleave(input, input_size, channel, 12, &output, output_size,
			      output_channels);
	zassert_equal(ret, -EINVAL, "Failed interleave pcm_bit_depth not divisible by 8: ret %d",
		      ret);

	ret = pscm_interleave(input, input_size, channel, 40, &output, output_size,
			      output_channels);
	zassert_equal(ret, -EINVAL, "Failed interleave pcm_bit_depth greater than %d: ret %d",
		      PSCM_MAX_CARRIER_BIT_DEPTH, ret);

	ret = pscm_interleave(input, input_size, channel, pcm_bit_depth, NULL, output_size,
			      output_channels);
	zassert_equal(ret, -EINVAL, "Failed interleave for output NULL: ret %d", ret);

	ret = pscm_interleave(input, input_size, channel, pcm_bit_depth, &output, 0,
			      output_channels);
	zassert_equal(ret, -EINVAL, "Failed interleave for output size 0: ret %d", ret);

	ret = pscm_interleave(input, input_size, channel, pcm_bit_depth, &output,
			      input_size * (output_channels - 1), output_channels);
	zassert_equal(ret, -EINVAL, "Failed interleave for output size too small: ret %d", ret);
}

ZTEST(suite_pscm_deint, test_pscm_deinterleave_api_parameters)
{
	int ret;
	uint8_t input[2], output[2];
	size_t input_size = sizeof(input);
	size_t output_size = sizeof(output);
	size_t test_output_size = 0;
	uint8_t channel = 0;
	uint8_t input_channels = 2;
	uint8_t pcm_bit_depth = 8;

	ret = pscm_deinterleave(NULL, input_size, input_channels, channel, pcm_bit_depth, &output,
				output_size);
	zassert_equal(ret, -EINVAL, "Failed de-interleave for input NULL: ret %d", ret);

	ret = pscm_deinterleave(input, 0, input_channels, channel, pcm_bit_depth, &output,
				output_size);
	zassert_equal(ret, -EINVAL, "Failed de-interleave for input size 0: ret %d", ret);

	ret = pscm_deinterleave(input, input_size, 0, channel, pcm_bit_depth, &output, output_size);
	zassert_equal(ret, -EINVAL, "Failed de-interleave input channels == 0: ret %d", ret);

	ret = pscm_deinterleave(input, input_size, input_channels, channel, 0, &output,
				output_size);
	zassert_equal(ret, -EINVAL, "Failed de-interleave pcm_bit_depth == 0: ret %d", ret);

	ret = pscm_deinterleave(input, input_size, input_channels, channel, 12, &output,
				output_size);
	zassert_equal(ret, -EINVAL, "Failed de-interleave pcm_bit_depth not divisible by 8: ret %d",
		      ret);

	ret = pscm_deinterleave(input, input_size, input_channels, channel, 40, &output,
				output_size);
	zassert_equal(ret, -EINVAL, "Failed de-interleave pcm_bit_depth greater than %d: ret %d",
		      PSCM_MAX_CARRIER_BIT_DEPTH, ret);

	ret = pscm_deinterleave(input, input_size, input_channels, channel, pcm_bit_depth, NULL,
				output_size);
	zassert_equal(ret, -EINVAL, "Failed de-interleave for output NULL: ret %d", ret);

	ret = pscm_deinterleave(input, input_size, input_channels, channel, pcm_bit_depth, &output,
				test_output_size);
	zassert_equal(ret, -EINVAL, "Failed de-interleave for output size 0: ret %d", ret);

	test_output_size = output_size < (input_size / (input_channels + 1));
	ret = pscm_deinterleave(input, input_size, input_channels, channel, pcm_bit_depth, &output,
				test_output_size);
	zassert_equal(ret, -EINVAL, "Failed de-interleave for output size too small: ret %d", ret);
}

int interleave_test(void const *const input, size_t input_size, uint8_t pcm_bit_depth,
		    uint32_t channel, uint8_t output_channels, uint8_t *test_result,
		    size_t test_result_size, bool zero)
{
	int ret;
	uint8_t output[TEST_PCM_INT_SIZE];
	size_t output_size = sizeof(output);

	if (zero) {
		memset(output, 0, sizeof(output));
	}

	ret = pscm_interleave(input, input_size, channel, pcm_bit_depth, &output, output_size,
			      output_channels);
	zassert_equal(ret, 0, "Failed interleave: ret %d", ret);
	zassert_mem_equal(&output[0], &test_result[0], test_result_size,
			  "Failed to interleave, output != test_result");

	return 0;
}

int deinterleave_test(void const *const input, size_t input_size, uint8_t pcm_bit_depth,
		      uint32_t channel, uint8_t input_channels, uint8_t *test_result,
		      size_t test_result_size)
{
	int ret;
	uint8_t output[TEST_PCM_DEINT_SIZE];
	size_t output_size = sizeof(output);

	ret = pscm_deinterleave(input, input_size, input_channels, channel, pcm_bit_depth, &output,
				output_size);
	zassert_equal(ret, 0, "Failed de-interleave: ret %d", ret);
	zassert_mem_equal(&output[0], &test_result[0], test_result_size,
			  "Failed to interleave, output != test_result");

	return 0;
}

ZTEST(suite_pscm_deint, test_pscm_deinterleave)
{
	int ret;
	uint8_t num_chans = 2;

	ret = deinterleave_test(&combine_8_ch2[0], sizeof(combine_8_ch2), TEST_SAMPLE_BITS_8,
				TEST_AUDIO_CH_L, num_chans, &unpadded_left[0],
				sizeof(unpadded_left));
	zassert_equal(ret, 0, "Failed de-interleave 2 channel 8-bit carrier left: ret %d", ret);

	ret = deinterleave_test(&combine_8_ch2[0], sizeof(combine_8_ch2), TEST_SAMPLE_BITS_8,
				TEST_AUDIO_CH_R, num_chans, &unpadded_right[0],
				sizeof(unpadded_right));
	zassert_equal(ret, 0, "Failed de-interleave 2 channel 8-bit carrier right: ret %d", ret);

	ret = deinterleave_test(&combine_16[0], sizeof(combine_16), TEST_SAMPLE_BITS_16,
				TEST_AUDIO_CH_L, num_chans, &unpadded_left[0],
				sizeof(unpadded_left));
	zassert_equal(ret, 0, "Failed de-interleave 2 channel 16-bit carrier left: ret %d", ret);

	ret = deinterleave_test(&combine_16[0], sizeof(combine_16), TEST_SAMPLE_BITS_16,
				TEST_AUDIO_CH_R, num_chans, &unpadded_right[0],
				sizeof(unpadded_right));
	zassert_equal(ret, 0, "Failed de-interleave 2 channel 16-bit carrier right: ret %d", ret);

	ret = deinterleave_test(&combine_24[0], sizeof(combine_24), TEST_SAMPLE_BITS_24,
				TEST_AUDIO_CH_L, num_chans, &unpadded_left[0],
				sizeof(unpadded_left));
	zassert_equal(ret, 0, "Failed de-interleave 2 channel 24-bit carrier left: ret %d", ret);

	ret = deinterleave_test(&combine_24[0], sizeof(combine_24), TEST_SAMPLE_BITS_24,
				TEST_AUDIO_CH_R, num_chans, &unpadded_right[0],
				sizeof(unpadded_right));
	zassert_equal(ret, 0, "Failed de-interleave 2 channel 24-bit carrier right: ret %d", ret);

	ret = deinterleave_test(&combine_32[0], sizeof(combine_32), TEST_SAMPLE_BITS_32,
				TEST_AUDIO_CH_L, num_chans, &unpadded_left[0],
				sizeof(unpadded_left));
	zassert_equal(ret, 0, "Failed de-interleave 2 channel 32-bit carrier left: ret %d", ret);

	ret = deinterleave_test(&combine_32[0], sizeof(combine_32), TEST_SAMPLE_BITS_32,
				TEST_AUDIO_CH_R, num_chans, &unpadded_right[0],
				sizeof(unpadded_right));
	zassert_equal(ret, 0, "Failed de-interleave 2 channel 32-bit carrier right: ret %d", ret);
}

ZTEST(suite_pscm_int, test_pscm_interleave_zero_pad)
{
	int ret;
	uint8_t num_chans = 2;

	ret = interleave_test(&unpadded_left[0], sizeof(unpadded_left), TEST_SAMPLE_BITS_8,
			      TEST_AUDIO_CH_L, num_chans, &left_zero_padded_8[0],
			      sizeof(left_zero_padded_8), true);
	zassert_equal(ret, 0, "Failed interleave 2 channel 8-bit carrier left: ret %d", ret);

	ret = interleave_test(&unpadded_right[0], sizeof(unpadded_right), TEST_SAMPLE_BITS_8,
			      TEST_AUDIO_CH_R, num_chans, &right_zero_padded_8[0],
			      sizeof(right_zero_padded_8), true);
	zassert_equal(ret, 0, "Failed de-interleave 2 channel 8-bit carrier right: ret %d", ret);

	ret = interleave_test(&unpadded_left[0], sizeof(unpadded_left), TEST_SAMPLE_BITS_16,
			      TEST_AUDIO_CH_L, num_chans, &left_zero_padded_16[0],
			      sizeof(left_zero_padded_16), true);
	zassert_equal(ret, 0, "Failed de-interleave 2 channel 16-bit carrier left: ret %d", ret);

	ret = interleave_test(&unpadded_left[0], sizeof(unpadded_left), TEST_SAMPLE_BITS_16,
			      TEST_AUDIO_CH_R, num_chans, &right_zero_padded_16[0],
			      sizeof(right_zero_padded_16), true);
	zassert_equal(ret, 0, "Failed de-interleave 2 channel 16-bit carrier right: ret %d", ret);

	ret = interleave_test(&unpadded_left[0], sizeof(unpadded_left), TEST_SAMPLE_BITS_24,
			      TEST_AUDIO_CH_L, num_chans, &left_zero_padded_24[0],
			      sizeof(left_zero_padded_24), true);
	zassert_equal(ret, 0, "Failed de-interleave 2 channel 24-bit carrier left: ret %d", ret);

	ret = interleave_test(&unpadded_left[0], sizeof(unpadded_left), TEST_SAMPLE_BITS_24,
			      TEST_AUDIO_CH_R, num_chans, &right_zero_padded_24[0],
			      sizeof(right_zero_padded_24), true);
	zassert_equal(ret, 0, "Failed de-interleave 2 channel 24-bit carrier right: ret %d", ret);

	ret = interleave_test(&unpadded_left[0], sizeof(unpadded_left), TEST_SAMPLE_BITS_32,
			      TEST_AUDIO_CH_L, num_chans, &left_zero_padded_32[0],
			      sizeof(left_zero_padded_32), true);
	zassert_equal(ret, 0, "Failed interleave 2 channel 32-bit carrier left: ret %d", ret);

	ret = interleave_test(&unpadded_left[0], sizeof(unpadded_left), TEST_SAMPLE_BITS_32,
			      TEST_AUDIO_CH_R, num_chans, &right_zero_padded_32[0],
			      sizeof(right_zero_padded_32), true);
	zassert_equal(ret, 0, "Failed interleave 2 channel 32-bit carrier right : ret %d", ret);
}

ZTEST(suite_pscm_int, test_pscm_interleave_multi)
{
	int ret;
	uint8_t output[TEST_PCM_INT_MULTI_SIZE] = {0};

	ret = pscm_interleave(&unpadded_left[0], sizeof(unpadded_left), TEST_AUDIO_CH_L,
			      TEST_SAMPLE_BITS_8, &output[0], TEST_PCM_INT_MULTI_SIZE,
			      TEST_CHANNELS_5);
	zassert_equal(ret, 0, "Failed interleave: ret %d", ret);

	ret = pscm_interleave(&unpadded_right[0], sizeof(unpadded_right), TEST_AUDIO_CH_R,
			      TEST_SAMPLE_BITS_8, &output, TEST_PCM_INT_MULTI_SIZE,
			      TEST_CHANNELS_5);
	zassert_equal(ret, 0, "Failed interleave: ret %d", ret);

	ret = pscm_interleave(&unpadded_centre[0], sizeof(unpadded_centre), TEST_AUDIO_CH_C,
			      TEST_SAMPLE_BITS_8, &output[0], TEST_PCM_INT_MULTI_SIZE,
			      TEST_CHANNELS_5);
	zassert_equal(ret, 0, "Failed interleave: ret %d", ret);

	ret = pscm_interleave(&unpadded_surround_left[0], sizeof(unpadded_surround_left),
			      TEST_AUDIO_CH_SL, TEST_SAMPLE_BITS_8, &output[0],
			      TEST_PCM_INT_MULTI_SIZE, TEST_CHANNELS_5);
	zassert_equal(ret, 0, "Failed interleave: ret %d", ret);

	ret = pscm_interleave(&unpadded_surround_right[0], sizeof(unpadded_surround_right),
			      TEST_AUDIO_CH_SR, TEST_SAMPLE_BITS_8, &output[0],
			      TEST_PCM_INT_MULTI_SIZE, TEST_CHANNELS_5);
	zassert_equal(ret, 0, "Failed interleave: ret %d", ret);

	zassert_mem_equal(&output[0], &multi_split[0], sizeof(multi_split),
			  "Failed to interleave multi channels, output != multi_split");
}

ZTEST(suite_pscm_deint, test_pscm_deinterleave_multi)
{
	int ret;

	ret = deinterleave_test(&multi_split[0], sizeof(multi_split), TEST_SAMPLE_BITS_8,
				TEST_AUDIO_CH_L, TEST_CHANNELS_5, &unpadded_left[0],
				sizeof(unpadded_left));
	zassert_equal(ret, 0, "Failed de-interleave 8-bit carrier left: ret %d", ret);

	ret = deinterleave_test(&multi_split[0], sizeof(multi_split), TEST_SAMPLE_BITS_8,
				TEST_AUDIO_CH_R, TEST_CHANNELS_5, &unpadded_right[0],
				sizeof(unpadded_right));
	zassert_equal(ret, 0, "Failed de-interleave 8-bit carrier right: ret %d", ret);

	ret = deinterleave_test(&multi_split[0], sizeof(multi_split), TEST_SAMPLE_BITS_8,
				TEST_AUDIO_CH_C, TEST_CHANNELS_5, &unpadded_centre[0],
				sizeof(unpadded_centre));
	zassert_equal(ret, 0, "Failed de-interleave 8-bit carrier centre: ret %d", ret);

	ret = deinterleave_test(&multi_split[0], sizeof(multi_split), TEST_SAMPLE_BITS_8,
				TEST_AUDIO_CH_SL, TEST_CHANNELS_5, &unpadded_surround_left[0],
				sizeof(unpadded_surround_left));
	zassert_equal(ret, 0, "Failed de-interleave 8-bit carrier surround left: ret %d", ret);

	ret = deinterleave_test(&multi_split[0], sizeof(multi_split), TEST_SAMPLE_BITS_8,
				TEST_AUDIO_CH_SR, TEST_CHANNELS_5, &unpadded_surround_right[0],
				sizeof(unpadded_surround_right));
	zassert_equal(ret, 0, "Failed de-interleave 8-bit carrier surround right: ret %d", ret);
}

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
ZTEST_SUITE(suite_pscm_int, NULL, NULL, NULL, NULL, NULL);
ZTEST_SUITE(suite_pscm_deint, NULL, NULL, NULL, NULL, NULL);
