/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>

#include "bl_storage.h"
#include <zephyr/sys/reboot.h>

void test_monotonic_counter(void)
{
	int ret;

	printk("get_monotonic_counter() = %d\n", get_monotonic_counter());

	zassert_equal(CONFIG_SB_NUM_VER_COUNTER_SLOTS,
		num_monotonic_counter_slots(), NULL);
	zassert_equal(CONFIG_FW_INFO_FIRMWARE_VERSION,
		get_monotonic_counter() >> 1,
		NULL);
	zassert_equal(-EINVAL,
		set_monotonic_counter(CONFIG_FW_INFO_FIRMWARE_VERSION << 1),
		NULL);
	zassert_equal(-EINVAL, set_monotonic_counter(0), NULL);
	ret = set_monotonic_counter((CONFIG_FW_INFO_FIRMWARE_VERSION + 1) << 1);
	zassert_equal(0, ret, "ret %d\r\n", ret);
	zassert_equal(CONFIG_FW_INFO_FIRMWARE_VERSION + 1,
		get_monotonic_counter() >> 1, NULL);
	printk("Rebooting. Should fail to validate because of "
		   "monotonic counter.\n");
	sys_reboot(0);
}

void test_lcs_single(void)
{
	int ret;
	enum lcs lcs;

	ret = read_life_cycle_state(&lcs);
	zassert_equal(0, ret, "read lcs failed %d", ret);
	zassert_equal(lcs, BL_STORAGE_LCS_ASSEMBLY, "got wrong lcs, expected %d got %d",
		      BL_STORAGE_LCS_ASSEMBLY, lcs);
	ret = update_life_cycle_state(BL_STORAGE_LCS_PROVISIONING);
	zassert_equal(0, ret, "write lcs failed %d", ret);
	ret = read_life_cycle_state(&lcs);
	zassert_equal(0, ret, "read lcs failed with %d", ret);
	zassert_equal(lcs, BL_STORAGE_LCS_PROVISIONING, "got wrong lcs, expected %d got %d",
		      BL_STORAGE_LCS_PROVISIONING, lcs);
}

/* The rest of bl_storage's functionality is tested via the bl_validation
 * tests.
 */

void test_main(void)
{
	ztest_test_suite(test_bl_storage,
			 ztest_unit_test(test_lcs_single),
			 ztest_unit_test(test_monotonic_counter)
	);
	ztest_run_test_suite(test_bl_storage);
}
