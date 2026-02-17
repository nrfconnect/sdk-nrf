/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <stdio.h>


ZTEST(mcuboot_watchdog_feed_tests, test_mcuboot_watchdog_feed)
{
	/* If we reach here, MCUboot fed the watchdog during boot (no reset). */
	zassert_true(true, "Needed to pass.");
}

ZTEST_SUITE(mcuboot_watchdog_feed_tests, NULL, NULL, NULL, NULL, NULL);
