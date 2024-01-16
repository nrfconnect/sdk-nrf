/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/ztest.h>
#include "test_encode.h"

ZTEST(lib_zcbor_test, test_encode)
{
	uint8_t payload[32];
	uint32_t payload_len;

	uint8_t first_name[] = "Foo";
	uint8_t last_name[] = "Bar";
	uint8_t time[] = {1, 2, 3, 4, 5, 6, 7, 8};
	struct Test test = {
		.Test_name_tstr = {
			{.value = first_name, .len = sizeof(first_name) - 1},
			{.value = last_name, .len = sizeof(last_name) - 1},
		},
		.Test_name_tstr_count = 2,
		.Test_timestamp = {.value = time, .len = sizeof(time)},
		.Test_types_choice = Test_types_something_c,
	};

	int res = cbor_encode_Test(payload, sizeof(payload), &test, &payload_len);

	zassert_equal(ZCBOR_SUCCESS, res, "Encoding should have been successful\n");
}

ZTEST_SUITE(lib_zcbor_test, NULL, NULL, NULL, NULL, NULL);
