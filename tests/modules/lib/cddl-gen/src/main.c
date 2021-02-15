/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <ztest.h>
#include "test_encode.h"

static void test_encode(void)
{
	Test_t test;

	zassert_true(&test != NULL, "Should not be NULL");
}

void test_main(void)
{
	ztest_test_suite(lib_cddl_gen_test,
	     ztest_unit_test(test_encode)
	 );

	ztest_run_test_suite(lib_cddl_gen_test);
}
