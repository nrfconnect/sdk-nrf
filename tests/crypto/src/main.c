/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <ztest.h>
#include <common_test.h>

test_info_t test_info;           /**< Overall test results. */


void test_main(void)
{
	u32_t test_case_count = TEST_CASE_COUNT;
	struct unit_test test_suite[test_case_count + 1];

	for (u32_t i = 0; i < test_case_count; i++)
	{
		test_case_t * test_case = &__start_test_case_data[i];
		test_info.p_test_case_name = test_case->p_test_case_name;

		test_suite[i].name = test_case->p_test_case_name;
		test_suite[i].test = test_case->exec;
		test_suite[i].setup = test_case->setup;
		test_suite[i].teardown = test_case->teardown;
		test_suite[i].thread_options = 0;
	}
	test_suite[test_case_count].test = NULL;
	z_ztest_run_test_suite("SHA tests", test_suite);
}
