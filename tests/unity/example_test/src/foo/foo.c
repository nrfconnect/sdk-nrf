/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <foo/foo.h>

int foo_init(void *handle)
{
	/* This implementation will be wrapped and mocked. */
	return 0;
}

int foo_execute(void)
{
	return 0;
}
