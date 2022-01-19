/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <unity.h>
#include <stdbool.h>
#include "call.h"
#include "test_code/mock_test_code.h"

/*
 * These tests verify the wrapping functionality in scripts/unity that parse
 * C header files and rename functions using python and regular expressions.
 */

void test_syscall(void)
{
	__wrap_syscall_Expect();

	call_syscall();
}

void test_comment_removal_allow_url(void)
{
	TEST_ASSERT_EQUAL_STRING("https://my.page.com/somewhere", SOME_ADDR);
}

void test_static_inline_fn(void)
{
	const int EXPECT_STATIC_INLINE = 1;

	__wrap_static_inline_fn_ExpectAndReturn(EXPECT_STATIC_INLINE);

	TEST_ASSERT_EQUAL(EXPECT_STATIC_INLINE, call_static_inline_fn());
}

void test_inline_static_fn(void)
{
	const int EXPECT_INLINE_STATIC = 2;

	__wrap_inline_static_fn_ExpectAndReturn(EXPECT_INLINE_STATIC);

	TEST_ASSERT_EQUAL(EXPECT_INLINE_STATIC, call_inline_static_fn());
}

void test_always_inline_fn(void)
{
	const int EXPECT_ALWAYS_INLINE = 3;

	__wrap_always_inline_fn_ExpectAndReturn(EXPECT_ALWAYS_INLINE);

	TEST_ASSERT_EQUAL(EXPECT_ALWAYS_INLINE, call_always_inline_fn());
}

void test_after_macro(void)
{
	const int EXPECT_AFTER_MACRO = 4;

	__wrap_after_macro_ExpectAndReturn(EXPECT_AFTER_MACRO);

	TEST_ASSERT_EQUAL(EXPECT_AFTER_MACRO, call_after_macro());
}

void test_extra_whitespace(void)
{
	const int EXPECT_EXTRA_WHITESPACE = 5;

	__wrap_extra_whitespace_ExpectAndReturn(EXPECT_EXTRA_WHITESPACE);

	TEST_ASSERT_EQUAL(EXPECT_EXTRA_WHITESPACE, call_extra_whitespace());
}

void test_multiline_def(void)
{
	const int EXPECT_MULTILINE_DEF = 6;

	__wrap_multiline_def_ExpectAndReturn(1, 2, EXPECT_MULTILINE_DEF);

	TEST_ASSERT_EQUAL(EXPECT_MULTILINE_DEF, call_multiline_def(1, 2));
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
