/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>

ZTEST(bl_validation_ff_test, test_ff_key)
{
	/* This test has been built with a public key whose hash contains
	 * 0xFFFF, which can be manipulated when the key is in OTP. The
	 * bootloader checks this and should refuse to boot when it finds such
	 * a key hash.
	 */
	zassert_true(false, "Should not have booted.");
}

ZTEST_SUITE(bl_validation_ff_test, NULL, NULL, NULL, NULL, NULL);
