/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <stdio.h>


ZTEST(direct_xip_test, test_direct_xip)
{
	/* Checks if the test application boots up in the direct XiP mode. */
	zassert_true(true, "Needed to pass.");
}

ZTEST_SUITE(direct_xip_test, NULL, NULL, NULL, NULL, NULL);
