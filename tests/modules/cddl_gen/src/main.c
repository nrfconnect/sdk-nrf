/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <string.h>
#include <zephyr/types.h>
#include <stdbool.h>
#include <ztest.h>

#include <pet_decode.h>

uint8_t buf[1024];

static void test_decode(void)
{
	/* Only for compilation testing */
	cbor_decode_Pet(NULL, 0, NULL, true);
	zassert_true(true, NULL);
}

void test_main(void)
{
	ztest_test_suite(lib_cddl_gen_test,
			 ztest_unit_test(test_decode)
			 );

	ztest_run_test_suite(lib_cddl_gen_test);
}
