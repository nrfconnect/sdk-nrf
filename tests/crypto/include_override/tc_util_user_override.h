/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef __TC_UTIL_USER_OVERRIDE_H__
#define __TC_UTIL_USER_OVERRIDE_H__

#include <string.h>

#ifdef PRINT_LINE
#error "PRINT_LINE already defined!"
#else
/*
 * A separator is not strictly needed.
 */
#define PRINT_LINE
#endif

#ifdef TC_START
#error "TC_START already defined!"
#else
/*
 * Print the test number, the test vector name, and add a delimiter.
 */
#define TC_START(s)                                                            \
	do {                                                                   \
		const char *DELIM = " -- ";                                    \
		static int count;                                              \
		char name[128] = { 0 };                                        \
		memcpy(name, s, strstr(s, DELIM) - s);                         \
		printk("%d: %s%s", ++count, name, DELIM);                      \
	} while (0)
#endif

#ifdef Z_TC_END_RESULT
#error "Z_TC_END_RESULT already defined!"
#else
/*
 * Print the result, a delimiter, and where the test vector was taken from.
 */
#define Z_TC_END_RESULT(result, s)                                             \
	do {                                                                   \
		const char *DELIM = " -- ";                                    \
		char *info = strstr(s, DELIM) + strlen(DELIM);                 \
		printk("%s%s%s\n", TC_RESULT_TO_STR(result), DELIM, info);     \
	} while (0)
#endif

#ifdef TC_PASS_STR
#error "TC_PASS_STR already defined!"
#else
#define TC_PASS_STR "PASS"
#endif

#ifdef TC_FAIL_STR
#error "TC_FAIL_STR already defined!"
#else
#define TC_FAIL_STR "FAILED"
#endif

#ifdef TC_SKIP_STR
#error "TC_SKIP_STR already defined!"
#else
#define TC_SKIP_STR "SKIPPED"
#endif

#endif /* __TC_UTIL_USER_OVERRIDE_H__ */
