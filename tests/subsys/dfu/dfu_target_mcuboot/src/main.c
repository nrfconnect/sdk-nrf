/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <string.h>
#include <zephyr/types.h>
#include <stdbool.h>
#include <ztest.h>
#include <dfu_target.h>
#include <dfu_target_mcuboot.h>

/* Create buffer which we will fill with strings to test with.
 * This is needed since 'dfu_ctx_Mcuboot_set_b1_file` will modify its
 * 'file' parameter, hence we can not provide it raw c strings as they
 * are stored in read-only memory
 */
char buf[1024];

#define S0_S1 "s0 s1"
#define NO_SPACE "s0s1"

static void test_dfu_ctx_mcuboot_set_b1_file(void)
{
	int err;
	const char *update;
	bool s0_active = true;

	memcpy(buf, S0_S1, sizeof(S0_S1));
	err = dfu_ctx_mcuboot_set_b1_file(buf, s0_active, &update);

	zassert_equal(err, 0, NULL);
	zassert_true(strcmp("s1", update) == 0, NULL);

	s0_active = false;
	err = dfu_ctx_mcuboot_set_b1_file(buf, s0_active, &update);

	zassert_equal(err, 0, NULL);
	zassert_true(strcmp("s0", update) == 0, NULL);
}

static void test_dfu_ctx_mcuboot_set_b1_file__no_separator(void)
{
	int err;
	const char *update;
	bool s0_active = true;

	memcpy(buf, NO_SPACE, sizeof(NO_SPACE));
	err = dfu_ctx_mcuboot_set_b1_file(buf, s0_active, &update);

	zassert_equal(err, 0, "Should not get error when missing separator");
	zassert_equal(update, NULL, "update should be NULL when no separator");
}

static void test_dfu_ctx_mcuboot_set_b1_file__null(void)
{
	int err;
	const char *update;
	bool s0_active = true;

	err = dfu_ctx_mcuboot_set_b1_file(NULL, s0_active, &update);
	zassert_true(err < 0, NULL);

	err = dfu_ctx_mcuboot_set_b1_file(buf, s0_active, NULL);
	zassert_true(err < 0, NULL);
}

static void test_dfu_ctx_mcuboot_set_b1_file__not_terminated(void)
{
	int err;
	const char *update;
	bool s0_active = true;

	/* Remove any null terminator */
	for (int i = 0; i < sizeof(buf); ++i) {
		buf[i] = 'a';
	}
	err = dfu_ctx_mcuboot_set_b1_file(buf, s0_active, &update);
	zassert_true(err < 0, NULL);
}

static void test_dfu_ctx_mcuboot_set_b1_file__empty(void)
{
	int err;
	const char *update;
	bool s0_active = true;

	err = dfu_ctx_mcuboot_set_b1_file("", s0_active, &update);
	zassert_true(update == NULL, "update should not be set");
}

void test_main(void)
{
	ztest_test_suite(lib_dfu_target_mcuboot_test,
	     ztest_unit_test(test_dfu_ctx_mcuboot_set_b1_file__no_separator),
	     ztest_unit_test(test_dfu_ctx_mcuboot_set_b1_file__null),
	     ztest_unit_test(test_dfu_ctx_mcuboot_set_b1_file__not_terminated),
	     ztest_unit_test(test_dfu_ctx_mcuboot_set_b1_file__empty),
	     ztest_unit_test(test_dfu_ctx_mcuboot_set_b1_file)
	 );

	ztest_run_test_suite(lib_dfu_target_mcuboot_test);
}
