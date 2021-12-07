/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <ztest.h>
#include "test_decode.h"
#include "cbor_encode.h"

static void test_decode(void)
{
	uint8_t payload[32] = {0};
	uint32_t payload_len;
	uint8_t time[] = {1, 2, 3, 4, 5, 6, 7, 8};
	cbor_state_t states[4];
	struct Test test;
	bool res;

	new_state(states, 4, payload, sizeof(payload), 1);

	res = list_start_encode(states, 3);
	res |= list_start_encode(states, 2);
	res |= tstrx_put_term(states, "Foo");
	res |= tstrx_put_term(states, "Bar");
	res |= list_end_encode(states, 0);
	res |= bstrx_encode(states, &(cbor_string_type_t){.value = time, .len = (sizeof(time))});
	res |= intx32_put(states, 2);
	res |= list_end_encode(states, 0);
	zassert_true(res, "Encoding should have been successful\n");

	uint32_t len = states->payload - payload;

	res = cbor_decode_Test(payload, len, &test, &payload_len);

	zassert_true(res, "Decoding should have been successful\n");

	zassert_mem_equal(test._Test_name_tstr[0].value, "Foo", test._Test_name_tstr[0].len, NULL);
	zassert_mem_equal(test._Test_name_tstr[1].value, "Bar", test._Test_name_tstr[1].len, NULL);
}

void test_main(void)
{
	ztest_test_suite(lib_cddl_gen_test1,
	     ztest_unit_test(test_decode)
	 );

	ztest_run_test_suite(lib_cddl_gen_test1);
}
