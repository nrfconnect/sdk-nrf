/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>

#include "bl_storage.h"
#include <zephyr/sys/reboot.h>

ZTEST(bl_storage_test, test_monotonic_counter)
{
	uint16_t counter_value;
	uint16_t counter_slots;
	/* The initial counter is written by B0 and the function
	 * set_monotonic_version. This function calculates the counter
	 * with the formula: counter = (version << 1) | !slot. So for the
	 * version 10 (as configured in the FW info of this test) the
	 * value of the counter will actually be 21 when we start this
	 * test.
	 */

	zassert_equal(0, get_monotonic_counter(BL_MONOTONIC_COUNTERS_DESC_NSIB, &counter_value),
		      NULL);
	printk("Counter value from get_montonic_counter() = %d\n", counter_value);

	zassert_equal(0,
		      num_monotonic_counter_slots(BL_MONOTONIC_COUNTERS_DESC_NSIB, &counter_slots),
		      NULL);

	zassert_equal(CONFIG_SB_NUM_VER_COUNTER_SLOTS, counter_slots, NULL);

	zassert_equal(CONFIG_FW_INFO_FIRMWARE_VERSION, counter_value >> 1, NULL);

	zassert_equal(-EINVAL,
		      set_monotonic_counter(BL_MONOTONIC_COUNTERS_DESC_NSIB,
					    CONFIG_FW_INFO_FIRMWARE_VERSION << 1),
		      NULL);

	zassert_equal(-EINVAL, set_monotonic_counter(BL_MONOTONIC_COUNTERS_DESC_NSIB, 0), NULL);

	zassert_equal(0,
		      set_monotonic_counter(BL_MONOTONIC_COUNTERS_DESC_NSIB,
					    (CONFIG_FW_INFO_FIRMWARE_VERSION + 1) << 1),
		      NULL);

	zassert_equal(0, get_monotonic_counter(BL_MONOTONIC_COUNTERS_DESC_NSIB, &counter_value),
		      NULL);
	zassert_equal(CONFIG_FW_INFO_FIRMWARE_VERSION + 1, counter_value >> 1, NULL);

	printk("Rebooting. Should fail to validate because of "
	       "monotonic counter.\n");
	sys_reboot(0);
}

ZTEST(bl_storage_test, test_lcs_single)
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

ZTEST_SUITE(bl_storage_test, NULL, NULL, NULL, NULL, NULL);
