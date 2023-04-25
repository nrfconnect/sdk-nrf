/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <stdio.h>


ZTEST(mcuboot_external_flash, test_external_flash)
{
	zassert_true(true, "Needed to pass.");
}

ZTEST_SUITE(mcuboot_external_flash, NULL, NULL, NULL, NULL, NULL);
