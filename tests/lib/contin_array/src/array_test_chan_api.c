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
					  .bytes_per_location = sizeof(test_arr),
					  .interleaved = CONTIN_TEST_DEINTERLEAVED,
					  .locations = BT_AUDIO_LOCATION_FRONT_LEFT,
					  .ref_ts_us = 0,
					  .data_rx_ts_us = 0,
					  .bad_data = false};

NET_BUF_POOL_FIXED_DEFINE(pool_contin, CONTIN_TEST_POOL_CONTIN_NUM, CONTIN_TEST_POOL_CONTIN_SIZE,
			  (sizeof(struct audio_metadata)), NULL);
NET_BUF_POOL_FIXED_DEFINE(pool_finite, CONTIN_TEST_POOL_FINITE_NUM, sizeof(test_arr),
			  sizeof(struct audio_metadata), NULL);

/* Test parameter validation */
ZTEST(suite_contin_array_chan, test_contin_buf_api)
{
	int ret;
	struct net_buf *pcm_contin = NULL;
	uint8_t *pcm_finite = (uint8_t *)test_arr;
	struct audio_metadata *meta_contin;
	uint16_t finite_pos = 0;
	uint8_t locs_out = 0x00000011;

	pcm_contin = net_buf_alloc(&pool_contin, K_NO_WAIT);

	meta_contin = net_buf_user_data(pcm_contin);
	memcpy(meta_contin, &test_meta, sizeof(struct audio_metadata));

	/* Test for pcm_contin pointer NULL */
	net_buf_add(pcm_contin, pcm_contin->size);
	ret = contin_array_buf_create(NULL, pcm_finite, sizeof(test_arr), locs_out, &finite_pos);
	zassert_equal(ret, -ENXIO, "Failed to recognize NULL pointer: %d", ret);

	/* Test for pcm_finite buffer pointer NULL */
	ret = contin_array_buf_create(pcm_contin, NULL, sizeof(test_arr), locs_out, &finite_pos);
	zassert_equal(ret, -ENXIO, "Failed to recognize NULL pointer: %d", ret);

	/* Test for pcm_finite buffer pointer NULL */
	ret = contin_array_buf_create(pcm_contin, pcm_finite, sizeof(test_arr), locs_out, NULL);
	zassert_equal(ret, -ENXIO, "Failed to recognize NULL pointer: %d", ret);

	/* Test for pcm_finite buffer size 0 */
	ret = contin_array_buf_create(pcm_contin, pcm_finite, 0, locs_out, &finite_pos);
	zassert_equal(ret, -EPERM, "Failed to recognize buffer size is zero: %d", ret);

	/* Test for out of range finite_pos */
	finite_pos = sizeof(test_arr) + 1;
	ret = contin_array_buf_create(pcm_contin, pcm_finite, sizeof(test_arr), locs_out,
				      &finite_pos);
	zassert_equal(ret, -EPERM, "Failed to recognize out of range finite_pos: %d", ret);

	/* Test for locations zero (mono) */
	ret = contin_array_buf_create(pcm_contin, pcm_finite, sizeof(test_arr),
				      BT_AUDIO_LOCATION_MONO_AUDIO, &finite_pos);
	zassert_equal(ret, -EPERM, "Failed to recognize channel out of range: %d", ret);

	/* Test for no channel in locations */
	ret = contin_array_buf_create(pcm_contin, pcm_finite, sizeof(test_arr), 10, &finite_pos);
	zassert_equal(ret, -EPERM, "Failed to recognize channel out of range: %d", ret);

	/* Test for invalid locations in the continuous buffer */
	meta_contin->locations = BT_AUDIO_LOCATION_MONO_AUDIO;
	ret = contin_array_buf_create(pcm_contin, pcm_finite, sizeof(test_arr),
				      BT_AUDIO_LOCATION_FRONT_LEFT, &finite_pos);
	zassert_equal(ret, -EPERM, "Failed to recognize channel not present: %d", ret);

	/* Test for invalid locations in the continuous buffer */
	meta_contin->locations = BT_AUDIO_LOCATION_FRONT_RIGHT;
	ret = contin_array_buf_create(pcm_contin, pcm_finite, sizeof(test_arr),
				      BT_AUDIO_LOCATION_FRONT_LEFT, &finite_pos);
	zassert_equal(ret, -EPERM, "Failed to recognize channel not present: %d", ret);

	/* Test for pcm_contin size 0 */
	memcpy(meta_contin, &test_meta, sizeof(struct audio_metadata));
	pcm_contin->size = 0;
	ret = contin_array_buf_create(pcm_contin, pcm_finite, sizeof(test_arr), locs_out,
				      &finite_pos);
	zassert_equal(ret, -EPERM, "Failed to recognize data size of 0: %d", ret);

	net_buf_unref(pcm_contin);
}

/* Test parameter validation */
ZTEST(suite_contin_array_chan, test_contin_net_buf_api)
{
	int ret;
	struct net_buf *pcm_contin = NULL;
	struct net_buf *pcm_finite = NULL;
	struct audio_metadata *meta_contin;
	struct audio_metadata *meta_finite;
	uint16_t finite_pos = 0;
	uint32_t locs_out = BT_AUDIO_LOCATION_FRONT_LEFT;

	pcm_contin = net_buf_alloc(&pool_contin, K_NO_WAIT);
	pcm_finite = net_buf_alloc(&pool_finite, K_NO_WAIT);
	net_buf_add(pcm_contin, pcm_contin->size);
	net_buf_add(pcm_finite, pcm_finite->size);

	meta_contin = net_buf_user_data(pcm_contin);
	meta_finite = net_buf_user_data(pcm_finite);
	memcpy(meta_contin, &test_meta, sizeof(struct audio_metadata));
	memcpy(meta_finite, &test_meta, sizeof(struct audio_metadata));

	/* Test for pcm_finite pointer NULL */
	ret = contin_array_net_buf_create(pcm_contin, NULL, locs_out, &finite_pos);
	zassert_equal(ret, -ENXIO, "Failed to recognize NULL pointer: %d", ret);

	/* Test meta_contin sample rate */
	finite_pos = 0;
	meta_contin->sample_rate_hz = 8000;
	ret = contin_array_net_buf_create(pcm_contin, pcm_finite, locs_out, &finite_pos);
	zassert_equal(ret, -EINVAL, "Failed to detect sample rate difference: %d", ret);

	/* Test meta_finite sample rate */
	memcpy(meta_contin, &test_meta, sizeof(struct audio_metadata));
	meta_finite->sample_rate_hz = 8000;
	ret = contin_array_net_buf_create(pcm_contin, pcm_finite, locs_out, &finite_pos);
	zassert_equal(ret, -EINVAL, "Failed to detect sample rate difference: %d", ret);

	/* Test meta_contin bits_per_sample */
	pcm_contin->size = CONTIN_TEST_POOL_CONTIN_SIZE;
	meta_contin->bits_per_sample = 8;
	ret = contin_array_net_buf_create(pcm_contin, pcm_finite, locs_out, &finite_pos);
	zassert_equal(ret, -EINVAL, "Failed to detect bits per sample difference: %d", ret);

	/* Test meta_contin bits_per_sample */
	memcpy(meta_contin, &test_meta, sizeof(struct audio_metadata));
	meta_contin->bits_per_sample = 32;
	ret = contin_array_net_buf_create(pcm_contin, pcm_finite, locs_out, &finite_pos);
	zassert_equal(ret, -EINVAL, "Failed to detect bits per sample difference: %d", ret);

	/* Test meta_contin carried_bits_per_sample */
	memcpy(meta_contin, &test_meta, sizeof(struct audio_metadata));
	meta_contin->carried_bits_per_sample = 8;
	ret = contin_array_net_buf_create(pcm_contin, pcm_finite, locs_out, &finite_pos);
	zassert_equal(ret, -EINVAL, "Failed to detect carrier bits per sample difference: %d", ret);

	/* Test meta_contin carried_bits_per_sample */
	memcpy(meta_contin, &test_meta, sizeof(struct audio_metadata));
	meta_contin->carried_bits_per_sample = 32;
	ret = contin_array_net_buf_create(pcm_contin, pcm_finite, locs_out, &finite_pos);
	zassert_equal(ret, -EINVAL, "Failed to detect carrier bits per sample difference: %d", ret);

	/* Test meta_finite bits_per_sample */
	memcpy(meta_contin, &test_meta, sizeof(struct audio_metadata));
	meta_finite->bits_per_sample = 8;
	ret = contin_array_net_buf_create(pcm_contin, pcm_finite, locs_out, &finite_pos);
	zassert_equal(ret, -EINVAL, "Failed to detect bits per sample difference: %d", ret);

	/* Test meta_finite bits_per_sample */
	memcpy(meta_finite, &test_meta, sizeof(struct audio_metadata));
	meta_finite->bits_per_sample = 32;
	ret = contin_array_net_buf_create(pcm_contin, pcm_finite, locs_out, &finite_pos);
	zassert_equal(ret, -EINVAL, "Failed to detect bits per sample difference: %d", ret);

	/* Test meta_finite carried_bits_per_sample */
	memcpy(meta_finite, &test_meta, sizeof(struct audio_metadata));
	meta_finite->carried_bits_per_sample = 8;
	ret = contin_array_net_buf_create(pcm_contin, pcm_finite, locs_out, &finite_pos);
	zassert_equal(ret, -EINVAL, "Failed to detect carrier bits per sample difference: %d", ret);

	/* Test meta_finite carried_bits_per_sample */
	memcpy(meta_finite, &test_meta, sizeof(struct audio_metadata));
	meta_finite->carried_bits_per_sample = 32;
	ret = contin_array_net_buf_create(pcm_contin, pcm_finite, locs_out, &finite_pos);
	zassert_equal(ret, -EINVAL, "Failed to detect carrier bits per sample difference: %d", ret);

	/* Test for pcm_finite len 0 */
	memcpy(meta_finite, &test_meta, sizeof(struct audio_metadata));
	pcm_finite->len = 0;
	ret = contin_array_net_buf_create(pcm_contin, pcm_finite, locs_out, &finite_pos);
	zassert_equal(ret, -EPERM, "Failed to recognize data size of 0: %d", ret);

	net_buf_unref(pcm_contin);
	net_buf_unref(pcm_finite);
}
