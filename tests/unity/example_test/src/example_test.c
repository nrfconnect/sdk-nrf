/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <unity.h>
#include <uut.h>
#include <stdbool.h>
#include "foo/mock_foo.h"

bool runtime_CONFIG_UUT_PARAM_CHECK;

void setUp(void)
{
	runtime_CONFIG_UUT_PARAM_CHECK = false;
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
