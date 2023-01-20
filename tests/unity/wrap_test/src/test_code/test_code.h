/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * DO NOT FORMAT THIS FILE!
 *
 * It contains various code blocks that is used to validate the test wrapper
 * scripts (func_name_list.py and header_parse.py) which renames function
 * declarations. The regular expression has had several bugs in the past, and
 * the formating of the code here is used to validate that the fixes still work
 * when modifying the regex.
 *
 * The code in this file ensure that the test wrapper scripts can handle the
 * situations described in following list of issues:
 * https://github.com/nrfconnect/sdk-nrf/pull/1413
 * https://github.com/nrfconnect/sdk-nrf/pull/2182
 * https://github.com/nrfconnect/sdk-nrf/pull/3432
 * https://github.com/nrfconnect/sdk-nrf/pull/5069
 * https://github.com/nrfconnect/sdk-nrf/pull/5814
 * https://github.com/nrfconnect/sdk-nrf/pull/6478
 * https://github.com/nrfconnect/sdk-nrf/pull/6547
 */

#ifndef __TEST_CODE_H
#define __TEST_CODE_H

// c++ comment
/* c comment */
/* c
 * multi line comment
 */
#define SOME_ADDR "https://my.page.com/somewhere"

static const char test_string[] = "test string";

#define TEST_MACRO(xx) do { \
	const STRUCT_SECTION_ITERABLE(settings_handler_static, \
			name) \
} while (0)

static inline int inline_static_fn(void)
{
	return 0;
}

inline static int static_inline_fn(void)
{
	return 0;
}

static ALWAYS_INLINE int always_inline_fn(void)
{
	return 0;
}

__syscall void syscall(void);

static inline void z_impl_syscall(void)
{
	/* empty */
}

/* macro function that was parsed incorrectly (NCSDK-10918) */
#define SETTINGS_STATIC_HANDLER_DEFINE(_hname, _tree, _get, _set, _commit,   \
				       _export)				     \
	const Z_STRUCT_SECTION_ITERABLE(settings_handler_static,	     \
					settings_handler_ ## _hname) = {     \
		.name = _tree,						     \
		.h_get = _get,						     \
		.h_set = _set,						     \
		.h_commit = _commit,					     \
		.h_export = _export,					     \
	}
int after_macro(void);

int 	extra_whitespace	 (void)	 ;

/* typedef that should not be parsed as a function */
typedef struct __attribute__ ((__packed__)) {
} bar_t;

/* newline between arguments */
int multiline_def(int arg,
					int len);

/* Test WORD_EXCLUDE parameter for cmock_handle() */
#define IGNORE_ME
IGNORE_ME void exclude_word_fn(void);

#endif /* __TEST_CODE_H */
