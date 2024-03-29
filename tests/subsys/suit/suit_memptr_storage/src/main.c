/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <stdint.h>
#include <suit_memptr_storage.h>

#define TEST_DATA_SIZE 64

static uint8_t test_data[TEST_DATA_SIZE] = {
	0,  1,	2,  3,	4,  5,	6,  7,	8,  9,	10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
	22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
	44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63};

ZTEST_SUITE(suit_memptr_storage_tests, NULL, NULL, NULL, NULL, NULL);

ZTEST(suit_memptr_storage_tests, test_suit_memptr_storage_get_OK)
{
	memptr_storage_handle_t handle = NULL;

	int err = suit_memptr_storage_get(&handle);

	zassert_equal(err, SUIT_PLAT_SUCCESS, "get_new_memptr_record failed - error %i", err);

	err = suit_memptr_storage_release(handle);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "suit_memptr_storage_release failed - error %i", err);
}

ZTEST(suit_memptr_storage_tests, test_suit_memptr_storage_get_NOK)
{
	memptr_storage_handle_t handles[CONFIG_SUIT_MAX_NUMBER_OF_INTEGRATED_PAYLOADS + 1];
	int err = SUIT_PLAT_SUCCESS;

	for (size_t i = 0; i < CONFIG_SUIT_MAX_NUMBER_OF_INTEGRATED_PAYLOADS; i++) {
		err = suit_memptr_storage_get(&handles[i]);
		zassert_equal(err, SUIT_PLAT_SUCCESS, "suit_memptr_storage_get failed - error %i",
			      err);
	}

	err = suit_memptr_storage_get(NULL);
	zassert_not_equal(err, SUIT_PLAT_SUCCESS,
			  "suit_memptr_storage_get should have failed - handle == NULL");

	err = suit_memptr_storage_get(&handles[CONFIG_SUIT_MAX_NUMBER_OF_INTEGRATED_PAYLOADS]);
	zassert_not_equal(err, SUIT_PLAT_SUCCESS,
			  "suit_memptr_storage_get %u should have failed - max number or memptr "
			  "records reached",
			  CONFIG_SUIT_MAX_NUMBER_OF_INTEGRATED_PAYLOADS);

	for (size_t i = 0; i < CONFIG_SUIT_MAX_NUMBER_OF_INTEGRATED_PAYLOADS; i++) {
		err = suit_memptr_storage_release(handles[i]);
		zassert_equal(err, SUIT_PLAT_SUCCESS,
			      "memptr_storage[%u].release failed - error %i", i, err);
	}
}

ZTEST(suit_memptr_storage_tests, test_memptr_storage_release_NOK)
{
	memptr_storage_handle_t handle = NULL;

	int err = suit_memptr_storage_get(&handle);

	zassert_equal(err, SUIT_PLAT_SUCCESS, "suit_memptr_storage_get failed - error %i", err);

	err = suit_memptr_storage_release(NULL);
	zassert_not_equal(err, SUIT_PLAT_SUCCESS,
			  "suit_memptr_storage_release should have failed - ctx == NULL");

	err = suit_memptr_storage_release(handle);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "suit_memptr_storage_release failed - error %i", err);
}

ZTEST(suit_memptr_storage_tests, test_memptr_storage_save_OK)
{
	memptr_storage_handle_t handle = NULL;

	int err = suit_memptr_storage_get(&handle);

	zassert_equal(err, SUIT_PLAT_SUCCESS, "suit_memptr_storage_get failed - error %i", err);

	err = suit_memptr_storage_ptr_store(handle, test_data, TEST_DATA_SIZE);
	zassert_equal(err, SUIT_PLAT_SUCCESS,
		      "memptr_suit_memptr_storage_ptr_store failed - error %i", err);

	err = suit_memptr_storage_release(handle);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "suit_memptr_storage_release failed - error %i", err);
}

ZTEST(suit_memptr_storage_tests, test_memptr_storage_save_NOK)
{
	memptr_storage_handle_t handle = NULL;

	int err = suit_memptr_storage_get(&handle);

	zassert_equal(err, SUIT_PLAT_SUCCESS, "suit_memptr_storage_get failed - error %i", err);

	err = suit_memptr_storage_ptr_store(NULL, test_data, TEST_DATA_SIZE);
	zassert_not_equal(err, SUIT_PLAT_SUCCESS,
			  "memptr_suit_memptr_storage_ptr_store should have failed - ctx == NULL");

	err = suit_memptr_storage_release(handle);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "suit_memptr_storage_release failed - error %i", err);

	err = suit_memptr_storage_ptr_store(handle, test_data, TEST_DATA_SIZE);
	zassert_not_equal(
		err, SUIT_PLAT_SUCCESS,
		"memptr_suit_memptr_storage_ptr_store should have failed - unallocated record");
}

ZTEST(suit_memptr_storage_tests, test_memptr_storage_get_OK)
{
	memptr_storage_handle_t handle = NULL;
	const uint8_t *payload_ptr;
	size_t payload_size;

	int err = suit_memptr_storage_get(&handle);

	zassert_equal(err, SUIT_PLAT_SUCCESS, "suit_memptr_storage_get failed - error %i", err);

	err = suit_memptr_storage_ptr_store(handle, test_data, TEST_DATA_SIZE);
	zassert_equal(err, SUIT_PLAT_SUCCESS,
		      "memptr_suit_memptr_storage_ptr_store failed - error %i", err);

	err = suit_memptr_storage_ptr_get(handle, &payload_ptr, &payload_size);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "suit_memptr_storage_ptr_get failed - error %i", err);
	zassert_equal(payload_ptr, test_data,
		      "suit_memptr_storage_ptr_get, payload_ptr and data address mismatch");
	zassert_equal(payload_size, TEST_DATA_SIZE,
		      "suit_memptr_storage_ptr_get, payload_size and data size mismatch");

	err = suit_memptr_storage_release(handle);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "suit_memptr_storage_release failed - error %i", err);
}

ZTEST(suit_memptr_storage_tests, test_memptr_storage_get_NOK)
{
	memptr_storage_handle_t handle = NULL;
	const uint8_t *payload_ptr;
	size_t payload_size;

	int err = suit_memptr_storage_get(&handle);

	zassert_equal(err, SUIT_PLAT_SUCCESS, "suit_memptr_storage_get failed - error %i", err);

	err = suit_memptr_storage_ptr_store(handle, test_data, TEST_DATA_SIZE);
	zassert_equal(err, SUIT_PLAT_SUCCESS,
		      "memptr_suit_memptr_storage_ptr_store failed - error %i", err);

	err = suit_memptr_storage_ptr_get(NULL, &payload_ptr, &payload_size);
	zassert_not_equal(err, SUIT_PLAT_SUCCESS,
			  "suit_memptr_storage_ptr_get should have failed - ctx == NULL");

	err = suit_memptr_storage_ptr_get(handle, NULL, &payload_size);
	zassert_not_equal(err, SUIT_PLAT_SUCCESS,
			  "suit_memptr_storage_ptr_get should have failed - *payload_ptr == NULL");

	err = suit_memptr_storage_ptr_get(handle, &payload_ptr, NULL);
	zassert_not_equal(err, SUIT_PLAT_SUCCESS,
			  "suit_memptr_storage_ptr_get should have failed - payload_size == NULL");

	err = suit_memptr_storage_release(handle);
	zassert_equal(err, SUIT_PLAT_SUCCESS, "suit_memptr_storage_release failed - error %i", err);

	err = suit_memptr_storage_ptr_get(handle, &payload_ptr, &payload_size);
	zassert_not_equal(err, SUIT_PLAT_SUCCESS,
			  "suit_memptr_storage_ptr_get should have failed - unallocated record");
}
