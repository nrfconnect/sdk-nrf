/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <ztest.h>

#include "bl_purse.h"
#include "power/reboot.h"

void test_monotonic_counter(void)
{
	printk("get_monotonic_counter() = %d\n", get_monotonic_counter());

	zassert_equal(CONFIG_SB_NUM_MONOTONIC_COUNTERS,
		num_monotonic_counter_slots(), NULL);
	zassert_equal(CONFIG_FW_INFO_FIRMWARE_VERSION, get_monotonic_counter(),
		NULL);
	zassert_equal(-EINVAL,
		set_monotonic_counter(CONFIG_FW_INFO_FIRMWARE_VERSION),
		NULL);
	zassert_equal(-EINVAL, set_monotonic_counter(0), NULL);
	zassert_equal(-EINVAL, set_monotonic_counter(0xFFFF), NULL);
	zassert_equal(0,
		set_monotonic_counter(CONFIG_FW_INFO_FIRMWARE_VERSION + 1),
		NULL);
	zassert_equal(CONFIG_FW_INFO_FIRMWARE_VERSION + 1,
		get_monotonic_counter(), NULL);
	printk("Rebooting. Should fail to validated because of "
		"monotonic counter.\n");
	sys_reboot(0);
}

/* The rest of bl_purse's functionality is tested via the bl_validation
 * tests.
 */

void test_main(void)
{
	ztest_test_suite(test_purse,
			 ztest_unit_test(test_monotonic_counter)
	);
	ztest_run_test_suite(test_purse);
}
