/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>

#include "bl_storage.h"
#include <zephyr/sys/reboot.h>

void test_lcs_single(void)
{
	int ret;
	enum lcs lcs;

	ret = read_life_cycle_state(&lcs);
	zassert_equal(0, ret, "read lcs failed %d", ret);
	zassert_equal(lcs, BL_STORAGE_LCS_ASSEMBLY,
			"got wrong lcs, expected %d got %d", BL_STORAGE_LCS_ASSEMBLY, lcs);
	ret = update_life_cycle_state(BL_STORAGE_LCS_PROVISIONING);
	zassert_equal(0, ret, "write lcs failed %d", ret);
	ret = read_life_cycle_state(&lcs);
	zassert_equal(0, ret, "read lcs failed with %d", ret);
	zassert_equal(lcs, BL_STORAGE_LCS_PROVISIONING,
			"got wrong lcs, expected %d got %d", BL_STORAGE_LCS_PROVISIONING, lcs);
}

/* The rest of bl_storage's functionality is tested via the bl_validation
 * tests.
 */

void test_main(void)
{
	ztest_test_suite(test_bl_storage,
			 ztest_unit_test(test_lcs_single),
	);
	ztest_run_test_suite(test_bl_storage);
}
