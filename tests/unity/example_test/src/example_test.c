/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <unity.h>
#include <uut.h>
#include <stdbool.h>
#include "foo/mock_foo.h"

bool runtime_CONFIG_UUT_PARAM_CHECK;

#define TEST_FOO_ADDR FOO_ADDR

void setUp(void)
{
	runtime_CONFIG_UUT_PARAM_CHECK = false;
}

/* Suite teardown shall finalize with mandatory call to generic_suiteTearDown. */
extern int generic_suiteTearDown(int num_failures);

int test_suiteTearDown(int num_failures)
{
	return generic_suiteTearDown(num_failures);
}

void test_uut_init(void)
{
	int err;

	__wrap_foo_init_ExpectAndReturn(NULL, 0);
	__wrap_foo_execute_ExpectAndReturn(0);
	__wrap_foo_execute2_ExpectAndReturn(0);

	err = uut_init(NULL);
	TEST_ASSERT_EQUAL(0, err);
}

void test_uut_init_with_param_check(void)
{
	int err;

	/* Enable compile time option to check input parameters. */
	runtime_CONFIG_UUT_PARAM_CHECK = true;

	err = uut_init(NULL);
	TEST_ASSERT_EQUAL(-1, err);
}

/* It is required to be added to each test. That is because unity is using
 * different main signature (returns int) and zephyr expects main which does
 * not return value.
 */
extern int unity_main(void);

void main(void)
{
	(void)unity_main();
}
