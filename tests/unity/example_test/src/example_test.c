/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <unity.h>
#include <uut.h>
#include <stdbool.h>
#include "foo/cmock_foo.h"

bool runtime_CONFIG_UUT_PARAM_CHECK;

#define TEST_FOO_ADDR FOO_ADDR

void setUp(void)
{
	runtime_CONFIG_UUT_PARAM_CHECK = false;
}

void test_uut_init(void)
{
	int err;

	__cmock_foo_init_ExpectAndReturn(NULL, 0);
	__cmock_foo_execute_ExpectAndReturn(0);

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

/* It is required to be added to each test. That is because unity's
 * main may return nonzero, while zephyr's main currently must
 * return 0 in all cases (other values are reserved).
 */
extern int unity_main(void);

int main(void)
{
	(void)unity_main();

	return 0;
}
