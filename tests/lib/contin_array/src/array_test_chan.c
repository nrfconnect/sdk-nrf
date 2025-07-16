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
					  .bytes_per_location = ARRAY_SIZE(test_arr),
					  .interleaved = CONTIN_TEST_DEINTERLEAVED,
					  .locations = BT_AUDIO_LOCATION_FRONT_LEFT,
					  .ref_ts_us = 0,
					  .data_rx_ts_us = 0,
					  .bad_data = false};

NET_BUF_POOL_FIXED_DEFINE(pool_contin, CONTIN_TEST_POOL_CONTIN_NUM, CONTIN_TEST_POOL_CONTIN_SIZE,
			  (sizeof(struct audio_metadata)), NULL);
NET_BUF_POOL_FIXED_DEFINE(pool_finite, CONTIN_TEST_POOL_FINITE_NUM, ARRAY_SIZE(test_arr),
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
	uint8_t const *pcm_finite_buf = test_arr;
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
		num_ch = audio_metadata_num_ch_get(meta_contin);
		meta_contin->bytes_per_location = pcm_contin->size / num_ch;

		if (filled) {
			/* Fill all channels in the net_nuf with a known value for testing that the
			 * function only writes to the required location(s) and no others.
			 */
			memset(pcm_contin->data, CONTIN_TEST_BYTE, pcm_contin->size);
			net_buf_add(pcm_contin, meta_contin->bytes_per_location * num_ch);
		} else {
			memset(pcm_contin->data, 0, pcm_contin->size);
		}

		ret = contin_array_buf_create(pcm_contin, pcm_finite_buf, ARRAY_SIZE(test_arr),
					      locs_out, &finite_pos);
		zassert_equal(ret, 0, "contin_array_buf_create did not return zero: %d", ret);

		validate_contin_array(pcm_contin, pcm_finite_buf, locs_out, finite_start,
				      finite_pos, interleaved, filled);

		net_buf_unref(pcm_contin);
	}

	/* Test with two net_bufs */
	finite_pos = 0;

	pcm_finite = net_buf_alloc(&pool_finite, K_NO_WAIT);
	net_buf_add_mem(pcm_finite, (uint8_t *)test_arr, ARRAY_SIZE(test_arr));

	meta_finite = net_buf_user_data(pcm_finite);
	memcpy(meta_finite, &test_meta, sizeof(struct audio_metadata));
	meta_finite->bits_per_sample = bits_per_samp;
	meta_finite->carried_bits_per_sample = carrier_bits;
	meta_finite->interleaved = interleaved;
	meta_finite->bytes_per_location = ARRAY_SIZE(test_arr);
	meta_finite->locations = BT_AUDIO_LOCATION_MONO_AUDIO;

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
		num_ch = audio_metadata_num_ch_get(meta_contin);
		meta_contin->bytes_per_location = pcm_contin->size / num_ch;

		if (filled) {
			/* Fill all channels in the net_nuf with a known value for testing that the
			 * function only writes to the required location(s) and no others.
			 */
			memset(pcm_contin->data, CONTIN_TEST_BYTE, pcm_contin->size);
			net_buf_add(pcm_contin, meta_contin->bytes_per_location * num_ch);
		} else {
			memset(pcm_contin->data, 0, pcm_contin->size);
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
	test_contin(TEST_BITS_8, TEST_BITS_8, BT_AUDIO_LOCATION_MONO_AUDIO,
		    BT_AUDIO_LOCATION_MONO_AUDIO, CONTIN_TEST_DEINTERLEAVED, CONTIN_TEST_NOT_FILLED,
		    100);
	test_contin(TEST_BITS_8, TEST_BITS_8, BT_AUDIO_LOCATION_FRONT_LEFT,
		    BT_AUDIO_LOCATION_FRONT_LEFT, CONTIN_TEST_DEINTERLEAVED, CONTIN_TEST_NOT_FILLED,
		    100);
	test_contin(TEST_BITS_8, TEST_BITS_8, BT_AUDIO_LOCATION_FRONT_RIGHT,
		    BT_AUDIO_LOCATION_FRONT_RIGHT, CONTIN_TEST_DEINTERLEAVED,
		    CONTIN_TEST_NOT_FILLED, 100);
	test_contin(TEST_BITS_8, TEST_BITS_16,
		    BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT,
		    BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT,
		    CONTIN_TEST_DEINTERLEAVED, CONTIN_TEST_NOT_FILLED, 110);
	test_contin(TEST_BITS_8, TEST_BITS_16,
		    BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT |
			    BT_AUDIO_LOCATION_FRONT_CENTER,
		    BT_AUDIO_LOCATION_FRONT_LEFT, CONTIN_TEST_DEINTERLEAVED, CONTIN_TEST_NOT_FILLED,
		    120);
	test_contin(TEST_BITS_16, TEST_BITS_16, BT_AUDIO_LOCATION_FRONT_LEFT,
		    BT_AUDIO_LOCATION_FRONT_LEFT, CONTIN_TEST_DEINTERLEAVED, CONTIN_TEST_NOT_FILLED,
		    100);
	test_contin(TEST_BITS_16, TEST_BITS_16, BT_AUDIO_LOCATION_FRONT_RIGHT,
		    BT_AUDIO_LOCATION_FRONT_RIGHT, CONTIN_TEST_DEINTERLEAVED,
		    CONTIN_TEST_NOT_FILLED, 100);
	test_contin(TEST_BITS_16, TEST_BITS_16,
		    BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT |
			    BT_AUDIO_LOCATION_FRONT_CENTER,
		    BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT |
			    BT_AUDIO_LOCATION_FRONT_CENTER,
		    CONTIN_TEST_DEINTERLEAVED, CONTIN_TEST_NOT_FILLED, 100);
	test_contin(TEST_BITS_16, TEST_BITS_32,
		    BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT,
		    BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT,
		    CONTIN_TEST_DEINTERLEAVED, CONTIN_TEST_NOT_FILLED, 110);
	test_contin(TEST_BITS_24, TEST_BITS_32,
		    BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT |
			    BT_AUDIO_LOCATION_FRONT_CENTER,
		    BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT,
		    CONTIN_TEST_DEINTERLEAVED, CONTIN_TEST_NOT_FILLED, 120);
	test_contin(TEST_BITS_32, TEST_BITS_32,
		    BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT,
		    BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT,
		    CONTIN_TEST_DEINTERLEAVED, CONTIN_TEST_NOT_FILLED, 120);
}

/* Test continuous array formation for interleaved net_buffers */
ZTEST(suite_contin_array_chan, test_contin_int_empty_loop)
{
	test_contin(TEST_BITS_8, TEST_BITS_8, BT_AUDIO_LOCATION_MONO_AUDIO,
		    BT_AUDIO_LOCATION_MONO_AUDIO, CONTIN_TEST_INTERLEAVED, CONTIN_TEST_NOT_FILLED,
		    100);
	test_contin(TEST_BITS_8, TEST_BITS_8, BT_AUDIO_LOCATION_FRONT_LEFT,
		    BT_AUDIO_LOCATION_FRONT_LEFT, CONTIN_TEST_INTERLEAVED, CONTIN_TEST_NOT_FILLED,
		    100);
	test_contin(TEST_BITS_8, TEST_BITS_8, BT_AUDIO_LOCATION_FRONT_RIGHT,
		    BT_AUDIO_LOCATION_FRONT_RIGHT, CONTIN_TEST_INTERLEAVED, CONTIN_TEST_NOT_FILLED,
		    100);
	test_contin(TEST_BITS_8, TEST_BITS_16,
		    BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT,
		    BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT,
		    CONTIN_TEST_INTERLEAVED, CONTIN_TEST_NOT_FILLED, 110);
	test_contin(TEST_BITS_8, TEST_BITS_16,
		    BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT |
			    BT_AUDIO_LOCATION_FRONT_CENTER,
		    BT_AUDIO_LOCATION_FRONT_LEFT, CONTIN_TEST_INTERLEAVED, CONTIN_TEST_NOT_FILLED,
		    120);
	test_contin(TEST_BITS_16, TEST_BITS_16, BT_AUDIO_LOCATION_FRONT_LEFT,
		    BT_AUDIO_LOCATION_FRONT_LEFT, CONTIN_TEST_INTERLEAVED, CONTIN_TEST_NOT_FILLED,
		    100);
	test_contin(TEST_BITS_16, TEST_BITS_16, BT_AUDIO_LOCATION_FRONT_RIGHT,
		    BT_AUDIO_LOCATION_FRONT_RIGHT, CONTIN_TEST_INTERLEAVED, CONTIN_TEST_NOT_FILLED,
		    100);
	test_contin(TEST_BITS_16, TEST_BITS_16,
		    BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT |
			    BT_AUDIO_LOCATION_FRONT_CENTER,
		    BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT |
			    BT_AUDIO_LOCATION_FRONT_CENTER,
		    CONTIN_TEST_INTERLEAVED, CONTIN_TEST_NOT_FILLED, 100);
	test_contin(TEST_BITS_16, TEST_BITS_32,
		    BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT,
		    BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT,
		    CONTIN_TEST_INTERLEAVED, CONTIN_TEST_NOT_FILLED, 110);
	test_contin(TEST_BITS_24, TEST_BITS_32,
		    BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT |
			    BT_AUDIO_LOCATION_FRONT_CENTER,
		    BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT,
		    CONTIN_TEST_INTERLEAVED, CONTIN_TEST_NOT_FILLED, 120);
	test_contin(TEST_BITS_32, TEST_BITS_32,
		    BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT,
		    BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT,
		    CONTIN_TEST_INTERLEAVED, CONTIN_TEST_NOT_FILLED, 120);
}

/* Test continuous array formation for de-interleaved net_buffers */
ZTEST(suite_contin_array_chan, test_contin_deint_filled_loop)
{
	test_contin(TEST_BITS_8, TEST_BITS_8, BT_AUDIO_LOCATION_MONO_AUDIO,
		    BT_AUDIO_LOCATION_MONO_AUDIO, CONTIN_TEST_DEINTERLEAVED, CONTIN_TEST_FILLED,
		    100);
	test_contin(TEST_BITS_8, TEST_BITS_8, BT_AUDIO_LOCATION_FRONT_LEFT,
		    BT_AUDIO_LOCATION_FRONT_LEFT, CONTIN_TEST_DEINTERLEAVED, CONTIN_TEST_FILLED,
		    100);
	test_contin(TEST_BITS_8, TEST_BITS_16,
		    BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT,
		    BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT,
		    CONTIN_TEST_DEINTERLEAVED, CONTIN_TEST_FILLED, 110);
	test_contin(TEST_BITS_8, TEST_BITS_16,
		    BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT |
			    BT_AUDIO_LOCATION_FRONT_CENTER,
		    BT_AUDIO_LOCATION_FRONT_LEFT, CONTIN_TEST_DEINTERLEAVED, CONTIN_TEST_FILLED,
		    120);
	test_contin(TEST_BITS_16, TEST_BITS_16, BT_AUDIO_LOCATION_FRONT_LEFT,
		    BT_AUDIO_LOCATION_FRONT_LEFT, CONTIN_TEST_DEINTERLEAVED, CONTIN_TEST_FILLED,
		    100);
	test_contin(TEST_BITS_16, TEST_BITS_16, BT_AUDIO_LOCATION_FRONT_RIGHT,
		    BT_AUDIO_LOCATION_FRONT_RIGHT, CONTIN_TEST_DEINTERLEAVED, CONTIN_TEST_FILLED,
		    100);
	test_contin(TEST_BITS_16, TEST_BITS_16,
		    BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT |
			    BT_AUDIO_LOCATION_FRONT_CENTER,
		    BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT |
			    BT_AUDIO_LOCATION_FRONT_CENTER,
		    CONTIN_TEST_DEINTERLEAVED, CONTIN_TEST_FILLED, 100);
	test_contin(TEST_BITS_16, TEST_BITS_32,
		    BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT,
		    BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT,
		    CONTIN_TEST_DEINTERLEAVED, CONTIN_TEST_FILLED, 110);
	test_contin(TEST_BITS_24, TEST_BITS_32,
		    BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT |
			    BT_AUDIO_LOCATION_FRONT_CENTER,
		    BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT,
		    CONTIN_TEST_DEINTERLEAVED, CONTIN_TEST_FILLED, 120);
	test_contin(TEST_BITS_32, TEST_BITS_32,
		    BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT,
		    BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT,
		    CONTIN_TEST_DEINTERLEAVED, CONTIN_TEST_FILLED, 120);
}

/* Test continuous array formation for interleaved net_buffers */
ZTEST(suite_contin_array_chan, test_contin_int_filled_loop)
{
	test_contin(TEST_BITS_8, TEST_BITS_8, BT_AUDIO_LOCATION_MONO_AUDIO,
		    BT_AUDIO_LOCATION_MONO_AUDIO, CONTIN_TEST_INTERLEAVED, CONTIN_TEST_FILLED, 100);
	test_contin(TEST_BITS_8, TEST_BITS_8, BT_AUDIO_LOCATION_FRONT_LEFT,
		    BT_AUDIO_LOCATION_FRONT_LEFT, CONTIN_TEST_INTERLEAVED, CONTIN_TEST_FILLED, 100);
	test_contin(TEST_BITS_8, TEST_BITS_16,
		    BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT,
		    BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT,
		    CONTIN_TEST_INTERLEAVED, CONTIN_TEST_FILLED, 110);
	test_contin(TEST_BITS_8, TEST_BITS_16,
		    BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT |
			    BT_AUDIO_LOCATION_FRONT_CENTER,
		    BT_AUDIO_LOCATION_FRONT_LEFT, CONTIN_TEST_INTERLEAVED, CONTIN_TEST_FILLED, 120);
	test_contin(TEST_BITS_16, TEST_BITS_16, BT_AUDIO_LOCATION_FRONT_LEFT,
		    BT_AUDIO_LOCATION_FRONT_LEFT, CONTIN_TEST_INTERLEAVED, CONTIN_TEST_FILLED, 100);
	test_contin(TEST_BITS_16, TEST_BITS_16, BT_AUDIO_LOCATION_FRONT_RIGHT,
		    BT_AUDIO_LOCATION_FRONT_RIGHT, CONTIN_TEST_INTERLEAVED, CONTIN_TEST_FILLED,
		    100);
	test_contin(TEST_BITS_16, TEST_BITS_16,
		    BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT |
			    BT_AUDIO_LOCATION_FRONT_CENTER,
		    BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT |
			    BT_AUDIO_LOCATION_FRONT_CENTER,
		    CONTIN_TEST_INTERLEAVED, CONTIN_TEST_FILLED, 100);
	test_contin(TEST_BITS_16, TEST_BITS_32,
		    BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT,
		    BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT,
		    CONTIN_TEST_INTERLEAVED, CONTIN_TEST_FILLED, 110);
	test_contin(TEST_BITS_24, TEST_BITS_32,
		    BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT |
			    BT_AUDIO_LOCATION_FRONT_CENTER,
		    BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT,
		    CONTIN_TEST_INTERLEAVED, CONTIN_TEST_FILLED, 120);
	test_contin(TEST_BITS_32, TEST_BITS_32,
		    BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT,
		    BT_AUDIO_LOCATION_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT,
		    CONTIN_TEST_INTERLEAVED, CONTIN_TEST_FILLED, 120);
}
