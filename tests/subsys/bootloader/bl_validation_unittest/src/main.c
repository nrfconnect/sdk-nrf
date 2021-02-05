/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ztest.h>
#include <../subsys/bootloader/bl_validation/bl_validation_internal.h>

void test_within(void)
{
	zassert_false(within(10, 10, 10), NULL);
	zassert_false(within(10, 10, 9), NULL);
	zassert_false(within(0, 0xFFFFFFFF, 1), NULL);
	zassert_true(within(0x10000, 0x10000, 0x100000), NULL);
	zassert_true(within(0x10001, 0x10000, 0x100000), NULL);
	zassert_true(within(0xFFFFF, 0x10000, 0x100000), NULL);
	zassert_false(within(0xFFFF, 0x10000, 0x100000), NULL);
	zassert_false(within(0x100000, 0x10000, 0x100000), NULL);
	zassert_false(within(0x100001, 0x10000, 0x100000), NULL);
}


void test_region_within(void)
{
	zassert_true(region_within(0x10000, 0x100000, 0x10000, 0x100000), NULL);
	zassert_true(region_within(0x20000, 0x20000, 0x10000, 0x100000), NULL);
	zassert_false(region_within(0x20001, 0x20000, 0x10000, 0x100000), NULL);
	zassert_true(region_within(0x20001, 0x100000, 0x10000, 0x100000), NULL);
	zassert_false(region_within(0xFFFF, 0x20000, 0x10000, 0x100000), NULL);
}

void test_main(void)
{
	ztest_test_suite(test_bl_validation_unittest,
			 ztest_unit_test(test_within),
			 ztest_unit_test(test_region_within)
	);
	ztest_run_test_suite(test_bl_validation_unittest);
}
