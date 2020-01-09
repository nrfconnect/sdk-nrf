/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <ztest.h>
#include <bl_validation.h>


void test_key_looping(void)
{
	/* If this boots, the test passed. The public key is at the end of the
	 * list, so the bootloader looped through all to validate this app.
	 */
}

void test_main(void)
{
	ztest_test_suite(test_bl_validation,
			 ztest_unit_test(test_key_looping)
	);
	ztest_run_test_suite(test_bl_validation);
}
