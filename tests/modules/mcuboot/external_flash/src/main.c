/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ztest.h>
#include <stdio.h>


void test_external_flash(void)
{
	zassert_true(true, "Needed to pass.");
}

void test_main(void)
{
	ztest_test_suite(test_mcuboot_external_flash,
			 ztest_unit_test(test_external_flash)
	);
	ztest_run_test_suite(test_mcuboot_external_flash);
}
