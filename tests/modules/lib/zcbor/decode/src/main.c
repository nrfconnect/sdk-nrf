/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/ztest.h>
#include "test_decode.h"
#include "zcbor_encode.h"

ZTEST(lib_zcbor_test1, test_decode)
{
	uint8_t payload[32] = {0};
	uint32_t payload_len;
	uint8_t time[] = {1, 2, 3, 4, 5, 6, 7, 8};

	ZCBOR_STATE_D(states, 4, payload, sizeof(payload), 1);
	struct Test test;
	bool res;
	int int_res;

	/* Initialize struct to ensure test isn't checking uninitialized pointers */
	test._Test_name_tstr[0] = (struct zcbor_string){NULL, 0};
	test._Test_name_tstr[1] = (struct zcbor_string){NULL, 0};

	res = zcbor_list_start_encode(states, 3);
	res |= zcbor_list_start_encode(states, 2);
	res |= zcbor_tstr_put_term(states, "Foo");
	res |= zcbor_tstr_put_term(states, "Bar");
	res |= zcbor_list_end_encode(states, 0);
	res |= zcbor_bstr_put_arr(states, time);
	res |= zcbor_int32_put(states, 2);
	res |= zcbor_list_end_encode(states, 0);
	zassert_true(res, "Encoding should have been successful\n");

	uint32_t len = states->payload - payload;

	int_res = cbor_decode_Test(payload, len, &test, &payload_len);

	zassert_equal(ZCBOR_SUCCESS, int_res, "Decoding should have been successful\n");

	zassert_mem_equal(test._Test_name_tstr[0].value, "Foo", test._Test_name_tstr[0].len, NULL);
	zassert_mem_equal(test._Test_name_tstr[1].value, "Bar", test._Test_name_tstr[1].len, NULL);
}

ZTEST_SUITE(lib_zcbor_test1, NULL, NULL, NULL, NULL, NULL);
