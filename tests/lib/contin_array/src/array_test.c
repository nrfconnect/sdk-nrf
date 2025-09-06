/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
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

	char contin_first_val;
	char contin_last_val;
	uint32_t finite_pos = 0;

	for (int i = 0; i < NUM_ITERATIONS; i++) {
		ret = contin_array_create(contin_arr, CONTIN_ARR_SIZE, test_arr, const_arr_size,
					  &finite_pos);
		zassert_equal(ret, 0, "contin_array_create did not return zero");

		if (i == 0) { /* First run */
			zassert_equal(contin_arr[0], test_arr[0], "First value is not identical");
			zassert_equal(
				contin_arr[CONTIN_LAST_VAL_IDX], test_arr[CONTIN_LAST_VAL_IDX],
				"Last value is not identical 0x%x 0x%x",
				contin_arr[CONTIN_LAST_VAL_IDX], test_arr[CONTIN_LAST_VAL_IDX]);
		} else {
			/* The last val is the last element of the test_arr */
			if (contin_last_val == test_arr[const_arr_size - 1]) {
				zassert_equal(contin_arr[0], test_arr[0],
					      "First value is not identical after wrap");
			} else {
				zassert_equal(
					contin_arr[0], contin_last_val + 1,
					"First value is not identical to last val + 1. "
					"contin_arr[0]: 0x%04x, contin_last_val - 1: 0x%04x\n",
					contin_arr[0], contin_last_val + 1);
			}
		}

		contin_first_val = contin_arr[0];
		contin_last_val = contin_arr[CONTIN_LAST_VAL_IDX];
	}
}

/* Test with const array size being shorter than contin array size */
ZTEST(suite_contin_array, test_simp_arr_loop_short)
{
	const uint32_t NUM_ITERATIONS = 2000;
	const size_t CONTIN_ARR_SIZE = 97; /* Test with random "uneven" value */
	const uint32_t CONTIN_LAST_VAL_IDX = (CONTIN_ARR_SIZE - 1);
	/* Test with random "uneven" value, shorter than CONTIN_ARR_SIZE */
	const size_t const_arr_size = 44;
	char contin_arr[CONTIN_ARR_SIZE];
	int ret;

	char contin_first_val;
	char contin_last_val;
	uint32_t finite_pos = 0;

	for (int i = 0; i < NUM_ITERATIONS; i++) {
		ret = contin_array_create(contin_arr, CONTIN_ARR_SIZE, test_arr, const_arr_size,
					  &finite_pos);
		zassert_equal(ret, 0, "contin_array_create did not return zero");

		if (i == 0) { /* First run */
			zassert_equal(contin_arr[0], test_arr[0], "First value is not identical");
			zassert_equal(contin_arr[const_arr_size], test_arr[0],
				      "First repetition value is not identical");
		} else {
			/* The last val is the last element of the test_arr */
			if (contin_last_val == test_arr[const_arr_size - 1]) {
				zassert_equal(contin_arr[0], test_arr[0],
					      "First value is not identical after wrap");
			} else {
				zassert_equal(contin_arr[0], contin_last_val + 1,
					      "Last value is not identical. contin_arr[0]: 0x%04x, "
					      "contin_last_val - 1: 0x%04x\n",
					      contin_arr[0], contin_last_val + 1);
			}
		}

		contin_first_val = contin_arr[0];
		contin_last_val = contin_arr[CONTIN_LAST_VAL_IDX];
	}
}
