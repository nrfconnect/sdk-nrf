/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __FOO_H
#define __FOO_H

#include <toolchain/common.h>

#include "foo_internal.h"
// c++ comment
/* c comment */
/* c
 * multi line comment
 */
#define FOO_ADDR "https://my.page.com/somewhere"

int foo_init(void *handle);

static const char test_string[] = "test string";

#define TEST_MACRO(xx) do { \
	const STRUCT_SECTION_ITERABLE(settings_handler_static, \
			name) \
} while (0)

static inline int foo_execute(void)
{
	foo_t val = 0;

	return (int)val;
}

static ALWAYS_INLINE int foo_execute2(void)
{
	return 0;
}

inline static int foo_execute3(void)
{
	return 0;
}

__syscall void foo_syscall(void);

static inline void z_impl_foo_syscall(void)
{
	/* empty */
}

#endif /* __FOO_H */
