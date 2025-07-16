/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <errno.h>
#include <zephyr/tc_util.h>
#include <contin_array.h>
#include "array_test_data.h"

ZTEST(suite_contin_array, test_simp_arr_loop)
{
	const uint32_t NUM_ITERATIONS = 200;
	const size_t CONTIN_ARR_SIZE = 97; /* Test with random "uneven" value */
	const uint32_t CONTIN_LAST_VAL_IDX = (CONTIN_ARR_SIZE - 1);
	const size_t const_arr_size = ARRAY_SIZE(test_arr);
	char contin_arr[CONTIN_ARR_SIZE];
	int ret;

	uint8_t contin_first_val;
	uint8_t contin_last_val;
	uint32_t finite_pos = 0;

	for (int i = 0; i < NUM_ITERATIONS; i++) {
		contin_first_val = test_arr[finite_pos];

		ret = contin_array_create(contin_arr, CONTIN_ARR_SIZE, test_arr, const_arr_size,
					  &finite_pos);
		zassert_equal(ret, 0, "contin_array_create did not return zero");

		if (finite_pos) {
			contin_last_val = test_arr[finite_pos - 1];
		} else {
			contin_last_val = test_arr[const_arr_size - 1];
		}

		zassert_equal(contin_arr[0], contin_first_val, "First value is not identical");
		zassert_equal(contin_arr[CONTIN_LAST_VAL_IDX], contin_last_val,
			      "Last value is not identical");
	}
}

/* Test with const array size being shorter than contin array size */
ZTEST(suite_contin_array, test_simp_arr_loop_short)
{
	int ret;
	const uint32_t NUM_ITERATIONS = 200;
	const size_t CONTIN_ARR_SIZE = 97; /* Test with random "uneven" value */
	const uint32_t CONTIN_LAST_VAL_IDX = (CONTIN_ARR_SIZE - 1);
	/* Test with random "uneven" value, shorter than CONTIN_ARR_SIZE */
	const size_t const_arr_size = 44;
	char contin_arr[CONTIN_ARR_SIZE];
	uint8_t contin_first_val;
	uint8_t contin_last_val;
	uint32_t finite_pos = 0;

	for (int i = 0; i < NUM_ITERATIONS; i++) {
		if (finite_pos > (const_arr_size - 1)) {
			contin_first_val = test_arr[0];
		} else {
			contin_first_val = test_arr[finite_pos];
		}

		ret = contin_array_create(contin_arr, CONTIN_ARR_SIZE, test_arr, const_arr_size,
					  &finite_pos);
		zassert_equal(ret, 0, "Contin_array_create did not return zero");

		if (finite_pos) {
			contin_last_val = test_arr[finite_pos - 1];
		} else {
			contin_last_val = test_arr[const_arr_size - 1];
		}

		zassert_equal(contin_arr[0], contin_first_val,
			      "%d: First value is not identical: %d vs %d (%d %d)", i,
			      contin_arr[0], contin_first_val, finite_pos, const_arr_size);
		zassert_equal(contin_arr[CONTIN_LAST_VAL_IDX], contin_last_val,
			      "Last value is not identical");
	}
}
