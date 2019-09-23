/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef __FOO_H
#define __FOO_H

#include <toolchain/common.h>

int foo_init(void *handle);

static inline int foo_execute(void)
{
	return 0;
}

static ALWAYS_INLINE int foo_execute2(void)
{
	return 0;
}

#endif /* __FOO_H */
