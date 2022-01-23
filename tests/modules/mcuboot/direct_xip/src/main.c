/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ztest.h>
#include <stdio.h>


void test_direct_xip(void)
{
	zassert_true(true, "Needed to pass.");
}

void test_main(void)
{
	ztest_test_suite(test_mcuboot_direct_xip,
			 ztest_unit_test(test_direct_xip)
	);
	ztest_run_test_suite(test_mcuboot_direct_xip);
}
