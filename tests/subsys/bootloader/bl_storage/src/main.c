/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ztest.h>

#include "bl_storage.h"
#include "power/reboot.h"

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

/* The rest of bl_storage's functionality is tested via the bl_validation
 * tests.
 */

void test_main(void)
{
	ztest_test_suite(test_bl_storage,
			 ztest_unit_test(test_monotonic_counter)
	);
	ztest_run_test_suite(test_bl_storage);
}
