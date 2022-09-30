/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/ztest.h>
#include <zcbor_encode.h>

static void test_raw_encode(void)
{
	uint8_t payload[32] = {0};
	uint8_t time[] = {1, 2, 3, 4, 5, 6, 7, 8};

	ZCBOR_STATE_E(states, 4, payload, sizeof(payload), 1);
	bool res;

	res = zcbor_list_start_encode(states, 3);
	res |= zcbor_list_start_encode(states, 2);
	res |= zcbor_tstr_put_term(states, "Foo");
	res |= zcbor_tstr_put_term(states, "Bar");
	res |= zcbor_list_end_encode(states, 0);
	res |= zcbor_bstr_put_arr(states, time);
	res |= zcbor_int32_put(states, 2);
	res |= zcbor_list_end_encode(states, 0);

	zassert_true(res, "Encoding should have been successful\n");
}

void test_main(void)
{
	ztest_test_suite(lib_zcbor_test3,
	     ztest_unit_test(test_raw_encode)
	 );

	ztest_run_test_suite(lib_zcbor_test3);
}
