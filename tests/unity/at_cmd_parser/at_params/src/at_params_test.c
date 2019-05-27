/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <unity.h>
#include <at_params.h>
#include <errno.h>

#include "mock_at_params_alloc.h"

void setUp(void)
{
	/* Empty. */
}

void test_at_params_list_init_null_param(void)
{
	int err = at_params_list_init(NULL, 0);
	TEST_ASSERT_EQUAL(-EINVAL, err);
}

void test_at_params_list_init_no_mem(void)
{
	const size_t MAX_PARAMS = 4;

	/* Return a null pointer when allocating memory in the at_params module. */
	__wrap_at_params_calloc_ExpectAndReturn(MAX_PARAMS, sizeof(struct at_param), NULL);

	struct at_param_list list;
	int err = at_params_list_init(&list, MAX_PARAMS);
	TEST_ASSERT_EQUAL(-ENOMEM, err);
}
