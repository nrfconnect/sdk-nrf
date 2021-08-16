/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/* Purpose: miscellaneous helpful things  */

#ifndef PTT_UTILS_H__
#define PTT_UTILS_H__

#include <stdint.h>

#define PTT_COMPILE_TIME_ASSERT_CAT_(a, b) a##b
#define PTT_COMPILE_TIME_ASSERT_CAT(a, b) PTT_COMPILE_TIME_ASSERT_CAT_(a, b)

#define PTT_COMPILE_TIME_ASSERT(cond)                                                              \
	typedef char PTT_COMPILE_TIME_ASSERT_CAT(assertion_failed_, __LINE__)[(cond) ? 1 : -1]

#define PTT_UNUSED(x) ((void)(x))

#define PTT_CAST_TO_UINT8_P(x) ((uint8_t *)(x))
#define PTT_CAST_TO_STR(x) ((char *)(x))

#endif /* PTT_UTILS_H__ */
