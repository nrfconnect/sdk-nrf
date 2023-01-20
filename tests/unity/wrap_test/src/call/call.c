/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/sys/util.h>
#include <stddef.h>

#include "call.h"
#include "test_code/test_code.h"

void call_syscall(void)
{
	syscall();
}

int call_static_inline_fn(void)
{
	return static_inline_fn();
}

int call_inline_static_fn(void)
{
	return inline_static_fn();
}

int call_always_inline_fn(void)
{
	return always_inline_fn();
}

int call_after_macro(void)
{
	return after_macro();
}

int call_extra_whitespace(void)
{
	return extra_whitespace();
}

int call_multiline_def(int arg, int len)
{
	return multiline_def(arg, len);
}

void call_exclude_word_fn(void)
{
	exclude_word_fn();
}
