/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ztest.h>

void test_ff_key(void)
{
	/* This test has been built with a public key whose hash contains
	 * 0xFFFF, which can be manipulated when the key is in OTP. The
	 * bootloader checks this and should refuse to boot when it finds such
	 * a key hash.
	 */
	zassert_true(false, "Should not have booted.");
}

void test_main(void)
{
	ztest_test_suite(test_bl_crypto_ff_key,
			 ztest_unit_test(test_ff_key)
	);
	ztest_run_test_suite(test_bl_crypto_ff_key);
}
