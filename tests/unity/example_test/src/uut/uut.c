/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <uut.h>
#include <foo/foo.h>
#include <sys/util.h>
#include <stddef.h>

int uut_init(void *handle)
{
	int err;

	if (IS_ENABLED(CONFIG_UUT_PARAM_CHECK)) {
		if (handle == NULL) {
			return -1;
		}
	}

	err = foo_init(handle);

	if (err != 0) {
		return err;
	}

	return foo_execute() + foo_execute2();
}
