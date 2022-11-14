/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>

#include "bl_otp_counters.h"
#include <zephyr/sys/reboot.h>

void test_monotonic_counter(void)
{
	uint16_t counter_value;
	uint16_t counter_slots;
	/* The initial counter is written by B0 and the function set_monotonic_version.
	 * This function calculates the counter with the formula: counter = (version << 1) | !slot
	 * So for the version 10 (as configured in the FW info of this test) the value of the counter
	 * will actually be 21 when we start this test.
	 */

	zassert_equal(0, get_monotonic_counter(BL_MONOTONIC_COUNTERS_DESC_NSIB, &counter_value), NULL);
	printk("Counter value from get_montonic_counter() = %d\n", counter_value);

	zassert_equal(0,
		num_monotonic_counter_slots(BL_MONOTONIC_COUNTERS_DESC_NSIB, &counter_slots), NULL);

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

	zassert_equal(0, get_monotonic_counter(BL_MONOTONIC_COUNTERS_DESC_NSIB, &counter_value), NULL);
	zassert_equal(CONFIG_FW_INFO_FIRMWARE_VERSION + 1, counter_value >> 1, NULL);

	printk("Rebooting. Should fail to validate because of "
		   "monotonic counter.\n");
	sys_reboot(0);
}

/* The rest of bl_otp_counter's functionality is tested via the bl_validation
 * tests.
 */
void test_main(void)
{
	ztest_test_suite(test_bl_otp_counters,
			 ztest_unit_test(test_monotonic_counter)
	);
	ztest_run_test_suite(test_bl_otp_counters);
}
