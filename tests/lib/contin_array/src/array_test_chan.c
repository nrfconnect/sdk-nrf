/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
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
					  .locations = TEST_LOC_L,
					  .ref_ts_us = 0,
					  .data_rx_ts_us = 0,
					  .bad_data = false};

NET_BUF_POOL_FIXED_DEFINE(pool_contin, CONTIN_TEST_POOL_CONTIN_NUM, CONTIN_TEST_POOL_CONTIN_SIZE,
			  (sizeof(struct audio_metadata)), NULL);
NET_BUF_POOL_FIXED_DEFINE(pool_finite, CONTIN_TEST_POOL_FINITE_NUM, CONTIN_TEST_POOL_FINITE_SIZE,
			  sizeof(struct audio_metadata), NULL);

/* Test continuous array by setting a single channel in the output with the tone */
static void test_contin(uint8_t bits_per_samp, uint8_t carrier_bits, uint8_t contin_locs,
			uint8_t locs_out, bool interleaved, bool filled, size_t iterations)
{
	int ret;
	struct net_buf *pcm_contin;
	struct net_buf *pcm_finite;
	struct audio_metadata *meta_contin;
	struct audio_metadata *meta_finite;
	uint8_t *pcm_finite_buf = (uint8_t *)test_arr;
	uint8_t num_ch;
	uint8_t carrier_bytes = carrier_bits / 8;
	uint16_t finite_pos = 0;
	uint16_t finite_start;

	for (size_t i = 0; i < iterations; i++) {
		finite_start = finite_pos;

		pcm_contin = net_buf_alloc(&pool_contin, K_NO_WAIT);

		meta_contin = net_buf_user_data(pcm_contin);
		memcpy(meta_contin, &test_meta, sizeof(struct audio_metadata));
		meta_contin->bits_per_sample = bits_per_samp;
		meta_contin->carried_bits_per_sample = carrier_bits;
		meta_contin->interleaved = interleaved;
		meta_contin->locations = contin_locs;
		num_ch = metadata_num_ch_get(meta_contin);
		meta_contin->bytes_per_location =
			((pcm_contin->size / num_ch) / carrier_bytes) * carrier_bytes;

		memset(pcm_contin->data, CONTIN_TEST_BYTE, pcm_contin->size);

		if (filled) {
			net_buf_add(pcm_contin, meta_contin->bytes_per_location * num_ch);
		}

		ret = contin_array_buf_create(pcm_contin, pcm_finite_buf,
					      CONTIN_TEST_POOL_FINITE_SIZE, locs_out, &finite_pos);
		zassert_equal(ret, 0, "contin_array_buf_create did not return zero: %d", ret);

		validate_contin_array(pcm_contin, pcm_finite_buf, locs_out, finite_start,
				      finite_pos, interleaved, filled);

		net_buf_unref(pcm_contin);
	}

	/* Test with two net_bufs */
	finite_pos = 0;

	pcm_finite = net_buf_alloc(&pool_finite, K_NO_WAIT);
	net_buf_add_mem(pcm_finite, (uint8_t *)test_arr, CONTIN_TEST_POOL_FINITE_SIZE);

	meta_finite = net_buf_user_data(pcm_finite);
	memcpy(meta_finite, &test_meta, sizeof(struct audio_metadata));
	meta_finite->bits_per_sample = bits_per_samp;
	meta_finite->carried_bits_per_sample = carrier_bits;
	meta_finite->interleaved = interleaved;
	meta_finite->bytes_per_location = CONTIN_TEST_POOL_FINITE_SIZE;
	meta_finite->locations = TEST_LOC_MONO;

	for (size_t i = 0; i < iterations; i++) {
		finite_start = finite_pos;

		pcm_contin = net_buf_alloc(&pool_contin, K_NO_WAIT);

		meta_contin = net_buf_user_data(pcm_contin);
		memcpy(meta_contin, &test_meta, sizeof(struct audio_metadata));
		meta_contin->bits_per_sample = bits_per_samp;
		meta_contin->carried_bits_per_sample = carrier_bits;
		carrier_bytes = carrier_bits / 8;
		meta_contin->interleaved = interleaved;
		meta_contin->locations = contin_locs;
		num_ch = metadata_num_ch_get(meta_contin);
		meta_contin->bytes_per_location =
			((pcm_contin->size / num_ch) / carrier_bytes) * carrier_bytes;

		memset(pcm_contin->data, CONTIN_TEST_BYTE, pcm_contin->size);

		if (filled) {
			net_buf_add(pcm_contin, meta_contin->bytes_per_location * num_ch);
		}

		ret = contin_array_net_buf_create(pcm_contin, pcm_finite, locs_out, &finite_pos);
		zassert_equal(ret, 0, "contin_array_buf_create did not return zero: %d", ret);

		validate_contin_array(pcm_contin, pcm_finite->data, locs_out, finite_start,
				      finite_pos, interleaved, filled);

		net_buf_unref(pcm_contin);
	}

	net_buf_unref(pcm_finite);
}

/* Test continuous array formation for de-interleaved net_buffers */
ZTEST(suite_contin_array_chan, test_contin_deint_empty_loop)
{
	test_contin(TEST_BITS_8, TEST_BITS_8, TEST_LOC_MONO, TEST_LOC_MONO, false, false, 100);
	test_contin(TEST_BITS_8, TEST_BITS_8, TEST_LOC_L, TEST_LOC_L, false, false, 100);
	test_contin(TEST_BITS_8, TEST_BITS_8, TEST_LOC_R, TEST_LOC_R, false, false, 100);
	test_contin(TEST_BITS_8, TEST_BITS_16, TEST_LOC_L_R, TEST_LOC_L_R, false, false, 110);
	test_contin(TEST_BITS_8, TEST_BITS_16, TEST_LOC_L_R_C, TEST_LOC_L, false, false, 120);
	test_contin(TEST_BITS_16, TEST_BITS_16, TEST_LOC_L, TEST_LOC_L, false, false, 100);
	test_contin(TEST_BITS_16, TEST_BITS_16, TEST_LOC_R, TEST_LOC_R, true, true, 100);
	test_contin(TEST_BITS_16, TEST_BITS_16, TEST_LOC_L_R_C, TEST_LOC_L_R_C, false, false, 100);
	test_contin(TEST_BITS_16, TEST_BITS_32, TEST_LOC_L_R, TEST_LOC_L_R, false, false, 110);
	test_contin(TEST_BITS_24, TEST_BITS_32, TEST_LOC_L_R_C, TEST_LOC_L_R, false, false, 120);
	test_contin(TEST_BITS_32, TEST_BITS_32, TEST_LOC_L_R, TEST_LOC_L_R, false, false, 120);
}

/* Test continuous array formation for interleaved net_buffers */
ZTEST(suite_contin_array_chan, test_contin_int_empty_loop)
{
	test_contin(TEST_BITS_8, TEST_BITS_8, TEST_LOC_MONO, TEST_LOC_MONO, true, false, 100);
	test_contin(TEST_BITS_8, TEST_BITS_8, TEST_LOC_L, TEST_LOC_L, true, false, 100);
	test_contin(TEST_BITS_8, TEST_BITS_8, TEST_LOC_R, TEST_LOC_R, true, false, 100);
	test_contin(TEST_BITS_8, TEST_BITS_16, TEST_LOC_L_R, TEST_LOC_L_R, true, false, 110);
	test_contin(TEST_BITS_8, TEST_BITS_16, TEST_LOC_L_R_C, TEST_LOC_L, true, false, 120);
	test_contin(TEST_BITS_16, TEST_BITS_16, TEST_LOC_L, TEST_LOC_L, true, false, 100);
	test_contin(TEST_BITS_16, TEST_BITS_16, TEST_LOC_R, TEST_LOC_R, true, true, 100);
	test_contin(TEST_BITS_16, TEST_BITS_16, TEST_LOC_L_R_C, TEST_LOC_L_R_C, true, false, 100);
	test_contin(TEST_BITS_16, TEST_BITS_32, TEST_LOC_L_R, TEST_LOC_L_R, true, false, 110);
	test_contin(TEST_BITS_24, TEST_BITS_32, TEST_LOC_L_R_C, TEST_LOC_L_R, true, false, 120);
	test_contin(TEST_BITS_32, TEST_BITS_32, TEST_LOC_L_R, TEST_LOC_L_R, true, false, 120);
}

/* Test continuous array formation for de-interleaved net_buffers */
ZTEST(suite_contin_array_chan, test_contin_deint_filled_loop)
{
	test_contin(TEST_BITS_8, TEST_BITS_8, TEST_LOC_MONO, TEST_LOC_MONO, false, true, 100);
	test_contin(TEST_BITS_8, TEST_BITS_8, TEST_LOC_L, TEST_LOC_L, false, true, 100);
	test_contin(TEST_BITS_8, TEST_BITS_16, TEST_LOC_L_R, TEST_LOC_L_R, false, true, 110);
	test_contin(TEST_BITS_8, TEST_BITS_16, TEST_LOC_L_R_C, TEST_LOC_L, false, true, 120);
	test_contin(TEST_BITS_16, TEST_BITS_16, TEST_LOC_L, TEST_LOC_L, false, true, 100);
	test_contin(TEST_BITS_16, TEST_BITS_16, TEST_LOC_R, TEST_LOC_R, false, true, 100);
	test_contin(TEST_BITS_16, TEST_BITS_16, TEST_LOC_L_R_C, TEST_LOC_L_R_C, false, true, 100);
	test_contin(TEST_BITS_16, TEST_BITS_32, TEST_LOC_L_R, TEST_LOC_L_R, false, true, 110);
	test_contin(TEST_BITS_24, TEST_BITS_32, TEST_LOC_L_R_C, TEST_LOC_L_R, false, true, 120);
	test_contin(TEST_BITS_32, TEST_BITS_32, TEST_LOC_L_R, TEST_LOC_L_R, false, true, 120);
}

/* Test continuous array formation for interleaved net_buffers */
ZTEST(suite_contin_array_chan, test_contin_int_filled_loop)
{
	test_contin(TEST_BITS_8, TEST_BITS_8, TEST_LOC_MONO, TEST_LOC_MONO, true, true, 100);
	test_contin(TEST_BITS_8, TEST_BITS_8, TEST_LOC_L, TEST_LOC_L, true, true, 100);
	test_contin(TEST_BITS_8, TEST_BITS_16, TEST_LOC_L_R, TEST_LOC_L_R, true, true, 110);
	test_contin(TEST_BITS_8, TEST_BITS_16, TEST_LOC_L_R_C, TEST_LOC_L, true, true, 120);
	test_contin(TEST_BITS_16, TEST_BITS_16, TEST_LOC_L, TEST_LOC_L, true, true, 100);
	test_contin(TEST_BITS_16, TEST_BITS_16, TEST_LOC_R, TEST_LOC_R, true, true, 100);
	test_contin(TEST_BITS_16, TEST_BITS_16, TEST_LOC_L_R_C, TEST_LOC_L_R_C, true, true, 100);
	test_contin(TEST_BITS_16, TEST_BITS_32, TEST_LOC_L_R, TEST_LOC_L_R, true, true, 110);
	test_contin(TEST_BITS_24, TEST_BITS_32, TEST_LOC_L_R_C, TEST_LOC_L_R, true, true, 120);
	test_contin(TEST_BITS_32, TEST_BITS_32, TEST_LOC_L_R, TEST_LOC_L_R, true, true, 120);
}
