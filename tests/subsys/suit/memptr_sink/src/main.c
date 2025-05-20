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

static uint8_t test_data[] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
			      16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
			      32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
			      48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63};

ZTEST_SUITE(memptr_sink_tests, NULL, NULL, NULL, NULL, NULL);

ZTEST(memptr_sink_tests, test_suit_memptr_sink_get_OK)
{
	struct stream_sink memptr_sink;
	memptr_storage_handle_t handle = NULL;

	int err = suit_memptr_storage_get(&handle);

	zassert_equal(err, SUIT_PLAT_SUCCESS, "suit_memptr_storage_get failed - error %i", err);

	err = suit_memptr_sink_get(&memptr_sink, handle);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "suit_memptr_sink_get failed - error %i", err);

	err = suit_memptr_storage_release(handle);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "memptr_storage.release failed - error %i", err);
}

ZTEST(memptr_sink_tests, test_suit_memptr_sink_get_NOK)
{
	struct stream_sink memptr_sink;
	memptr_storage_handle_t handle = NULL;

	int err = suit_memptr_storage_get(&handle);

	zassert_equal(err, SUIT_PLAT_SUCCESS, "suit_memptr_storage_get failed - error %i", err);

	err = suit_memptr_sink_get(NULL, handle);
	zassert_not_equal(err, SUIT_PLAT_SUCCESS,
			  "suit_memptr_sink_get should have failed - memptr_sink == NULL");

	err = suit_memptr_sink_get(&memptr_sink, NULL);
	zassert_not_equal(err, SUIT_PLAT_SUCCESS,
			  "suit_memptr_sink_get should have failed - storage == NULL");

	err = suit_memptr_storage_release(handle);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "memptr_storage.release failed - error %i", err);
}

ZTEST(memptr_sink_tests, test_memptr_sink_write_OK)
{
	struct stream_sink memptr_sink;
	const uint8_t *payload_ptr;
	size_t payload_size = 0;
	memptr_storage_handle_t handle = NULL;

	int err = suit_memptr_storage_get(&handle);

	zassert_equal(err, SUIT_PLAT_SUCCESS, "suit_memptr_storage_get failed - error %i", err);

	err = suit_memptr_sink_get(&memptr_sink, handle);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "suit_memptr_sink_get failed - error %i", err);

	err = memptr_sink.write(memptr_sink.ctx, test_data, sizeof(test_data));
	zassert_equal(err, SUIT_PLAT_SUCCESS, "memptr_storage.save failed - error %i", err);

	err = suit_memptr_storage_ptr_get(handle, &payload_ptr, &payload_size);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "memptr_storage.get failed - error %i", err);
	zassert_equal(payload_ptr, test_data,
		      "memptr_storage.get, payload_ptr and data address mismatch");
	zassert_equal(payload_size, sizeof(test_data),
		      "memptr_storage.get, payload_size and data size mismatch");

	err = suit_memptr_storage_release(handle);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "memptr_storage.release failed - error %i", err);
}

ZTEST(memptr_sink_tests, test_memptr_sink_write_NOK)
{
	struct stream_sink memptr_sink;
	memptr_storage_handle_t handle = NULL;

	int err = suit_memptr_storage_get(&handle);

	zassert_equal(err, SUIT_PLAT_SUCCESS, "suit_memptr_storage_get failed - error %i", err);

	err = suit_memptr_sink_get(&memptr_sink, handle);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "suit_memptr_sink_get failed - error %i", err);

	err = memptr_sink.write(NULL, test_data, sizeof(test_data));
	zassert_not_equal(err, SUIT_PLAT_SUCCESS,
			  "memptr_sink.write should have failed - ctx == NULL");

	err = memptr_sink.write(memptr_sink.ctx, NULL, sizeof(test_data));
	zassert_not_equal(err, SUIT_PLAT_SUCCESS,
			  "memptr_sink.write should have failed - payload_ptr == NULL");

	err = memptr_sink.write(memptr_sink.ctx, test_data, 0);
	zassert_not_equal(err, SUIT_PLAT_SUCCESS,
			  "memptr_sink.write should have failed - payload_size == 0");

	err = suit_memptr_storage_release(handle);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "memptr_sink.release failed - error %i", err);

	err = memptr_sink.write(memptr_sink.ctx, test_data, sizeof(test_data));
	zassert_not_equal(err, SUIT_PLAT_SUCCESS,
			  "memptr_sink.write should have failed - storage released");
}

ZTEST(memptr_sink_tests, test_memptr_sink_used_storage_OK)
{
	struct stream_sink memptr_sink;
	memptr_storage_handle_t handle = NULL;
	size_t size = 0;

	int err = suit_memptr_storage_get(&handle);

	zassert_equal(err, SUIT_PLAT_SUCCESS, "suit_memptr_storage_get failed - error %i", err);

	err = suit_memptr_sink_get(&memptr_sink, handle);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "suit_memptr_sink_get failed - error %i", err);

	err = memptr_sink.used_storage(memptr_sink.ctx, &size);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "memptr_sink.used_storage failed - error %i", err);
	zassert_equal(size, 0, "used_storage should return 0 if nothing was written to storage");

	err = memptr_sink.write(memptr_sink.ctx, test_data, sizeof(test_data));
	zassert_equal(err, SUIT_PLAT_SUCCESS, "memptr_storage.save failed - error %i", err);

	err = memptr_sink.used_storage(memptr_sink.ctx, &size);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "memptr_sink.used_storage failed - error %i", err);
	zassert_equal(size, sizeof(test_data),
		      "used_storage should return 1 if something was written to storage");

	err = suit_memptr_storage_release(handle);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "memptr_sink.release failed - error %i", err);
}

ZTEST(memptr_sink_tests, test_memptr_sink_used_storage_NOK)
{
	struct stream_sink memptr_sink;
	memptr_storage_handle_t handle = NULL;
	size_t size = 0;

	int err = suit_memptr_storage_get(&handle);

	zassert_equal(err, SUIT_PLAT_SUCCESS, "suit_memptr_storage_get failed - error %i", err);

	err = suit_memptr_sink_get(&memptr_sink, handle);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "suit_memptr_sink_get failed - error %i", err);

	err = memptr_sink.used_storage(NULL, &size);
	zassert_not_equal(err, SUIT_PLAT_SUCCESS,
			  "memptr_sink.used_storage should have failed - ctx == NULL");

	err = memptr_sink.used_storage(memptr_sink.ctx, NULL);
	zassert_not_equal(err, SUIT_PLAT_SUCCESS,
			  "memptr_sink.used_storage should have failed - size == NULL");

	err = suit_memptr_storage_release(handle);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "memptr_sink.release failed - error %i", err);

	err = memptr_sink.used_storage(memptr_sink.ctx, &size);
	zassert_not_equal(err, SUIT_PLAT_SUCCESS,
			  "memptr_sink.used_storage should have failed - storage released");
}
