/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>
#include <zephyr/ztest.h>
#include <fota_download_util.h>
#include "test_fota_download_common.h"

/* fota_download_parse_dual_resource_locator may modify the resource locator passed to it, so we
 * cannot directly pass test constants. Instead, we copy them to a modifiable buffer first.
 */
static char buf[1024];

char *const flash_ptr = S0_A " " S1_A;

ZTEST_SUITE(fota_download_parse_dual_resource_locator_tests, NULL, NULL, NULL, NULL, NULL);


/**
 * @brief Generic test of parse_dual_resource_locator
 * Not to be called directly by the test suite. Performs a parameterized test of
 * parse_dual_resource_locator
 *
 * @param resource_locator - The resource locator to be fed to parse_dual_resource_locator
 * @param expected_selection - The resource locator that parse_dual_resource_locator is expected to
 *			       select
 * @param s0_active - The slot activity state to be fed to parse_dual_resource_locator
 */
void test_fota_download_parse_dual_resource_locator_generic(const char * const resource_locator,
						    const char * const expected_selection,
						    const bool s0_active)
{
	int err;
	const char *update;

	strcpy(buf, resource_locator);
	err = fota_download_parse_dual_resource_locator(buf, s0_active, &update);

	zassert_equal(err, 0, "Selection of resource locator from valid dual resource locator "
			      " should not fail.");
	if (expected_selection == NULL) {
		zassert_is_null(update, "fota_download_parse_dual_resource_locator was expected to "
					"select NULL, and did not.");
	} else {
		zassert_not_null(update, "fota_download_parse_dual_resource_locator did not select "
					 "a resource locator");
		zassert_true(strcmp(expected_selection, update) == 0,
		     "fota_download_parse_dual_resource_locator did not select the expected "
		     "resource locator.");
	}

}

ZTEST(fota_download_parse_dual_resource_locator_tests, test_dual_A_s0_active)
{
	test_fota_download_parse_dual_resource_locator_generic(S0_A " " S1_A, S1_A, S0_ACTIVE);
}

ZTEST(fota_download_parse_dual_resource_locator_tests, test_dual_A_s1_active)
{
	test_fota_download_parse_dual_resource_locator_generic(S0_A " " S1_A, S0_A, S1_ACTIVE);
}

ZTEST(fota_download_parse_dual_resource_locator_tests, test_dual_B_s0_active)
{
	test_fota_download_parse_dual_resource_locator_generic(S0_B " " S1_B, S1_B, S0_ACTIVE);
}

ZTEST(fota_download_parse_dual_resource_locator_tests, test_dual_B_s1_active)
{
	test_fota_download_parse_dual_resource_locator_generic(S0_B " " S1_B, S0_B, S1_ACTIVE);
}

ZTEST(fota_download_parse_dual_resource_locator_tests, test_dual_C_s0_active)
{
	test_fota_download_parse_dual_resource_locator_generic(S0_C " " S1_C, S1_C, S0_ACTIVE);
}

ZTEST(fota_download_parse_dual_resource_locator_tests, test_dual_C_s1_active)
{
	test_fota_download_parse_dual_resource_locator_generic(S0_C " " S1_C, S0_C, S1_ACTIVE);
}

ZTEST(fota_download_parse_dual_resource_locator_tests, test_single_A)
{
	test_fota_download_parse_dual_resource_locator_generic(S0_A, NULL, S1_ACTIVE);
}

ZTEST(fota_download_parse_dual_resource_locator_tests, test_single_B)
{
	test_fota_download_parse_dual_resource_locator_generic(S1_B, NULL, S1_ACTIVE);
}

ZTEST(fota_download_parse_dual_resource_locator_tests, test_single_C)
{
	test_fota_download_parse_dual_resource_locator_generic(S0_C, NULL, S0_ACTIVE);
}

ZTEST(fota_download_parse_dual_resource_locator_tests, test_ptr_placement)
{
	/* This test cannot be executed on native posix as there is
	 * no mechanism in place do deduce if an address maps to non-volatile
	 * storage or RAM
	 *
	 * Recently, this test case was moved to a test suite without native posix support, so
	 * this skip condition is technically no longer necessary. It remains active in case native
	 * posix support is ever re-added.
	 */

	Z_TEST_SKIP_IFDEF(CONFIG_BOARD_NATIVE_POSIX);

	int err;
	const char *update;
	bool s0_active = true;

	err = fota_download_parse_dual_resource_locator(flash_ptr, s0_active, &update);
	zassert_true(err != 0, "Did not fail when given flash pointer");
}

ZTEST(fota_download_parse_dual_resource_locator_tests, test_no_separator)
{
	int err;
	const char *update;
	bool s0_active = true;

	strcpy(buf, S0_B S1_B);
	err = fota_download_parse_dual_resource_locator(buf, s0_active, &update);

	zassert_equal(err, 0, "Should not get error when missing separator");
	zassert_equal(update, NULL, "update should be NULL when no separator");
}

ZTEST(fota_download_parse_dual_resource_locator_tests, test_null)
{
	int err;
	const char *update;
	bool s0_active = true;

	err = fota_download_parse_dual_resource_locator(NULL, s0_active, &update);
	zassert_true(err < 0, NULL);

	err = fota_download_parse_dual_resource_locator(buf, s0_active, NULL);
	zassert_true(err < 0, NULL);
}

ZTEST(fota_download_parse_dual_resource_locator_tests, test_not_terminated)
{
	int err;
	const char *update;
	bool s0_active = true;

	/* Remove any null terminator */
	for (int i = 0; i < sizeof(buf); ++i) {
		buf[i] = 'a';
	}
	err = fota_download_parse_dual_resource_locator(buf, s0_active, &update);
	zassert_true(err < 0, NULL);
}

ZTEST(fota_download_parse_dual_resource_locator_tests, test_empty)
{
	int err;
	const char *update;
	bool s0_active = true;
	char empty[] = "";

	err = fota_download_parse_dual_resource_locator(empty, s0_active, &update);
	zassert_true(update == NULL, "update should not be set");
}
