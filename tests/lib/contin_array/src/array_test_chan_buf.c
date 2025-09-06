/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/ztest.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <zephyr/kernel.h>
#include <zephyr/tc_util.h>
#include <audio_defines.h>
#include <contin_array.h>

#include "array_test_data.h"

static struct audio_metadata test_meta = {.data_coding = PCM,
					  .data_len_us = 10000,
					  .sample_rate_hz = 48000,
					  .bits_per_sample = 16,
					  .carried_bits_per_sample = 16,
					  .bytes_per_location = CONTIN_TEST_FINITE_DATA_SIZE,
					  .interleaved = false,
					  .locations = 0x00000111,
					  .ref_ts_us = 0,
					  .data_rx_ts_us = 0,
					  .bad_data = false};

NET_BUF_POOL_FIXED_DEFINE(pool_contin, CONTIN_TEST_POOL_CONTIN_NUM, CONTIN_TEST_POOL_CONTIN_SIZE,
			  (sizeof(struct audio_metadata)), NULL);

/* Test parameter validation */
ZTEST(suite_contin_array_chan_buf, test_contin_single_buf_api)
{
	int ret;
	struct net_buf *pcm_contin = NULL;
	uint8_t *pcm_finite = (uint8_t *)test_arr;
	struct audio_metadata *meta_contin;
	uint8_t ch_out = TEST_CHANS_2;
	uint16_t finite_pos = 0;

	pcm_contin = net_buf_alloc(&pool_contin, K_NO_WAIT);

	meta_contin = net_buf_user_data(pcm_contin);
	memcpy(meta_contin, &test_meta, sizeof(struct audio_metadata));

	/* Test for pcm_contin->len == 0 */
	ret = contin_array_single_chan_buf_create(
		pcm_contin, pcm_finite, CONTIN_TEST_POOL_FINITE_SIZE, ch_out, &finite_pos);
	zassert_equal(ret, -EPERM, "Failed to recognize pcm_contin->len == 0: %d", ret);

	/* Test for pcm_contin pointer NULL */
	net_buf_add(pcm_contin, pcm_contin->size);
	ret = contin_array_single_chan_buf_create(NULL, pcm_finite, CONTIN_TEST_POOL_FINITE_SIZE,
						  ch_out, &finite_pos);
	zassert_equal(ret, -ENXIO, "Failed to recognize NULL pointer: %d", ret);

	/* Test for pcm_finite buffer pointer NULL */
	ret = contin_array_single_chan_buf_create(pcm_contin, NULL, CONTIN_TEST_POOL_FINITE_SIZE,
						  ch_out, &finite_pos);
	zassert_equal(ret, -ENXIO, "Failed to recognize NULL pointer: %d", ret);

	/* Test for pcm_finite buffer pointer NULL */
	ret = contin_array_single_chan_buf_create(pcm_contin, pcm_finite,
						  CONTIN_TEST_POOL_FINITE_SIZE, ch_out, NULL);
	zassert_equal(ret, -ENXIO, "Failed to recognize NULL pointer: %d", ret);

	/* Test for pcm_finite buffer size 0 */
	ret = contin_array_single_chan_buf_create(pcm_contin, pcm_finite, 0, ch_out, &finite_pos);
	zassert_equal(ret, -EPERM, "Failed to recognize buffer size is zero: %d", ret);

	/* Test for out of range finite_pos */
	finite_pos = CONTIN_TEST_POOL_FINITE_SIZE + 1;
	ret = contin_array_single_chan_buf_create(
		pcm_contin, pcm_finite, CONTIN_TEST_POOL_FINITE_SIZE, ch_out, &finite_pos);
	zassert_equal(ret, -EPERM, "Failed to recognize out of range finite_pos: %d", ret);

	/* Test for no channel in channels */
	ret = contin_array_single_chan_buf_create(pcm_contin, pcm_finite,
						  CONTIN_TEST_POOL_FINITE_SIZE, 10, &finite_pos);
	zassert_equal(ret, -EPERM, "Failed to recognize channel out of range: %d", ret);

	/* Test for invalid channels in the continuous buffer */
	meta_contin->locations = 0x00000001;
	ret = contin_array_single_chan_buf_create(
		pcm_contin, pcm_finite, CONTIN_TEST_POOL_FINITE_SIZE, ch_out, &finite_pos);
	zassert_equal(ret, -EPERM, "Failed to recognize channel out of range: %d", ret);

	/* Test for pcm_contin size 0 */
	memcpy(meta_contin, &test_meta, sizeof(struct audio_metadata));
	pcm_contin->size = 0;
	ret = contin_array_single_chan_buf_create(
		pcm_contin, pcm_finite, CONTIN_TEST_POOL_FINITE_SIZE, ch_out, &finite_pos);
	zassert_equal(ret, -EPERM, "Failed to recognize data size of 0: %d", ret);

	net_buf_unref(pcm_contin);
}

/* Test parameter validation */
ZTEST(suite_contin_array_chan_buf, test_contin_chans_buf_api)
{
	int ret;
	struct net_buf *pcm_contin = NULL;
	uint8_t *pcm_finite = (uint8_t *)test_arr;
	struct audio_metadata *meta_contin;
	uint16_t finite_pos = 0;

	pcm_contin = net_buf_alloc(&pool_contin, K_NO_WAIT);

	meta_contin = net_buf_user_data(pcm_contin);

	memcpy(meta_contin, &test_meta, sizeof(struct audio_metadata));

	/* Test for pcm_contin pointer NULL */
	ret = contin_array_chans_buf_create(NULL, pcm_finite, CONTIN_TEST_POOL_FINITE_SIZE,
					    &finite_pos);
	zassert_equal(ret, -ENXIO, "Failed to recognize NULL pointer: %d", ret);

	/* Test for pcm_finite buffer pointer NULL */
	ret = contin_array_chans_buf_create(pcm_contin, NULL, CONTIN_TEST_POOL_FINITE_SIZE,
					    &finite_pos);
	zassert_equal(ret, -ENXIO, "Failed to recognize NULL pointer: %d", ret);

	/* Test for pcm_finite buffer pointer NULL */
	ret = contin_array_chans_buf_create(pcm_contin, pcm_finite, CONTIN_TEST_POOL_FINITE_SIZE,
					    NULL);
	zassert_equal(ret, -ENXIO, "Failed to recognize NULL pointer: %d", ret);

	/* Test for pcm_finite buffer size 0 */
	ret = contin_array_chans_buf_create(pcm_contin, pcm_finite, 0, &finite_pos);
	zassert_equal(ret, -EPERM, "Failed to recognize buffer size is zero: %d", ret);

	/* Test for out of range finite_pos */
	finite_pos = CONTIN_TEST_POOL_FINITE_SIZE + 1;
	ret = contin_array_chans_buf_create(pcm_contin, pcm_finite, CONTIN_TEST_POOL_FINITE_SIZE,
					    &finite_pos);
	zassert_equal(ret, -EPERM, "Failed to recognize out of range finite_pos: %d", ret);

	/* Test for pcm_contin size 0 */
	memcpy(meta_contin, &test_meta, sizeof(struct audio_metadata));
	pcm_contin->size = 0;
	ret = contin_array_chans_buf_create(pcm_contin, pcm_finite, CONTIN_TEST_POOL_FINITE_SIZE,
					    &finite_pos);
	zassert_equal(ret, -EPERM, "Failed to recognize data size of 0: %d", ret);

	net_buf_unref(pcm_contin);
}

/* Test continuous array by setting a single channel in the output with the tone */
static void test_contin_single(uint8_t bits_per_samp, uint8_t carrier_bits, uint8_t num_ch,
			       uint8_t ch_out, bool interleaved, size_t iterations)
{
	int ret;
	struct net_buf *pcm_contin;
	struct audio_metadata *meta_contin;
	uint8_t *pcm_finite = (uint8_t *)test_arr;
	uint8_t carrier_bytes;
	uint16_t finite_pos = 0;
	uint16_t finite_start;

	pcm_contin = net_buf_alloc(&pool_contin, K_NO_WAIT);

	meta_contin = net_buf_user_data(pcm_contin);
	memcpy(meta_contin, &test_meta, sizeof(struct audio_metadata));

	meta_contin->bits_per_sample = bits_per_samp;
	meta_contin->carried_bits_per_sample = carrier_bits;
	carrier_bytes = carrier_bits / 8;
	meta_contin->bytes_per_location =
		((pcm_contin->size / num_ch) / carrier_bytes) * carrier_bytes;
	meta_contin->interleaved = interleaved;
	meta_contin->locations = (0x00000001 << num_ch) - 1;

	net_buf_add(pcm_contin, meta_contin->bytes_per_location * num_ch);

	for (size_t i = 0; i < iterations; i++) {
		finite_start = finite_pos;

		memset(pcm_contin->data, CONTIN_TEST_CONTIN_BYTE, pcm_contin->size);

		ret = contin_array_single_chan_buf_create(
			pcm_contin, pcm_finite, CONTIN_TEST_POOL_FINITE_SIZE, ch_out, &finite_pos);
		zassert_equal(ret, 0, "contin_array_chans_buf_create did not return zero: %d", ret);

		validate_contin_single_array(pcm_contin, pcm_finite, num_ch, ch_out, finite_start,
					     finite_pos, interleaved);
	}

	net_buf_unref(pcm_contin);
}

/* Test continuous array set all channels with the tone */
static void test_contin_chans(uint8_t bits_per_samp, uint8_t carrier_bits, uint8_t num_ch,
			      bool interleaved, size_t iterations)
{
	int ret;
	struct net_buf *pcm_contin;
	struct audio_metadata *meta_contin;
	uint8_t *pcm_finite = (uint8_t *)test_arr;
	uint8_t carrier_bytes;
	uint16_t finite_pos = 0;
	uint16_t finite_start;

	pcm_contin = net_buf_alloc(&pool_contin, K_NO_WAIT);

	for (size_t i = 0; i < iterations; i++) {
		finite_start = finite_pos;

		meta_contin = net_buf_user_data(pcm_contin);
		memcpy(meta_contin, &test_meta, sizeof(struct audio_metadata));

		meta_contin->bits_per_sample = bits_per_samp;
		meta_contin->carried_bits_per_sample = carrier_bits;
		carrier_bytes = carrier_bits / 8;
		meta_contin->bytes_per_location =
			((pcm_contin->size / num_ch) / carrier_bytes) * carrier_bytes;
		meta_contin->interleaved = interleaved;
		meta_contin->locations = (0x00000001 << num_ch) - 1;

		memset(pcm_contin->data, CONTIN_TEST_CONTIN_BYTE, pcm_contin->size);

		ret = contin_array_chans_buf_create(pcm_contin, pcm_finite,
						    CONTIN_TEST_POOL_FINITE_SIZE, &finite_pos);
		zassert_equal(ret, 0, "contin_array_chans_buf_create did not return zero: %d", ret);

		validate_contin_chans_array(pcm_contin, pcm_finite, num_ch, finite_start,
					    finite_pos, interleaved);

		net_buf_reset(pcm_contin);
	}

	net_buf_unref(pcm_contin);
}

/* Test continuous array formation for deinterleaved net_buffers */
ZTEST(suite_contin_array_chan_buf, test_contin_single_buf_deint_loop)
{
	test_contin_single(TEST_BITS_8, TEST_BITS_8, TEST_CHANS_1, TEST_CHANS_1, false, 100);
	test_contin_single(TEST_BITS_8, TEST_BITS_16, TEST_CHANS_2, TEST_CHANS_2, false, 110);
	test_contin_single(TEST_BITS_8, TEST_BITS_32, TEST_CHANS_3, TEST_CHANS_2, false, 120);
	test_contin_single(TEST_BITS_16, TEST_BITS_16, TEST_CHANS_1, TEST_CHANS_1, false, 100);
	test_contin_single(TEST_BITS_16, TEST_BITS_32, TEST_CHANS_2, TEST_CHANS_2, false, 110);
	test_contin_single(TEST_BITS_24, TEST_BITS_32, TEST_CHANS_3, TEST_CHANS_2, false, 120);
	test_contin_single(TEST_BITS_32, TEST_BITS_32, TEST_CHANS_2, TEST_CHANS_2, false, 120);
}

/* Test continuous array formation for interleaved net_buffers */
ZTEST(suite_contin_array_chan_buf, test_contin_single_buf_int_loop)
{
	test_contin_single(TEST_BITS_8, TEST_BITS_8, TEST_CHANS_1, TEST_CHANS_1, true, 100);
	test_contin_single(TEST_BITS_8, TEST_BITS_16, TEST_CHANS_2, TEST_CHANS_2, true, 110);
	test_contin_single(TEST_BITS_8, TEST_BITS_32, TEST_CHANS_3, TEST_CHANS_2, true, 120);
	test_contin_single(TEST_BITS_16, TEST_BITS_16, TEST_CHANS_1, TEST_CHANS_1, true, 100);
	test_contin_single(TEST_BITS_16, TEST_BITS_32, TEST_CHANS_2, TEST_CHANS_2, true, 110);
	test_contin_single(TEST_BITS_24, TEST_BITS_32, TEST_CHANS_3, TEST_CHANS_2, true, 120);
	test_contin_single(TEST_BITS_32, TEST_BITS_32, TEST_CHANS_2, TEST_CHANS_2, true, 120);
}

/* Test continuous array formation for deinterleaved net_buffers */
ZTEST(suite_contin_array_chan_buf, test_contin_chans_buf_deint_loop)
{
	test_contin_chans(TEST_BITS_8, TEST_BITS_8, TEST_CHANS_1, false, 100);
	test_contin_chans(TEST_BITS_8, TEST_BITS_16, TEST_CHANS_2, false, 110);
	test_contin_chans(TEST_BITS_8, TEST_BITS_32, TEST_CHANS_3, false, 120);
	test_contin_chans(TEST_BITS_16, TEST_BITS_16, TEST_CHANS_1, false, 100);
	test_contin_chans(TEST_BITS_16, TEST_BITS_32, TEST_CHANS_2, false, 110);
	test_contin_chans(TEST_BITS_24, TEST_BITS_32, TEST_CHANS_2, false, 120);
	test_contin_chans(TEST_BITS_32, TEST_BITS_32, TEST_CHANS_2, false, 120);
}

/* Test continuous array formation for interleaved net_buffers */
ZTEST(suite_contin_array_chan_buf, test_contin_chans_buf_int_loop)
{
	test_contin_chans(TEST_BITS_8, TEST_BITS_8, TEST_CHANS_1, true, 100);
	test_contin_chans(TEST_BITS_8, TEST_BITS_16, TEST_CHANS_2, true, 110);
	test_contin_chans(TEST_BITS_8, TEST_BITS_32, TEST_CHANS_3, true, 120);
	test_contin_chans(TEST_BITS_16, TEST_BITS_16, TEST_CHANS_1, true, 100);
	test_contin_chans(TEST_BITS_16, TEST_BITS_32, TEST_CHANS_2, true, 110);
	test_contin_chans(TEST_BITS_24, TEST_BITS_32, TEST_CHANS_2, true, 120);
	test_contin_chans(TEST_BITS_32, TEST_BITS_32, TEST_CHANS_2, true, 120);
}
