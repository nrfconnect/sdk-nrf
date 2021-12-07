/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <ztest.h>
#include <cbor_encode.h>

static void test_raw_encode(void)
{
	uint8_t payload[32] = {0};
	uint8_t time[] = {1, 2, 3, 4, 5, 6, 7, 8};
	cbor_state_t states[4];
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
}

void test_main(void)
{
	ztest_test_suite(lib_cddl_gen_test3,
	     ztest_unit_test(test_raw_encode)
	 );

	ztest_run_test_suite(lib_cddl_gen_test3);
}
