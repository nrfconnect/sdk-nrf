/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <unity.h>
#include <at_params.h>
#include <errno.h>

void setUp(void)
{
	/* Empty. */
}

void test_at_params_list_init_null_param(void)
{
	int err = at_params_list_init(NULL, 0);
	TEST_ASSERT_EQUAL(-EINVAL, err);
}
