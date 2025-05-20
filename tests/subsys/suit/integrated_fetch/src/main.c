/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <stdint.h>
#include <suit_sink.h>
#include <suit_memptr_sink.h>
#include <suit_memptr_storage.h>
#include <suit_memptr_streamer.h>

#define TEST_DATA_SIZE 64

static uint8_t test_data[] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
			      16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
			      32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
			      48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63};

ZTEST_SUITE(memptr_streamer_tests, NULL, NULL, NULL, NULL, NULL);

ZTEST(memptr_streamer_tests, test_memptr_streamer_OK)
{
	struct stream_sink memptr_sink;
	memptr_storage_handle_t handle = NULL;

	int err = suit_memptr_storage_get(&handle);

	zassert_equal(err, SUIT_PLAT_SUCCESS, "suit_memptr_storage_get failed - error %i", err);

	err = suit_memptr_sink_get(&memptr_sink, handle);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "suit_memptr_sink_get failed - error %i", err);

	err = suit_memptr_streamer_stream(test_data, TEST_DATA_SIZE, &memptr_sink);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "memptr_streamer failed - error %i", err);

	err = suit_memptr_storage_release(handle);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "memptr_storage.release failed - error %i", err);
}

ZTEST(memptr_streamer_tests, test_memptr_streamer_NOK)
{
	struct stream_sink memptr_sink;
	memptr_storage_handle_t handle = NULL;

	int err = suit_memptr_storage_get(&handle);

	zassert_equal(err, SUIT_PLAT_SUCCESS, "suit_memptr_storage_get failed - error %i", err);

	err = suit_memptr_sink_get(&memptr_sink, handle);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "suit_memptr_sink_get failed - error %i", err);

	err = suit_memptr_streamer_stream(NULL, TEST_DATA_SIZE, &memptr_sink);
	zassert_not_equal(err, SUIT_PLAT_SUCCESS,
			  "memptr_streamer should have failed - payload == NULL");

	err = suit_memptr_streamer_stream(test_data, 0, &memptr_sink);
	zassert_not_equal(err, SUIT_PLAT_SUCCESS, "memptr_streamer should have failed - size == 0");

	err = suit_memptr_streamer_stream(test_data, TEST_DATA_SIZE, NULL);
	zassert_not_equal(err, SUIT_PLAT_SUCCESS,
			  "memptr_streamer should have failed - sink == NULL");

	err = suit_memptr_storage_release(handle);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "memptr_storage.release failed - error %i", err);
}
